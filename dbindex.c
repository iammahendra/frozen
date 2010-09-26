#include <memcachedb.h>
#include <dbmap.h>
#include <dbindex.h>

/* DBIndex IBlocks {{{1 */

static char *dbindex_iblock_get_page(dbindex *index, unsigned short iblock){
	return index->file.data + index->data_offset + (iblock - 1) * index->page_size;
}

/* iblock locks {{{2 */
static void dbindex_iblock_lock_init(pthread_rwlock_t **iblock_locks, unsigned short iblock){
	if(iblock_locks == NULL)
		return;

	pthread_rwlock_t *new_lock = malloc(sizeof(pthread_rwlock_t));
	
	pthread_rwlock_init(new_lock, NULL);

	iblock_locks[iblock] = new_lock;
}

static void dbindex_iblock_lock_free(pthread_rwlock_t **iblock_locks, unsigned short iblock){
	free( iblock_locks[iblock] );
}

static void dbindex_iblock_wrlock(dbindex* index, unsigned short iblock){
	pthread_rwlock_wrlock( index->iblock_locks[iblock] );
}

static void dbindex_iblock_rdlock(dbindex* index, unsigned short iblock){
	pthread_rwlock_rdlock( index->iblock_locks[iblock] );
}

static void dbindex_iblock_unlock(dbindex* index, unsigned short iblock){
	pthread_rwlock_unlock( index->iblock_locks[iblock] );
}
/* }}}2 */
/* iblock start {{{2 */
static void  dbindex_iblock_init_start(dbindex *index, unsigned short iblock, unsigned int value){
	char *free_table;
	char *page_offset = dbindex_iblock_get_page(index, iblock);
	
	free_table = index->file.data + DBINDEX_DATA_OFFSET + 0x20000;
	free_table[(iblock - 1) / 8] &= ~( 1 << (7 - (iblock - 1) % 8) ); // set not full
	
	*((unsigned int *)page_offset) = value;
}

static unsigned int dbindex_iblock_get_start(dbindex *index, unsigned short iblock, char **ppage_data_start, char **ppage_data_end){
	char         *free_table;
	bool          is_full;
	
	unsigned int  free_size = 0;
	char         *page_data_start = dbindex_iblock_get_page(index, iblock); 
	char         *page_data_end   = page_data_start + index->page_size;
	
	free_table = index->file.data + DBINDEX_DATA_OFFSET + 0x20000;
	
	is_full = (free_table[(iblock - 1) / 8] >> (7 - ((iblock - 1) % 8))) & 0x1;

	if(!is_full){
		free_size = *((unsigned int *)page_data_start);
		page_data_start += free_size;
	}
	*ppage_data_start = page_data_start;
	*ppage_data_end   = page_data_end;
	
	return free_size;
}

static void  dbindex_iblock_set_start(dbindex *index, unsigned short iblock, unsigned int delta){
	unsigned int  page_len;
	char         *page_offset;
	char         *page_data_start;
	char         *page_data_end;
	char         *free_table;

	page_len    = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
	page_offset = (page_data_start - page_len);
	
	free_table  = index->file.data + DBINDEX_DATA_OFFSET + 0x20000;
	
	page_data_start -= delta;

	if(page_data_start == page_offset){
		free_table[(iblock - 1) / 8] |=  ( 1 << (7 - (iblock - 1) % 8) ); // set full
	}else{
		free_table[(iblock - 1) / 8] &= ~( 1 << (7 - (iblock - 1) % 8) ); // set not full
		*(unsigned int *)page_offset = page_data_start - page_offset;
	}
}
/* }}}2 */


/* dbindex_iblock_search
 *
 * returns:
 *     0 - item pointed by *new_key found at pitem_offset address
 *     1 - item not found, pitem_offset points to address there this item can be, page is full
 *     2 - item not found, pitem_offset points to address there this item can be, page is not full
*/

