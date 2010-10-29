#include <libfrozen.h>
#include <backend.h>
#include <alloca.h>

typedef enum locate_mode {
	LINEAR_INCAPSULATED,  // public oid converted to private oid using div()
	LINEAR_ORIGINAL,      // public oid same as private oid, insecure
	INDEX_INCAPSULATED,   // public oid converted to private oid using index
	INDEX_ORIGINAL        // public oid same as private oid, but checked for existance via index
} locate_mode;

typedef struct locate_ud {
	locate_mode    mode;
	unsigned int   linear_scale;
	data_ctx_t     ctx;
	backend_t     *backend;
	data_type      backend_type;
	
	off_t          oid;
	data_type      oid_type;
	buffer_t       oid_buffer;
	buffer_t       idx_buffer;
} locate_ud;

static int locate_init(chain_t *chain){
	locate_ud *user_data = (locate_ud *)calloc(1, sizeof(locate_ud));
	if(user_data == NULL)
		return -ENOMEM;
	
	chain->user_data = user_data;
	return 0;
}

static int locate_destroy(chain_t *chain){
	locate_ud *data = (locate_ud *)chain->user_data;
	
	data_ctx_destroy(&data->ctx);
	if(data->backend != NULL)
		backend_destroy(data->backend);
	
	buffer_destroy(&data->oid_buffer);
	buffer_destroy(&data->idx_buffer);
	
	free(data);
	chain->user_data = NULL;
	return 0;
}

static int locate_configure(chain_t *chain, hash_t *config){
	unsigned int   linear_scale;
	char          *mode_str;
	void          *temp;
	hash_t        *r_backend;
	locate_ud     *data                = (locate_ud *)chain->user_data;

	if(hash_get_typed(config, TYPE_STRING, "mode", (void **)&mode_str, NULL) != 0)
		return_error(-EINVAL, "locate 'mode' not defined\n");
	
	      if(strcasecmp(mode_str, "linear-incapsulated") == 0){
		data->mode = LINEAR_INCAPSULATED;
	}else if(strcasecmp(mode_str, "linear-original") == 0){
		data->mode = LINEAR_ORIGINAL;
	}else if(strcasecmp(mode_str, "index-incapsulated") == 0){
		data->mode = INDEX_INCAPSULATED;
	}else if(strcasecmp(mode_str, "index-original") == 0){
		data->mode = INDEX_ORIGINAL;
	}else{
		return_error(-EINVAL, "locate 'mode' invalid or not supported\n");
	}
	
	if(hash_get_typed(config, TYPE_STRING, "oid-type", &temp, NULL) != 0)
		return_error(-EINVAL, "locate 'oid-type' not defined\n");
	if( (data->oid_type = data_type_from_string((char *)temp)) == -1)
		return_error(-EINVAL, "locate 'oid-type' invalid\n");
	buffer_init_from_bare(&data->oid_buffer, &data->oid, sizeof(data->oid)); // TODO support any oid
	
	buffer_init(&data->idx_buffer);
	
	linear_scale =
		(hash_get_typed(config, TYPE_INT32, "size", &temp, NULL) == 0) ?
		*(unsigned int *)temp : 0;
	
	if( (r_backend = hash_find(config, "backend")) != NULL){
		if( (data->backend = backend_new(r_backend)) == NULL)
			return_error(-EINVAL, "chain 'blocks' variable 'backend' invalid\n");
		
		if(hash_get_typed(config, TYPE_STRING, "backend-type", &temp, NULL) != 0)
			return_error(-EINVAL, "locate 'backend-type' not defined\n");
		if( (data->backend_type = data_type_from_string((char *)temp)) == -1)
			return_error(-EINVAL, "locate 'backend-type' invalid\n");
	}
	
	/* check everything */
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(
			      //oid_class_proto->func_add       == NULL       ||
			      //oid_class_proto->func_divide    == NULL       ||
			      //oid_class_size_type             != SIZE_FIXED ||
				linear_scale                    == 0
			)
				return_error(-EINVAL, "locate 'oid-type' not supported\n");
			break;
		case INDEX_INCAPSULATED:
			if(data->backend == NULL)
				return_error(-EINVAL, "locate 'backend' not supplied\n");
			break;
		case INDEX_ORIGINAL:
		case LINEAR_ORIGINAL:
			// everything ok
			break;
	};
	
	data_ctx_init(&data->ctx, data->oid_type, NULL); 
	
	data->linear_scale = linear_scale;
	
	// get backend name from 'index' 
	
	return 0;
}

