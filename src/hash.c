#include <libfrozen.h>

hash_t *         hash_find_value              (hash_t *hash, char *key){ // {{{
	hash_t      *value = hash;
	
	while(value->key != hash_ptr_end){
		if(
			value->key == key ||
			(
				key != hash_ptr_null && value->key != hash_ptr_null &&
				key != hash_ptr_end  &&
				strcmp(value->key, key) == 0
			)
		)
			return value;
		value++;
	}
	return NULL;
} // }}}
hash_t *         hash_find_typed_value        (hash_t *hash, data_type type, char *key){ // {{{
	hash_t      *hvalue = hash_find_value(hash, key);
	return (hvalue != NULL) ? ((hvalue->type != type) ? NULL : hvalue) : NULL;
} // }}}
hash_t *         hash_add_value               (hash_t *hash, data_type type, char *key, void *value){ // {{{
	hash_t      *hvalue;
	
	if( (hvalue = hash_find_value(hash, hash_ptr_null)) == NULL)
		return NULL;
	
	hvalue->type  = type;
	hvalue->value = value;
	hvalue->key   = key;
	return hvalue;
} // }}}
void               hash_del_value               (hash_t *hash, char *key){ // {{{
	hash_t       *hvalue;
	
	if( (hvalue = hash_find_value(hash, key)) == NULL)
		return;
	
	hvalue->key = hash_ptr_null;
} // }}}