static int   dbindex_iblock_search(dbindex *index, unsigned short iblock, char **pitem_offset, char *new_key){
	unsigned int  ret;
	char         *item_offset;
	char         *page_data_start;
	char         *page_data_end;
	
	ret = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);

	/* binary search */
	char          *range_start = page_data_start;
	char          *range_end   = page_data_end;
	unsigned long  range_elements;
	int            cret;
	

	if(range_start == range_end){
		*pitem_offset = range_end - index->item_size;
		return 2;
	}
	
	while(1){
		range_elements = (range_end - range_start) / index->item_size;
		item_offset    = range_start + (range_elements / 2) * index->item_size;
		
/*		printf("loop %llx %llx %llx-%llx %x %x\n",
			range_elements,
			item_offset - index->file.data,
			range_start - index->file.data,
			range_end   - index->file.data,
			*(unsigned int *)item_offset,
			*(unsigned int *)new_key
		);*/
	
		cret = memcmp(item_offset, new_key, index->key_len);
		if(cret == 0){
			*pitem_offset = item_offset;
			return 0;
		}else if(cret < 0){
			range_end   = item_offset;
		}else{ // if(ret > 0){
			range_start = item_offset;
		}
		
		if(range_elements == 1){
			*pitem_offset = range_end - index->item_size;
			
			return (ret == 0) ? 1 : 2;
		}
	}
}

static unsigned short dbindex_iblock_new(dbindex *index){
	unsigned short  iblock;
	char           *iblock_offset;
	unsigned long   ret;
	
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
		case DBINDEX_TYPE_INDEX:

			ret = dbmap_expand(&index->file, index->page_size);
			if(ret == -1){
				return 0; //error
			}

			iblock = ++index->iblock_last;
			
			dbindex_iblock_init_start (index, iblock, index->page_size);
			dbindex_iblock_lock_init  (index->iblock_locks, iblock);
			break;
	};

	return iblock;
}

static int dbindex_iblock_get_ourelements(dbindex *index, unsigned short iblock, char *our_element, char **our_elements_start, char **our_elements_end){
	int    no_our_elements = 0;
	char  *search_element     = zalloc(index->item_size);
				
	search_element[0] = our_element[0];
	search_element[1] = our_element[1];
	if(dbindex_iblock_search(index, iblock, our_elements_end, search_element) != 0){
		if(memcmp(search_element, *our_elements_end, 2) != 0){
			no_our_elements = 1;
		}
	}
			
	search_element[2] = (char)0xFF;
	search_element[3] = (char)0xFF;
	if(dbindex_iblock_search(index, iblock, our_elements_start, search_element) != 0){
		*our_elements_start += index->item_size;
		if(memcmp(search_element, *our_elements_start, 2) != 0){
			no_our_elements = 1;
		}
	}
			
	free(search_element);
	return no_our_elements;
}

static void dbindex_iblock_move_elements(dbindex *index, unsigned short iblock_old, unsigned short iblock_new, char *elements_start, char *elements_end){
	unsigned long  iblock_old_free;
	char          *iblock_old_data_start;
	char          *iblock_old_data_end;
	char          *iblock_new_data_start;
	char          *iblock_new_data_end;
			
	iblock_old_free = dbindex_iblock_get_start(index, iblock_old, &iblock_old_data_start, &iblock_old_data_end);
	                  dbindex_iblock_get_start(index, iblock_new, &iblock_new_data_start, &iblock_new_data_end);
	
	unsigned long  items_len = elements_end - elements_start + index->item_size;
	char          *items_pos = iblock_new_data_end - items_len;
		
	if(items_len > 0){
		memcpy(items_pos, elements_start, items_len);
	
		dbindex_iblock_set_start(index, iblock_new, items_len);
		
		memmove(iblock_old_data_start + items_len, iblock_old_data_start, elements_start - iblock_old_data_start);
		memset (iblock_old_data_start, 0, items_len); // NOT can remove

		dbindex_iblock_init_start(index, iblock_old, iblock_old_free + items_len);
	}
}

