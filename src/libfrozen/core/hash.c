#define HASH_C
#include <libfrozen.h>

// TODO recursive hashes in to\from_memory

// macros {{{
#define _hash_to_memory_cpy(_ptr,_size) { \
	if(_size > memory_size) \
		return -ENOMEM; \
	memcpy(memory, _ptr, _size); \
	memory      += _size; \
	memory_size -= _size; \
}
#define _hash_from_memory_cpy(_ptr,_size) { \
	if(_size > memory_size) \
		return -ENOMEM; \
	memcpy(_ptr, memory, _size); \
	memory      += _size; \
	memory_size -= _size; \
}
// }}}
/*
static ssize_t hash_to_buffer_one(hash_t *hash, void *p_buffer, void *p_null){ // {{{
	void     *data_ptr;
	
	data_ptr  = hash->data.ptr;
	
	if(data_ptr == NULL)
		return ITER_CONTINUE;
	
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	if(data_query(&hash->data, &r_len) < 0)
		return ITER_BREAK;
	
	buffer_add_tail_raw((buffer_t *)p_buffer, data_ptr, r_len.length);
	return ITER_CONTINUE;
} // }}}
*/

hash_t *           hash_new                     (size_t nelements){ // {{{
	size_t  i;
	hash_t *hash;
	
	if(__MAX(size_t) / sizeof(hash_t) <= nelements)
		return NULL;
	
	if( (hash = malloc(nelements * sizeof(hash_t))) == NULL)
		return NULL;
	
	for(i=0; i<nelements - 1; i++){
		hash_assign_hash_null(&hash[i]);
	}
	hash_assign_hash_end(&hash[nelements-1]);
	
	return hash;
} // }}}
hash_t *           hash_copy                    (hash_t *hash){ // {{{
	unsigned int  k;
	hash_t       *el, *el_new, *new_hash;
	
	if(hash == NULL)
		return NULL;
	
	for(el = hash, k=0; el->key != hash_ptr_end; el++){
		if(el->key == hash_ptr_null)
			continue;
		
		k++;
	}
	if( (new_hash = malloc((k+1)*sizeof(hash_t))) == NULL )
		return NULL;
	
	for(el = hash, el_new = new_hash; el->key != hash_ptr_end; el++){
		if(el->key == hash_ptr_null)
			continue;
		
		el_new->key = el->key;
		
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &el_new->data };
		data_query(&el->data, &r_copy);
		
		el_new++;
	}
	el_new->key      = hash_ptr_end;
	el_new->data.ptr = (el->data.ptr != NULL) ? hash_copy(el->data.ptr) : NULL;
	
	return new_hash;
} // }}}
void               hash_free                    (hash_t *hash){ // {{{
	hash_t       *el;
	
	if(hash == NULL)
		return;
	
	for(el = hash; el->key != hash_ptr_end; el++){
		if(el->key == hash_ptr_null)
			continue;
		
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(&el->data, &r_free);
	}
	hash_free(el->data.ptr);
	
	free(hash);
} // }}}

hash_t *           hash_find                    (hash_t *hash, hash_key_t key){ // {{{
	for(; hash != NULL; hash = (hash_t *)hash->data.ptr){
		goto loop_start;
		do{
			hash++;

		loop_start:
			if(hash->key == key)
				return hash;
		}while(hash->key != hash_ptr_end);
	}
	return NULL;
} // }}}
ssize_t            hash_iter                    (hash_t *hash, hash_iterator func, void *arg1, void *arg2){ // {{{
	hash_t      *value = hash;
	ssize_t      ret;
	
	if(hash == NULL)
		return ITER_BREAK;
	
	while(value->key != hash_ptr_end){
		if( value->key == hash_ptr_null )
			goto next;
		
		ret = func(value, arg1, arg2);
		if(ret != ITER_CONTINUE)
			return ret;
	
	next:	
		value++;
	}
	if(value->data.ptr != NULL)
		return hash_iter((hash_t *)value->data.ptr, func, arg1, arg2);
	
	return ITER_OK;
} // }}}
void               hash_chain                   (hash_t *hash, hash_t *hash_next){ // {{{
	hash_t *hend;
	do{
		hend = hash_find(hash, hash_ptr_end);
	}while( (hash = hend->data.ptr) != NULL );
	hend->data.ptr = hash_next;
} // }}}
void               hash_unchain                 (hash_t *hash, hash_t *hash_unchain){ // {{{
	hash_t *hend;
	do{
		hend = hash_find(hash, hash_ptr_end);
	}while( (hash = hend->data.ptr) != hash_unchain );
	hend->data.ptr = NULL;
} // }}}
size_t             hash_nelements               (hash_t *hash){ // {{{
	hash_t       *el;
	unsigned int  i;
	
	if(hash == NULL)
		return 0;
	
	for(el = hash, i=0; el->key != hash_ptr_end; el++){
		i++;
	}
	return i + 1;
} // }}}