/* private {{{
static int         hash_new_key                 (hash_t *hash, hash_key_t *hash_key){
	unsigned int  new_id;
	unsigned int  new_nelements;
	hash_el_t    *new_elements;
	
	new_id        = hash->nelements;
	new_nelements = hash->nelements + 1;
	if(new_nelements > HASH_MAX_ELEMENTS)
		return -ENOMEM;
	
	new_elements = realloc(hash->elements, new_nelements * sizeof(hash_el_t));
	if(new_elements == NULL)
		return -ENOMEM;
	
	hash->elements  = new_elements;
	hash->nelements = new_nelements;
	
	hash_key->hash    = hash;
	hash_key->element = &(hash->elements[new_id]);
	
	return 0;
}

static unsigned int hash_add_data_chunk (hash_t *hash, void *data, size_t size){ // {{{
	unsigned int  new_alloc_size;
	unsigned int  new_ptr;
	data_t       *new_data;
	
	new_alloc_size = (hash->data_end - hash->data) + size;
	if( new_alloc_size >= hash->alloc_size){
		new_data = realloc(hash->data, new_alloc_size + HASH_DATABLOCK_SIZE);
		if(new_data == NULL)
			return -1;
		
		hash->data_end = (new_data + (hash->data_end - hash->data));
		hash->data     =  new_data;
	}
	if(data != NULL)
		memcpy(hash->data_end, data, size);
	
	new_ptr          = (hash->data_end - hash->data); 
	hash->data_end  += size;
	hash->size      += size;
	
	return new_ptr;
} // }}}

static data_t *    hash_off_to_ptr              (hash_t *hash, unsigned int offset, size_t *buffer_size){ // {{{
	unsigned int  data_size;
	
	data_size = (hash->data_end - hash->data);
	
	if(offset >= data_size)
		return NULL;
	
	if(buffer_size != NULL)
		*buffer_size = (size_t)(data_size - offset);
	
	return (data_t *)(hash->data + offset);
} // }}}

static int         hash_search_key              (hash_t *hash, char *key, int lookup_layers, hash_key_t *hash_key){ // {{{
	unsigned int  i;
	hash_el_t    *element;
	data_t       *found_key;
	size_t        found_buf_size;
	
	element = hash->elements;
	for(i=0; i < hash->nelements; i++, element++){
		found_key = hash_off_to_ptr(hash, element->key, &found_buf_size);
		if(found_key == NULL)
			continue;
		
		//data_cmp(found_key, key)
		if(strncmp(found_key, key, found_buf_size) == 0){
			hash_key->hash    = hash;
			hash_key->element = element;
			
			return 0;
		}
	}
	if(hash->next_layer == NULL)
		return -1;
	if(lookup_layers == 0)
		return -1;
	
	return hash_search_key(hash->next_layer, key, lookup_layers, hash_key);
} // }}}

static int         hash_key_get_data            (hash_key_t *hash_key, data_type *type, data_t **data, size_t *size){ // {{{
	data_t       *el_data;
	size_t        el_buf_size;
	
	if(data != NULL || size != NULL){
		if( (el_data = hash_off_to_ptr(hash_key->hash, hash_key->element->value, &el_buf_size)) == NULL)
			return -1;
		
		if(data != NULL)
			*data = el_data;
	}
	
	if(size != NULL)
		if( (*size = data_bare_len(hash_key->element->type, el_data, el_buf_size)) == 0)
			return -1;
	
	if(type != NULL)
		*type = hash_key->element->type;
	
	return 0;	
} // }}}

hash_t *           hash_new                     (void){ // {{{
	hash_t     *hash;
	data_t     *data;
	
	hash     = malloc( sizeof(hash_t) );
	if(hash == NULL)
		goto cleanup1;
	
	data     = malloc( HASH_DATABLOCK_SIZE );
	if(data == NULL)
		goto cleanup2;
	
	hash->is_local   = 1;
	hash->nelements  = 0;
	hash->size       = HASH_T_NETWORK_SIZE;
	hash->alloc_size = HASH_DATABLOCK_SIZE;
	hash->elements   = NULL;
	hash->data       = data;
	hash->data_end   = data;
	hash->next_layer = NULL;

	return hash;
cleanup2:
	free(hash);
cleanup1:
	return NULL;
} // }}}
void               hash_empty                   (hash_t *hash){ // {{{
	if(hash->is_local != 1)
		return;
	
	if(hash->elements != NULL)
		free(hash->elements);
	
	hash->elements  = NULL;
	hash->nelements = 0;
	hash->size      = HASH_T_NETWORK_SIZE;
	hash->data_end  = hash->data;
} // }}}
void               hash_free                    (hash_t *hash){ // {{{
	if(hash->is_local == 1){
		if(hash->elements != NULL)
			free(hash->elements);
		free(hash->data);
	}
	free(hash);
} // }}}
void               hash_assign_layer            (hash_t *hash, hash_t *hash_layer){ // {{{
	if(hash == NULL)
		return;
	
	hash->next_layer = hash_layer;
} // }}}

int                hash_to_buffer               (hash_t  *hash, buffer_t *buffer);
int                hash_from_buffer             (hash_t **hash, buffer_t *buffer);

int                hash_set                     (hash_t *hash, char *key, data_type  type, data_t  *value){ // {{{
	unsigned int  key_chunk;
	unsigned int  data_chunk;
	hash_key_t    found_key;
	data_t       *found_data;
	size_t        found_size;
	size_t        value_size;
	int           ret;
	
	if(hash == NULL || key == NULL || value == NULL)
		return -1;
	
	value_size = data_bare_len(type, value, (size_t)-1); // TODO SEC bad bad bad bad...
	
	if(hash_search_key(hash, key, 0, &found_key) != 0)
		goto bad;
	
	if(found_key.element->type != type)
		goto bad;
	
	ret = hash_key_get_data(&found_key, NULL, &found_data, &found_size);
	if(ret != 0)
		goto bad;
	
	if(found_size >= value_size){
		memcpy(found_data, value, value_size);
		
		return 0;
	}
	
bad:	
	if(hash->is_local != 1)
		return -1;
	
	if(hash_new_key(hash, &found_key) != 0)
		return -1;
	
	key_chunk  = hash_add_data_chunk(hash, key,   strlen(key) + 1);
	if(key_chunk  == (unsigned int)-1)
		return -1;
	
	data_chunk = hash_add_data_chunk(hash, value, value_size);
	if(data_chunk == (unsigned int)-1)
		return -1;
	
	found_key.element->type  = type;
	found_key.element->key   = key_chunk;
	found_key.element->value = data_chunk;
	
	return 0;
} // }}}

int                hash_set_custom              (hash_t *hash, char *key, unsigned int chunk_size, data_t **data){ // {{{
	unsigned int  key_chunk;
	unsigned int  data_chunk;
	hash_key_t    found_key;
	
	if(hash == NULL || key == NULL || chunk_size == 0 || hash->is_local != 1)
		return -1;
	
	if(hash_search_key(hash, key, 0, &found_key) != 0){
		if(hash_new_key(hash, &found_key) != 0)
			return -1;
	}
	
	key_chunk  = hash_add_data_chunk(hash, key,   strlen(key) + 1);
	if(key_chunk  == (unsigned int)-1)
		return -1;
	
	data_chunk = hash_add_data_chunk(hash, NULL,  chunk_size);
	if(data_chunk == (unsigned int)-1)
		return -1;
	
	found_key.element->type  = TYPE_VOID;
	found_key.element->key   = key_chunk;
	found_key.element->value = data_chunk;
	
	if(data != NULL)
		*data = hash_off_to_ptr(hash, data_chunk, NULL);
	
	return 0;
} // }}}

int                hash_get_any                 (hash_t *hash, char *key, data_type *type, data_t **value, size_t *value_size){ // {{{
	hash_key_t  found_key;
	
	if(hash_search_key(hash, key, 1, &found_key) != 0)
		return -1;
	
	if(hash_key_get_data(&found_key, type, value, value_size) != 0)
		return -1;
	
	return 0;
} // }}}

int                hash_get                     (hash_t *hash, char *key, data_type  type, data_t **value, size_t *value_size){ // {{{
	data_type     found_type;
	int           ret;
	
	ret = hash_get_any(hash, key, &found_type, value, value_size);
	if(ret != 0 || found_type != type)
		return -1;
	
	return 0;
} // }}}

int                hash_get_copy                (hash_t *hash, char *key, data_type  type, data_t  *buf, size_t buf_size){ // {{{
	data_type     found_type;
	data_t       *found_data;
	size_t        found_size;
	int           ret;
	
	ret = hash_get_any(hash, key, &found_type, &found_data, &found_size);
	if(ret != 0 || found_type != type)
		return -1;
	
	if(found_size > buf_size)
		return -ENOMEM;
	
	memcpy(buf, found_data, found_size);
	return 0;
} // }}}

#ifdef DEBUG
void hash_dump(hash_t *hash){
	unsigned int  i,k;
	hash_el_t    *element;
	data_t       *found_key;
	data_t       *found_data;
	size_t        found_buf_size;
	size_t        found_data_size;
	
	element = hash->elements;
	printf("hash: %x\n", (unsigned int)hash);
	for(i=0; i < hash->nelements; i++, element++){
		found_key = hash_off_to_ptr(hash, element->key, &found_buf_size);
		if(found_key == NULL)
			continue;
		
		found_data = hash_off_to_ptr(hash, element->value, &found_data_size);
		if(found_data == NULL)
			continue;
		
		found_data_size = data_bare_len(element->type, found_data, found_data_size);
		
		printf(" - el: %x %s", (unsigned int)element->type, (char *)found_key);
		for(k=0; k<found_data_size; k++){
			if((k % 8) == 0)
				printf("\n   0x%.4x: ", k);
			
			printf("%.2x ", (unsigned int)(*((char *)found_data + k)));
		}
		printf("\n");
	}
	printf("end_hash\n");
}

#endif
*/
/*
// never use hash->size as buffer_size argument, it can lead to security problems
int                hash_audit                   (hash_t *hash, size_t buffer_size){
	unsigned int   looked_size;
	unsigned int   hash_size = hash->size;
	hash_el_t     *elements;
	
	// validate size
	if(hash_size > buffer_size)
		return -EINVAL;
	
	elements = (hash_el_t *)(hash + 1);
	
	// search for 0xFF slot
	looked_size = sizeof(hash_t);
	while(looked_size + sizeof(hash_el_t) <= hash_size){
		if(is_set(elements, 0xFF, sizeof(hash_el_t)) != 0){
			return 0;
		}
		
		elements++;
		looked_size += sizeof(hash_el_t);
	}
	return -1;
}

int                hash_is_valid_buf            (hash_t *hash, data_t *data, unsigned int size){
	if(size >= hash->size)
		return 0;
	if((void *)data <= (void *)hash || (void *)data >= (void *)hash + hash->size)
		return 0;
	if((void *)data + size > (void *)hash + hash->size)
		return 0;
	return 1;
}

int                hash_get                     (hash_t *hash, char *key, data_type *type, data_t **data){
	hash_el_t    *curr;
	hash_el_t    *elements;
	char         *key_hash;
	char         *key_args;
	char         *hash_data_ptr;
	char         *hash_data_end;
	unsigned int  hash_data_len;
	
	if(hash == NULL || key == NULL || type == NULL || data == NULL)
		return -EINVAL;
	
	elements = (hash_el_t *)(hash + 1);
	
	for(curr = elements; is_set(curr, 0xFF, sizeof(hash_el_t)) == 0; curr++);
	curr++;
	hash_data_ptr = (char *)curr;
	hash_data_end = (char *)hash + hash->size;
	hash_data_len = hash_data_end - hash_data_ptr;
	
	for(curr = elements; is_set(curr, 0xFF, sizeof(hash_el_t)) == 0; curr++){
		if(curr->key >= hash_data_len)
			goto next_element;                                               // broken element's key
		
		key_hash = hash_data_ptr + curr->key;
		key_args = key;
		while(key_hash < hash_data_end){
			if(*key_hash != *key_args)
				break;
			
			if(*key_hash == 0 && *key_args == 0){
				if(!data_type_is_valid( (data_type)curr->type ))
					goto next_element;                               // broken element's type
				
				*type = (data_type)curr->type;
				
				if(curr->value >= hash_data_len)
					goto next_element;                       // broken element's value
					
				*data = (data_t *)(hash_data_ptr + curr->value);
				return 0;
			}
			
			if(*key_hash == 0 || *key_args == 0)
				break;
			key_hash++;
			key_args++;
		}
	next_element:
		;
	}
	return -1;
}

data_t *           hash_get_typed               (hash_t *hash, char *key, data_type type){
	data_type      fetched_type;
	data_t        *fetched_data;
	
	if(
		hash_get(hash, key, &fetched_type, &fetched_data) == 0 &&
		fetched_type == type
	){
		return fetched_data;
	}
	
	return NULL;
}

int                hash_get_in_buf              (hash_t *hash, char *key, data_type type, data_t *buf){
	data_type      fetched_type;
	data_t        *fetched_data;
	size_t         fetched_len;
	
	if(
		hash_get(hash, key, &fetched_type, &fetched_data) == 0 &&
		fetched_type == type
	){
		fetched_len = data_len(fetched_type, fetched_data);
		
		memcpy(buf, fetched_data, fetched_len); 
		return 0;
	}
	return -1;
}
*/