static unsigned int dbindex_iblock_insert(dbindex *index, unsigned short iblock, unsigned int search_res, char *item_offset, char *new_key){
	char          *page_data_start;
	char          *page_data_end;
	
	if(search_res == 0){
		// found existing item
		
		memcpy(item_offset, new_key, index->item_size);
	}else if(search_res == 2){
		// insert new
			
		dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
		dbindex_iblock_set_start(index, iblock, index->item_size);
				
		if(item_offset >= page_data_start)
			memcpy(
				page_data_start - index->item_size,
				page_data_start,
				item_offset - page_data_start + index->item_size
			);
		
		memcpy(item_offset, new_key, index->item_size);	
	}
	
	return search_res;
}

static char* dbindex_iblock_pop(dbindex *index, unsigned short iblock){
	char  *page_data_start;
	char  *page_data_end;
	
	dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
	
	if(page_data_start == page_data_end)
		return NULL;
	
	char *new_key = zalloc(index->item_size);
	memcpy(new_key, page_data_start, index->item_size);
	
	dbindex_iblock_set_start(index, iblock, -index->item_size);
	
	return new_key;
}

/* }}}1 */
/* DBIndex Virtual mapping {{{1 */

static unsigned short dbindex_vm_virt2real(dbindex *index, unsigned short iblock_virt){
	return ((unsigned short *)(index->file.data + DBINDEX_DATA_OFFSET))[iblock_virt - 1];
}

static unsigned int dbindex_vm_search(dbindex *index, unsigned short *piblock_virt, unsigned short *piblock_real, char **pitem_offset, char *new_key){
	int             cret;
	unsigned int    page_len;
	unsigned long   range_elements;
	unsigned int    ret;
	unsigned short  iblock_virt;
	unsigned short  iblock_real;
	unsigned short  iblock_first_real;
	char           *item_offset_virt;
	char           *item_offset_real;
	char           *file_start;
	char           *file_end;
	char           *range_start;
	char           *range_end;
	char           *page_data_start;
	char           *page_data_end;
	
	int (*cmpfunc)(dbindex *, void *, void *) = index->cmpfunc;
	
	file_start        = index->file.data + index->data_offset;
	file_end          = file_start + index->iblock_last * index->page_size;
	
	iblock_first_real = dbindex_vm_virt2real(index, 1);
	page_len          = dbindex_iblock_get_start(index, iblock_first_real, &page_data_start, &page_data_end);
	
	range_start       = file_start + page_len;
	range_end         = file_end;
	
	if(range_start == range_end){
		printf("empty_range\n");
		*piblock_virt = 1;
		*piblock_real = 1;
		*pitem_offset = range_start - index->item_size;
		return 2;
	}

	while(1){
		range_elements   = (range_end - range_start) / index->item_size;
		item_offset_virt = range_start + (range_elements / 2) * index->item_size;
		
		iblock_virt = ((item_offset_virt - file_start) / index->page_size) + 1;
		iblock_real = dbindex_vm_virt2real(index, iblock_virt);
		
		*piblock_virt = iblock_virt;
		*piblock_real = iblock_real;
		
		item_offset_real = item_offset_virt + (iblock_real - iblock_virt) * index->page_size;
		
		ret = dbindex_iblock_get_start(index, iblock_real, &page_data_start, &page_data_end);
		
		/*printf("loop1: virt %d <=> real %d | elements: %llx offset: %llx range: %llx-%llx page: %llx-%llx\n",
			iblock_virt,
			iblock_real,
			range_elements,
			item_offset_real - index->file.data,
			range_start - index->file.data,
			range_end   - index->file.data,
			page_data_start - index->file.data,
			page_data_end - index->file.data
		);*/
			
		if(item_offset_real < page_data_start)
			item_offset_real = page_data_start; // real elements_count less than virtual
	
		cret = cmpfunc(index, item_offset_real, new_key);
		if(cret == 0){
			// element found, but index is not unique by keys, so we lie that element not found
			*pitem_offset = item_offset_real;
			return 2;
		}else if(cret == -2){
			printf("fatal\n");
			return -2;
		}else if(cret < 0){
			range_end   = item_offset_virt;
		}else{ // if(ret > 0){
			range_start = item_offset_virt;
		}
		
		if(range_elements <= 1){
			*pitem_offset = range_end + (iblock_real - iblock_virt) * index->page_size - index->item_size;

			return (ret == 0) ? 1 : 2;
		}
	}
}

