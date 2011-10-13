#include <libfrozen.h>
#include <string_t.h>
#include <hashkeys_int.h>

static int hash_bsearch_int(const void *m1, const void *m2){ // {{{
	hash_keypair_t *mi1 = (hash_keypair_t *) m1;
	hash_keypair_t *mi2 = (hash_keypair_t *) m2;
	return (mi1->key_val - mi2->key_val);
} // }}}

static ssize_t data_string_t_physlen(data_t *data, fastcall_physicallen *fargs){ // {{{
	fargs->length = strlen((char *)data->ptr) + 1;
	return 0;
} // }}}
static ssize_t data_string_t_loglen(data_t *data, fastcall_logicallen *fargs){ // {{{
	fargs->length = strlen((char *)data->ptr);
	return 0;
} // }}}
static ssize_t data_string_t_convert(data_t *dst, fastcall_convert *fargs){ // {{{
	hash_keypair_t  key, *ret;
	char           *string;

	if(fargs->src == NULL)
		return -EINVAL;
	
	switch( fargs->src->type ){
		case TYPE_STRINGT: dst->ptr = strdup((char *)fargs->src->ptr); return 0;
		case TYPE_HASHKEYT:
			
			key.key_val = *(hash_key_t *)(fargs->src->ptr);
			string      = "(unknown)";

			if(key.key_val != 0){
				if((ret = bsearch(&key, hash_keys,
					hash_keys_nelements, hash_keys_size,
					&hash_bsearch_int)) != NULL
				)
					string = ret->key_str;
			}else{
				string = "(null)";
			}
			dst->ptr = strdup(string);
			return 0;
		
		default: break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_string_t_transfer(data_t *src, fastcall_transfer *fargs){ // {{{
	ssize_t                ret;

	if(fargs->dest == NULL)
		return -EINVAL;
	
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, src->ptr, strlen(src->ptr) };
	if( (ret = data_query(fargs->dest, &r_write)) < -1)
		return ret;
	
	return 0;
} // }}}

data_proto_t string_t_proto = {
	.type          = TYPE_STRINGT,
	.type_str      = "string_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_string_t_physlen,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_string_t_loglen,
		[ACTION_CONVERT]     = (f_data_func)&data_string_t_convert,
		[ACTION_TRANSFER]    = (f_data_func)&data_string_t_transfer,
	}
};