static ssize_t incapsulate(locate_ud *data, buffer_t *buffer){
	// divide returned key on data->size
	if(data_buffer_arithmetic(&data->ctx, buffer, 0, '/', data->linear_scale) != 0){
		// TODO linear incapsulation can't really deal with keys that
		// not cleanly divide on linear_scale. For now we return -EIO,
		// but later need call some pad() to normalize or fix scaling
		
		return -EIO;
	}
	return 0;
}

static ssize_t incapsulate_ret(locate_ud *data, ssize_t ret){
	div_t  div_res;
	div_res = div(ret, data->linear_scale);
	
	if(div_res.rem != 0)             // return rounded up integer, coz user can write smaller data than size of one item
		return div_res.quot + 1;
	
	return div_res.quot;
}

static ssize_t locate_create(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t      ret  = -1;
	locate_ud  *data = (locate_ud *)chain->user_data;
	hash_t      *r_size;
	void        *o_size;
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if( (r_size = hash_find(request, "size")) == NULL)
				return -EINVAL;
			
			o_size = alloca(r_size->value_size);
			memcpy(o_size, r_size->value, r_size->value_size);
			if(data_bare_arithmetic(r_size->type, o_size, '*', data->linear_scale) != 0)
				return -EINVAL;
			
			hash_t  new_request[] = {
				{ "size", r_size->type, o_size, r_size->value_size },
				hash_next(request)
			};
			
			if( (ret = chain_next_query(chain, new_request, buffer)) <= 0)
				return ret;
			
			if(incapsulate(data, buffer) != 0)
				return -EIO;
			
			break;
		
		case INDEX_INCAPSULATED:
			// query undelying chain to create space for info
			ret = chain_next_query(chain, request, buffer);
			if(ret <= 0)
				return ret;
			
			// write to index new key
			hash_t  idx_request[] = {
				{ "action", DATA_INT32(ACTION_CRWD_WRITE) },
				{ "size",   DATA_INT32(1)                 },
				hash_end
			};
			ret = backend_query(data->backend, idx_request, buffer);
			if(ret <= 0)
				return ret;
			// backend_query return buffer with oid
			break;
			
		case INDEX_ORIGINAL:
			// ret = check_existance(key);
			// if(ret != 0)
			// 	return ret;
			
			// fall through
		case LINEAR_ORIGINAL:
			ret = chain_next_query(chain, request, buffer);
			break;
	};
	
	return ret;
}

static ssize_t locate_setgetdelete(chain_t *chain, request_t *request, buffer_t *buffer){
	hash_t      *r_key, *r_size, *r_action;
	void        *o_key, *o_size;
	unsigned int action;
	ssize_t      ret  = -1;
	locate_ud   *data = (locate_ud *)chain->user_data;
	
	r_action = hash_find(request, "action");
	r_key    = hash_find(request, "key");
	r_size   = hash_find(request, "size");
	
	action = HVALUE(r_action, unsigned int);
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if(r_key == NULL || r_size == NULL) return -EINVAL;
			
			o_key  = alloca(r_key->value_size);
			o_size = alloca(r_size->value_size);
			memcpy(o_key,  r_key->value,  r_key->value_size);
			memcpy(o_size, r_size->value, r_size->value_size);
			
			if(data_bare_arithmetic(r_key->type,  o_key,  '*', data->linear_scale) != 0) return -EINVAL;
			if(data_bare_arithmetic(r_size->type, o_size, '*', data->linear_scale) != 0) return -EINVAL;
			
			hash_t  new_request[] = {
				{ "key",  r_key->type,  o_key,  r_key->value_size  },
				{ "size", r_size->type, o_size, r_size->value_size },
				hash_next(request)
			};
			
			if( (ret = chain_next_query(chain, new_request, buffer)) <= 0)
				return ret;
			
			ret = incapsulate_ret(data, ret);
			break;
			
		case INDEX_INCAPSULATED:
			// if no key supplied - create new item
			if(action == ACTION_CRWD_WRITE && r_key == NULL){
				return locate_create(chain, request, buffer);
			}
			
			// query index
			hash_t  idx_req_get[] = {
				{ "action",   DATA_INT32(ACTION_CRWD_READ) },
				{ "size",     DATA_INT32(1)                },
				hash_next(request)
			};
			ret = backend_query(data->backend, idx_req_get, &data->idx_buffer);
			
			// convert idx_buffer to oid_buffer
			// data->backend_type
			buffer_read(&data->idx_buffer, 0, &data->oid, sizeof(data->oid));
			
			// query data
			hash_t  data_request[] = {
				{ "key",  data->backend_type, &data->oid, sizeof(data->oid) },
				hash_next(request)
			};
			ret = chain_next_query(chain, data_request, buffer);
			if(ret <= 0)
				return ret;
			
			if(action == ACTION_CRWD_DELETE){
				// delete from index
			}
			break;
			
		case INDEX_ORIGINAL:
			// ret = check_existance(key);
			// if(ret != 0)
			// 	return ret;
			
			// fall through
		case LINEAR_ORIGINAL:
			ret = chain_next_query(chain, request, buffer);
			break;
	};
	
	return ret;
}