/* }}}1 */

static unsigned long dbindex_expand(dbindex *index, unsigned long need_len){
	if(index->file.data_len < need_len){
		return dbmap_expand(&index->file, need_len - index->file.data_len);
	}
	return 1;
}

/* DBIndex initialisation {{{1 */
unsigned int dbindex_create(char *path, unsigned int type, unsigned int key_len, unsigned int value_len){
	unsigned long  res;
	unsigned int   ret = 1;
	unsigned short iblock;
	dbindex        new_index;
	
	if(key_len <= 2)
		return ret;
	
	memset(&new_index, 0, sizeof(dbindex));

	unlink(path); // SEC dangerous
	
	dbmap_map(path, &new_index.file);
	
	res = dbindex_expand(&new_index, DBINDEX_DATA_OFFSET);
	if(res != -1){
		new_index.type      = type;
		new_index.item_size = key_len + value_len;
		new_index.key_len   = key_len;
		new_index.value_len = value_len;
		new_index.page_size = (1 << (key_len - 2) * 8) * (key_len + value_len);
		
		int switch_ok = 0;
		switch(type){
			case DBINDEX_TYPE_PRIMARY:
			case DBINDEX_TYPE_INDEX:   // same layout as primary
				new_index.data_offset = DBINDEX_DATA_OFFSET + 0x20000 + 0x10000 / 8;
				new_index.iblock_last = 0;
				
				res = dbindex_expand(&new_index, DBINDEX_DATA_OFFSET + 0x20000 + 0x10000 / 8);
				if(res == -1)
					break;
				
				iblock = dbindex_iblock_new(&new_index);
				if(iblock == 0)
					break;
				
				// TODO check, can i use it for PRIMARY?
				if(type == DBINDEX_TYPE_INDEX){
					*(unsigned short *)(new_index.file.data + DBINDEX_DATA_OFFSET) = iblock;
				}
				
				switch_ok = 1;
				break;
		};
		if(switch_ok == 1){
			memcpy(new_index.file.data, &new_index, DBINDEX_SETTINGS_LEN);
			
			ret = 0;
		}
	}
	dbmap_unmap(&new_index.file);
	
	return ret;
}

void dbindex_load(char *path, dbindex *index){
	int      i, j;
	void    *ipage_ptr;
	short   *idata_ptr;
	short   *fdata_ptr;
	short    ipage_block;
	bool     ipage_not_null;
	unsigned short k;
	
	dbmap_map(path, &index->file);
	//pthread_rwlock_init(&index->lock, NULL);
	
	// TODO sanity check
	memcpy(index, index->file.data, DBINDEX_SETTINGS_LEN);
	
	fdata_ptr = (short *)(index->file.data + DBINDEX_DATA_OFFSET);
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			// TODO sanity check
			index->ipage_l1 = (void **)malloc(256 * sizeof(void *));
			if(index->ipage_l1 == NULL){
				fprintf(stderr, "dbindex_load: insufficient memory\n");
				return;
			}
			for(i=0; i<=255; i++){
				ipage_ptr = (void *)malloc(256 * sizeof(short));
				if(ipage_ptr == NULL){
					fprintf(stderr, "dbindex_load: insufficient memory\n");
					return;
				}
				
				idata_ptr = ipage_ptr;
				ipage_not_null = false;
				for(j=0; j<=255; j++){
					ipage_block = *fdata_ptr++;
					if(ipage_block != 0)
						ipage_not_null = true;
					*idata_ptr++ = ipage_block;
				}
				if(ipage_not_null == false){
					free(ipage_ptr);
					ipage_ptr = NULL;
				}
				index->ipage_l1[i] = ipage_ptr;
			}
			// fall throuth
			
		case DBINDEX_TYPE_INDEX:

			index->iblock_locks = zalloc(0xFFFF * sizeof(pthread_rwlock_t *));
			for(k=1; k <= index->iblock_last; k++){
				dbindex_iblock_lock_init(index->iblock_locks, k);
			}
			
			break;
		default:
			fprintf(stderr, "%s: wrong index type\n", path); // TODO add DEBUG define
			return;
	};
	index->loaded = 1;
}