/*
ssize_t            hash_to_buffer               (hash_t  *hash, buffer_t *buffer){ // {{{
	size_t  nelements;
	
	buffer_init(buffer);
	
	nelements = hash_nelements(hash);
	buffer_add_tail_raw(buffer, hash, nelements * sizeof(hash_t));
	
	if(hash_iter(hash, &hash_to_buffer_one, buffer, NULL) == ITER_BREAK)
		goto error;
	
	return 0;
error:
	buffer_free(buffer);
	return -EFAULT;
} // }}}
ssize_t            hash_from_buffer             (hash_t **hash, buffer_t *buffer){ // {{{
	hash_t    *curr;
	void      *memory, *chunk, *ptr;
	size_t     size, data_size;
	off_t      data_off = 0;
	
	// TODO remake in streaming style (read)
	// TODO SEC validate
	
	if(buffer_seek(buffer, 0, &chunk, &memory, &size) < 0)
		goto error;
	
	for(curr = (hash_t *)memory; curr->key != hash_ptr_end; curr++){
		if(size < sizeof(hash_t))
			goto error;
		
		size        -= sizeof(hash_t);
		data_off    += sizeof(hash_t);
	}
	data_off += sizeof(hash_t);
	
	for(curr = (hash_t *)memory; curr->key != hash_ptr_end; curr++){
		if(curr->key == hash_ptr_null)
			continue;
		
		if(buffer_seek(buffer, data_off, &chunk, &ptr, &size) < 0)
			goto error;
		
		data_size = curr->data.len;
		
		if(data_size > size)
			goto error;
		
		curr->data->type = curr->data.ptr;
		curr->data->ptr  = ptr;
		
		data_off += data_size;
	}	
	
	*hash = (hash_t *)memory;
	return 0;
error:
	return -EFAULT;
} // }}}
ssize_t            hash_to_memory               (hash_t  *hash, void *memory, size_t memory_size){ // {{{
	size_t  i, nelements, hash_size;
	
	nelements = hash_nelements(hash);
	hash_size = nelements * sizeof(hash_t);
	
	_hash_to_memory_cpy(hash, hash_size);
	for(i=0; i<nelements; i++, hash++){
		if(hash->data.ptr == NULL || hash->data.data_size == 0)
			continue;
		
		_hash_to_memory_cpy(hash->data.ptr, hash->data.data_size);
	}
	return 0;
} // }}}
ssize_t            hash_reread_from_memory      (hash_t  *hash, void *memory, size_t memory_size){ // {{{
	size_t  i, nelements, hash_size;
	
	nelements = hash_nelements(hash);
	hash_size = nelements * sizeof(hash_t);
	
	memory      += hash_size;
	memory_size -= hash_size;
	for(i=0; i<nelements; i++, hash++){
		if(hash->data.ptr == NULL || hash->data.data_size == 0)
			continue;
		if(hash->key == hash_ptr_null)
			continue;
		
		_hash_from_memory_cpy(hash->data.ptr, hash->data.data_size);
	}
	return 0;
} // }}}
ssize_t            hash_from_memory             (hash_t **hash, void *memory, size_t memory_size){ // {{{
	hash_t    *curr;
	size_t     data_size;
	off_t      data_off = 0;
	
	for(curr = (hash_t *)memory; curr->key != hash_ptr_end; curr++){
		if(memory_size < sizeof(hash_t))
			goto error;
		
		memory_size -= sizeof(hash_t);
		data_off    += sizeof(hash_t);
	}
	data_off += sizeof(hash_t);
	
	for(curr = (hash_t *)memory; curr->key != hash_ptr_end; curr++){
		if(curr->key == hash_ptr_null)
			continue;
		
		data_size = curr->data.data_size;
		
		if(data_size > memory_size)
			goto error;
		
		curr->data.ptr = memory + data_off;
		//data_assign_raw(
		//	&curr->data,
		//	curr->data.type,
		//	memory + data_off,
		//	data_size
		//);
		memory_size -= data_size;
		data_off    += data_size;
	}	
	
	*hash = (hash_t *)memory;
	return 0;
error:
	return -EFAULT;
} // }}}
*/

inline hash_key_t         hash_item_key                (hash_t *hash){ return hash->key; }
inline size_t             hash_item_is_null            (hash_t *hash){ return (hash->key == hash_ptr_null); }
inline data_t *           hash_item_data               (hash_t *hash){ return &(hash->data); }
inline data_t *           hash_data_find               (hash_t *hash, hash_key_t key){
	hash_t *temp;
	return ((temp = hash_find(hash, key)) == NULL) ?
		NULL : hash_item_data(temp);
}

#ifdef DEBUG
void hash_dump(hash_t *hash){ // {{{
	unsigned int  k;
	hash_t       *element  = hash;
	data_t        d_string = DATA_STRING(NULL);

	printf("hash: %p\n", hash);
start:
	while(element->key != hash_ptr_end){
		if(element->key == hash_ptr_null){
			printf(" - hash_null\n");
			goto next_item;
		}
		
		data_t           d_key     = DATA_HASHKEYT(element->key);
		fastcall_convert r_convert = { { 3, ACTION_CONVERT }, &d_key };
		data_query(&d_string, &r_convert);

		printf(" - %s [%s] -> %p", (char *)d_string.ptr, data_string_from_type(element->data.type), element->data.ptr);
		
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(&d_string, &r_free);

		fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
		data_query(&element->data, &r_len);

		for(k = 0; k < r_len.length; k++){
			if((k % 32) == 0)
				printf("\n   0x%.5x: ", k);
			
			printf("%.2hhx ", (unsigned int)(*((char *)element->data.ptr + k)));
		}
		printf("\n");
	
	next_item:
		element++;
	}
	printf("end_hash\n");
	if(element->data.ptr != NULL){
		printf("recursive hash: %p\n", element->data.ptr);
		element = element->data.ptr;
		goto start;
	}
} // }}}
#endif

