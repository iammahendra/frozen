#ifndef HASH_H
#define HASH_H

struct hash_t {
	char *          key;
	data_t          data;
};

typedef int  (*hash_iterator)(hash_t *, void *, void *);

#define hash_ptr_null  (void *)-1
#define hash_ptr_end   (void *)-2 

#define hash_null       {hash_ptr_null, {0, hash_ptr_null, 0}}
#define hash_next(hash) {hash_ptr_end,  {0, (void *)hash,  0}}
#define hash_end        hash_next(NULL)

#define HVALUE(_hash_t,_type)  *(_type *)_hash_t->data.data_ptr

#define hash_assign_hash_t(_dst, _src) {        \
	memcpy((_dst), (_src), sizeof(hash_t)); \
}
#define hash_assign_hash_null(_dst) { \
	(_dst)->key = hash_ptr_null;  \
}
#define hash_assign_hash_end(_dst) {  \
	(_dst)->key = hash_ptr_end;   \
	(_dst)->data.data_ptr = NULL; \
}

API int                hash_get                     (hash_t *hash, char *key, data_type *type, void **value, size_t *value_size);
API int                hash_get_typed               (hash_t *hash, data_type type, char *key, void **value, size_t *value_size);
API hash_t *           hash_set                     (hash_t *hash, char *key, data_type type, void *value, size_t value_size);
API void               hash_delete                  (hash_t *hash, char *key);

API void               hash_chain                   (hash_t *hash, hash_t *hash_next);
API int                hash_iter                    (hash_t *hash, hash_iterator func, void *arg1, void *arg2);
API hash_t *           hash_find                    (hash_t *hash, char *key);
API hash_t *           hash_find_typed              (hash_t *hash, data_type type, char *key);

API data_t *           hash_get_data                (hash_t *hash, char *key);
API data_t *           hash_get_typed_data          (hash_t *hash, data_type type, char *key);
API data_ctx_t *       hash_get_data_ctx            (hash_t *hash, char *key);

API data_type          hash_get_data_type           (hash_t *hash); // TODO deprecate
API void *             hash_get_value_ptr           (hash_t *hash);
API size_t             hash_get_value_size          (hash_t *hash);

API hash_t *           hash_copy                    (hash_t *hash);
API void               hash_free                    (hash_t *hash);
API size_t             hash_get_nelements           (hash_t *hash);

#ifdef DEBUG
API void               hash_dump                    (hash_t *hash);
#endif

API int                hash_to_buffer               (hash_t  *hash, buffer_t *buffer);
API int                hash_from_buffer             (hash_t **hash, buffer_t *buffer);

#endif // HASH_H