void dbindex_save(dbindex *index){
	int      i, j;
	short   *idata_ptr;
	short   *fdata_ptr;
	void    *ipage_ptr;
	
	//pthread_rwlock_rdlock(&index->lock);
	dbmap_lock(&index->file);
		
		memcpy(index->file.data, index, DBINDEX_SETTINGS_LEN);
		
		fdata_ptr = (short *)(index->file.data + DBINDEX_DATA_OFFSET);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
				for(i=0; i<=255; i++){
					idata_ptr = index->ipage_l1[i];
					if(idata_ptr == NULL){
						fdata_ptr += 256 * sizeof(short); // TODO 256???
					}else{
						for(j=0; j<=255; j++)
							*fdata_ptr++ = *idata_ptr++;
					}
				}
				break;
			case DBINDEX_TYPE_INDEX:
				// nothing to do
				break;
		};
	dbmap_unlock(&index->file);
	//pthread_rwlock_unlock(&index->lock);
}

void dbindex_unload(dbindex *index){
	int    i;
	void  *ipage_ptr;

	dbindex_save(index);

	//pthread_rwlock_wrlock(&index->lock);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
				if(index->ipage_l1 != NULL){
					for(i=0; i<=255; i++){
						ipage_ptr = index->ipage_l1[i];
						if(ipage_ptr != NULL)
							free(ipage_ptr);
					}
					free(index->ipage_l1);
				}
				
				for(i=1; i <= index->iblock_last; i++){
					dbindex_iblock_lock_free(index->iblock_locks, i);
				}
				free(index->iblock_locks);
				break;
			case DBINDEX_TYPE_INDEX:
				// nothing to do
				break;
		};
		dbmap_unmap(&index->file);
	//pthread_rwlock_unlock(&index->lock);
	index->loaded = 0;
}

void dbindex_set_cmpfunc(dbindex *index, int (*cmpfunc)(dbindex *, void *, void *)){
	index->cmpfunc = cmpfunc;
}

void dbindex_set_userdata(dbindex *index, void *data){
	index->user_data = data;
}

/* }}}1 */
/* DBIndex IPages {{{1 */
static short dbindex_ipage_get_iblock(dbindex* index, void *item_key){
	unsigned char    index_c1;
	unsigned char    index_c2;
	short           *ipage_l2;
	
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			index_c1 = *((char *)item_key + 0x03);
			index_c2 = *((char *)item_key + 0x02);
					
			if(index->ipage_l1 == NULL)
				return 0;

			ipage_l2 = (short *)index->ipage_l1[index_c1];
			if( ipage_l2 == NULL )
				return 0;
			
			return ipage_l2[index_c2];
			
			break;
		case DBINDEX_TYPE_INDEX:
			// bsearch block
			break;
	}
}

static void dbindex_ipage_set_iblock(dbindex* index, void *item_key, unsigned short iblock){
	unsigned char    index_c1;
	unsigned char    index_c2;
	short           *ipage_l2;

	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			index_c1 = *((char *)item_key + 0x03);
			index_c2 = *((char *)item_key + 0x02);
			
			ipage_l2 = (short *)index->ipage_l1[index_c1];
			if( ipage_l2 == NULL ){
				ipage_l2 = (short *)zalloc(256 * sizeof(short));
				if(ipage_l2 == NULL){
					// NOTE holly shit..
					fprintf(stderr, "dbindex_insert: insufficient memory, losing data\n");
					return;
				}
				index->ipage_l1[index_c1] = ipage_l2;
			}
			ipage_l2[index_c2] = iblock;
			break;
		case DBINDEX_TYPE_INDEX:
			// nothing to do
			break;
	};
}