static ssize_t locate_move(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t      ret  = -1;
	locate_ud  *data = (locate_ud *)chain->user_data;
	hash_t      *r_key_from, *r_key_to;
	hash_t      *r_size;
	char        *r_size_str = hash_ptr_null;
	void        *o_key_from, *o_key_to, *o_size;
	
	switch(data->mode){
		case LINEAR_INCAPSULATED:
			if( (r_key_from = hash_find(request, "key_from")) == NULL) return -EINVAL;
			if( (r_key_to   = hash_find(request, "key_to"))   == NULL) return -EINVAL;
			
			if( (r_size     = hash_find(request, "size")) != NULL){ // size is optional
				o_size     = alloca(r_size->value_size);
				memcpy(o_size,     r_size->value,     r_size->value_size);
				if(data_bare_arithmetic(r_size->type,     o_size,     '*', data->linear_scale) != 0) return -EINVAL;
				r_size_str = r_size->key;
			}
			
			o_key_from = alloca(r_key_from->value_size);
			o_key_to   = alloca(r_key_to->value_size);
			
			memcpy(o_key_from, r_key_from->value, r_key_from->value_size);
			memcpy(o_key_to,   r_key_to->value,   r_key_to->value_size);
			
			if(data_bare_arithmetic(r_key_from->type, o_key_from, '*', data->linear_scale) != 0) return -EINVAL;
			if(data_bare_arithmetic(r_key_to->type,   o_key_to,   '*', data->linear_scale) != 0) return -EINVAL;
			
			hash_t  new_request[] = {
				{ r_key_from->key,  r_key_from->type, o_key_from, r_key_from->value_size  },
				{ r_key_to->key,    r_key_to->type,   o_key_to,   r_key_to->value_size    },
				{ r_size_str,       r_size->type    , o_size,     r_size->value_size      },
				hash_next(request)
			};
			
			ret = chain_next_query(chain, new_request, buffer);
			break;
			
		case INDEX_INCAPSULATED:
			// ret = check_existance(key, buffer);
			// if(ret <= 0)
			// 	return ret;
			//
			// hash_set(request, "key", *buffer, ret);
			
			ret = chain_next_query(chain, request, buffer);
			break;
		
		case INDEX_ORIGINAL:
			// ret = check_existance(key);
			// if(ret != 0)
			// 	return ret;
			
			// fall through
		case LINEAR_ORIGINAL:
			ret = chain_next_query(chain, request, buffer);
			break;
	};
	
	return ret;
}

static ssize_t locate_count(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t      ret  = -1;
	locate_ud  *data = (locate_ud *)chain->user_data;
	
	if( (ret = chain_next_query(chain, request, buffer)) <= 0)
		return ret;
	
	if(incapsulate(data, buffer) != 0)
		return -EIO;
	
	return ret;
}

static chain_t chain_locate = {
	"locate",
	CHAIN_TYPE_CRWD,
	&locate_init,
	&locate_configure,
	&locate_destroy,
	{{
		.func_create = &locate_create,
		.func_set    = &locate_setgetdelete,
		.func_get    = &locate_setgetdelete,
		.func_delete = &locate_setgetdelete,
		.func_move   = &locate_move,
		.func_count  = &locate_count,
	}}
};
CHAIN_REGISTER(chain_locate)

/* vim: set filetype=c: */