/* }}}1 */

unsigned int dbindex_insert(dbindex *index, void *item_key, void *item_value){
	if(index->loaded == 0)
		return 1;
	
	unsigned short iblock;
	unsigned short iblock_new;	
	unsigned short iblock_curr_virt;
	unsigned short iblock_curr_real;
	unsigned short iblock_new_virt;
	unsigned short iblock_new_real;
	unsigned short iblock_prev_virt;
	unsigned short iblock_prev_real;
	
	unsigned int   ret = 1;
	unsigned int   cret;
	char          *item_offset;
	char          *new_key       = zalloc(index->item_size);
	
	//pthread_rwlock_rdlock(&index->lock);
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			// if item have mpage - we write it to this page
			// if item dont have own page - we write it to last page
						
			/* prepare item */
			revmemcpy(new_key, item_key, index->key_len);
			memcpy(new_key + index->key_len, item_value, index->value_len);	


			iblock = dbindex_ipage_get_iblock(index, item_key);
			if(iblock == 0)
				iblock = index->iblock_last;
			
			dbmap_lock(&index->file);
			dbindex_iblock_wrlock(index, iblock);
				
				cret = dbindex_iblock_search(index, iblock, &item_offset, new_key);
				cret = dbindex_iblock_insert(index, iblock, cret, item_offset, new_key);
				if(cret == 1){
					dbmap_unlock(&index->file);

					iblock_new = dbindex_iblock_new(index);
					
					if(iblock_new == 0){
						// no more blocks
						dbindex_iblock_unlock(index, iblock);
						break;
					}
					
					dbmap_lock(&index->file);
					dbindex_iblock_wrlock(index, iblock_new);
						
						char  *elements_start;
						char  *elements_end;
						
						ret = dbindex_iblock_get_ourelements(index, iblock, new_key, &elements_start, &elements_end);
						if(ret == 0){
							dbindex_iblock_move_elements(index, iblock, iblock_new, elements_start, elements_end);
						}
						cret = dbindex_iblock_search(index, iblock_new, &item_offset, new_key);
						cret = dbindex_iblock_insert(index, iblock_new, cret, item_offset, new_key);
						
					dbindex_iblock_unlock(index, iblock_new);	
					dbindex_iblock_unlock(index, iblock);
					dbmap_unlock(&index->file);
				}else{
					dbindex_iblock_unlock(index, iblock);
					dbmap_unlock(&index->file);
					
					iblock_new = iblock;
				}
			
			dbindex_ipage_set_iblock(index, item_key, iblock_new);
			
			ret = 0;
			break;
		case DBINDEX_TYPE_INDEX:
			cret = dbindex_vm_search(index, &iblock_curr_virt, &iblock_curr_real, &item_offset, item_key);
			cret = dbindex_iblock_insert(index, iblock_curr_real, cret, item_offset, item_key);
			if(cret == 1){ // page full
				char *poped_item;
				
				poped_item = dbindex_iblock_pop(index, iblock_curr_real);
				
				iblock_prev_virt = iblock_curr_virt - 1;
				iblock_prev_real = dbindex_vm_virt2real(index, iblock_prev_virt);
				
				while(1){
					if(iblock_prev_virt == 0){
						iblock_new_real = dbindex_iblock_new(index);
						//
						unsigned short *mapping;
						mapping = &(((unsigned short *)(index->file.data + DBINDEX_DATA_OFFSET))[iblock_curr_virt - 1]);
						
						memmove(
							mapping + 1,
							mapping,
							(0xFFFF - iblock_curr_virt) * sizeof(short)
						);
						*mapping = iblock_new_real;
						//
						iblock_prev_real = iblock_new_real;
					}
					
					unsigned int   page_free_size;
					char          *page_data_start;
					char          *page_data_end;
					
					page_free_size = dbindex_iblock_get_start(index, iblock_prev_real, &page_data_start, &page_data_end);
					if(page_free_size == 0){
						iblock_prev_virt = 0;
						continue;
					}
					dbindex_iblock_insert(index, iblock_prev_real, 2, page_data_end - index->item_size, poped_item);
					break;
				}
				cret = dbindex_vm_search(index, &iblock_curr_real, &iblock_curr_real, &item_offset, item_key);
				cret = dbindex_iblock_insert(index, iblock_curr_real, cret, item_offset, item_key);

				free(poped_item);
				break;
			}
			
			ret = 0;
			break;
	};
	//pthread_rwlock_unlock(&index->lock);
	free(new_key);

	return ret;
}


/* dbindex_query
 *
 * returns: 
 *     0 - found
 *     1 - not found
*/

unsigned int dbindex_query(dbindex *index, void *item_key, void *item_value){
	short            iblock;
	unsigned int     ret = 1;
	unsigned char    index_c1;
	unsigned char    index_c2;
	char            *item_revkey = malloc(index->key_len);
	char            *item_offset;
	short           *ipage_l2;

	//pthread_rwlock_rdlock(&index->lock);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
				iblock = dbindex_ipage_get_iblock(index, item_key);
				if(iblock == 0)
					break;
				
				dbmap_lock(&index->file);
				dbindex_iblock_rdlock(index, iblock);
					
					revmemcpy(item_revkey, item_key, index->key_len);
					if(dbindex_iblock_search(index, iblock, &item_offset, item_revkey) == 0){
						memcpy(item_value, item_offset + index->key_len, index->value_len);
						ret = 0;
					}else{
						printf("bug\n");
					}

				dbindex_iblock_unlock(index, iblock);
				dbmap_unlock(&index->file);
				break;
		}
	//pthread_rwlock_unlock(&index->lock);
	
	free(item_revkey);
	return ret;
}

/* dbindex_delete
 *
 * returns:
 * 	0 - found and deleted
 * 	1 - not found
*/

// TODO move actual deletion to another thread, mark item as deleted

unsigned int dbindex_delete(dbindex *index, void *item_key){
	short            iblock;
	unsigned int     ret = 1;
	unsigned int     free_space;
	char            *item_revkey = malloc(index->key_len);
	char            *page_data_start;
	char            *page_data_end;
	char            *item_offset;
	
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			iblock = dbindex_ipage_get_iblock(index, item_key);
			if(iblock == 0)
				break;
			
			dbmap_lock(&index->file);
			dbindex_iblock_wrlock(index, iblock);

				revmemcpy(item_revkey, item_key, index->key_len);
				if(dbindex_iblock_search(index, iblock, &item_offset, item_revkey) == 0){
					free_space = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
					
					if(item_offset > page_data_start)
						memmove(
							page_data_start + index->item_size,
							page_data_start,
							item_offset - page_data_start
						);
					dbindex_iblock_init_start(index, iblock, free_space + index->item_size);
					ret = 0;
				}

			dbindex_iblock_unlock(index, iblock);
			dbmap_unlock(&index->file);
			break;
	};
	
	free(item_revkey);
	return ret;
}

unsigned int dbindex_iter(dbindex *index, void *(*func)(void *, void *, void *, void *), void *arg1, void *arg2){
	unsigned short iblock;
	unsigned int   ret;
	unsigned int   stop = 0;
	char          *item_offset;
	char          *page_data_start;
	char          *page_data_end;
	char          *new_key = malloc(index->item_size);

	dbmap_lock(&index->file);
	
	iblock = 1;
	while(iblock <= index->iblock_last && stop == 0){
		dbindex_iblock_rdlock(index, iblock);
			
			ret = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
			
			item_offset = page_data_start;
			while(item_offset < page_data_end){
				revmemcpy(new_key, item_offset, index->key_len);

				if(func(new_key, (item_offset + index->key_len), arg1, arg2) == NULL){
					stop = 1;
					break;
				}

				item_offset += index->item_size;
			}
		dbindex_iblock_unlock(index, iblock);
		iblock++;
	}
	
	dbmap_unlock(&index->file);
	
	free(new_key);
}
