START_TEST (test_backend_mphf){
	hash_t config[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_backend_mphf_g.dat")       },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(name),        DATA_STRING("backend_mphf_g")                },
                        { HK(class),       DATA_STRING("cache")                         },
                        hash_end
                )},
                hash_null,
                
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_backend_mphf_v.dat")       },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(name),        DATA_STRING("backend_mphf_v")                },
                        { HK(class),       DATA_STRING("cache")                         },
                        hash_end
                )},
                hash_null,
                
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_backend_mphf_e.dat")       },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(name),        DATA_STRING("backend_mphf_e")                },
                        { HK(class),       DATA_STRING("cache-append")                  },
                        hash_end
                )},
                hash_null,
		
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_backend_mphf.dat")         },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("cache")                         },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("allocator")                     },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("incapsulate")                   },
                        { HK(multiply),    DATA_OFFT(30)                                },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("structs")                       },
                        { HK(size),        DATA_STRING("size")                          },
                        { HK(structure),   DATA_HASHT(
                                { HK(keyid), DATA_UINT32T(0)                            },
                                { HK(key),   DATA_STRING("")                            },
                                hash_end
                        )},
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(name),         DATA_STRING("backend_mphf")                 },
                        { HK(class),        DATA_STRING("mphf")                         },
                        { HK(type),         DATA_STRING("chm_imp")                      },
                        { HK(backend_g),    DATA_STRING("backend_mphf_g")               },
                        { HK(backend_e),    DATA_STRING("backend_mphf_e")               },
                        { HK(backend_v),    DATA_STRING("backend_mphf_v")               },
                        { HK(nelements),    DATA_UINT64T(100)                           },
                        { HK(value_bits),   DATA_UINT32T(32)                            },
                        { HK(keyid),        DATA_STRING("keyid")                        },
                        hash_end
                )},
		hash_end
	};
	
	backend_t *b_dat = backend_new(config);
	
	char  *data_array[] = {
		"http://google.ru/",
		"http://yandex.ru/",
		"http://bing.com/",
		"http://rambler.ru/",
		"http://aport.ru/",
		"http://hell.com/"
	};
	char buffer[30];
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		memset(buffer, 0, sizeof(buffer));
		
		request_t r_write[] = {
			{ HK(action),  DATA_UINT32T (ACTION_CRWD_CREATE)                        },
			{ HK(key),     DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			{ HK(buffer),  DATA_RAW (buffer, 30)                                    },
                        { HK(ret),     DATA_PTR_SIZET(&ret)                                     },
                        hash_end
		};
		backend_query(b_dat, r_write);
			fail_unless(ret >= 0, "backend backend_mphf: write array failed");
	}
	
	// check
	char data_read[1024];
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		memset(data_read, 0, 1024);
		
		request_t r_read[] = {
			{ HK(action), DATA_UINT32T(ACTION_CRWD_READ)                            },
			{ HK(key),    DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1)  },
			{ HK(buffer), DATA_PTR_STRING (&data_read, 1024)                        },
                        { HK(ret),    DATA_PTR_SIZET(&ret)                                      },
			hash_end
		};
		backend_query(b_dat, r_read);
			fail_unless(ret >= 0,                              "backend backend_mphf: read array failed");
                //        fail_unless(strcmp(data_read, data_array[i]) == 0, "backend backend_mphf: read array data failed");
                //printf("mphf_read: %s %s\n", data_read, data_array[i]);
	}
	
	backend_destroy(b_dat);
}
END_TEST
REGISTER_TEST(core, test_backend_mphf)

/* benchmarks {{{
typedef struct bench_t {
	struct timeval  tv_start;
	struct timeval  tv_end;
	struct timeval  tv_diff;
	unsigned long   time_ms;
	unsigned long   time_us;
	char            string[255];
} bench_t;

void timeval_subtract (result, x, y)
        struct timeval *result, *x, *y; 
{
    
        if (x->tv_usec < y->tv_usec) {
                int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
                y->tv_usec -= 1000000 * nsec;
                y->tv_sec += nsec;
        }   
        if (x->tv_usec - y->tv_usec > 1000000) {
                int nsec = (x->tv_usec - y->tv_usec) / 1000000;
                y->tv_usec += 1000000 * nsec;
                y->tv_sec -= nsec;
        }   
        result->tv_sec = x->tv_sec - y->tv_sec;
        result->tv_usec = x->tv_usec - y->tv_usec;
}


bench_t * bench_start(void){
        bench_t *bench = calloc(sizeof(bench_t), 1); 
        gettimeofday(&bench->tv_start, NULL);
        return bench; 
}
void bench_free(bench_t *bench){
	free(bench);
}
void bench_query(bench_t *bench){
	gettimeofday(&bench->tv_end, NULL);
    
        timeval_subtract(&bench->tv_diff, &bench->tv_end, &bench->tv_start);
        sprintf(bench->string, "%3u.%.6u", (unsigned int)bench->tv_diff.tv_sec, (unsigned int)bench->tv_diff.tv_usec);
	
	bench->time_ms = bench->tv_diff.tv_usec / 1000  + bench->tv_diff.tv_sec * 1000;
	bench->time_us = bench->tv_diff.tv_usec         + bench->tv_diff.tv_sec * 1000000;
	
	if(bench->time_ms == 0) bench->time_ms = 1;
	if(bench->time_us == 0) bench->time_us = 1;
}

#define NTESTS 10000
START_TEST (test_backend_mphf_speed){
	hash_t config[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_backend_mphs_g.dat")       },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(name),        DATA_STRING("backend_mphs_g")                },
                        { HK(class),       DATA_STRING("cache")                         },
                        hash_end
                )},
                hash_null,
                
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_backend_mphs_v.dat")       },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(name),        DATA_STRING("backend_mphs_v")                },
                        { HK(class),       DATA_STRING("cache")                         },
                        hash_end
                )},
                hash_null,
                
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_backend_mphs_e.dat")       },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(name),        DATA_STRING("backend_mphs_e")                },
                        { HK(class),       DATA_STRING("cache-append")                  },
                        hash_end
                )},
                hash_null,
		
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                          },
                        { HK(filename),    DATA_STRING("data_backend_mphs.dat")         },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("cache")                         },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("allocator")                     },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("incapsulate")                   },
                        { HK(multiply),    DATA_OFFT(30)                                },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("structs")                       },
                        { HK(size),        DATA_STRING("size")                          },
                        { HK(structure),   DATA_HASHT(
                                { HK(keyid), DATA_UINT32T(0)                            },
                                { HK(key),   DATA_STRING("")                            },
                                hash_end
                        )},
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(name),         DATA_STRING("backend_mphs")                 },
                        { HK(class),        DATA_STRING("mphf")                         },
                        { HK(type),         DATA_STRING("chm_imp")                      },
                        { HK(backend_g),    DATA_STRING("backend_mphs_g")               },
                        { HK(backend_e),    DATA_STRING("backend_mphs_e")               },
                        { HK(backend_v),    DATA_STRING("backend_mphs_v")               },
                        { HK(nelements),    DATA_UINT64T(NTESTS)                        },
                        { HK(value_bits),   DATA_UINT32T(32)                            },
                        { HK(keyid),        DATA_STRING("keyid")                        },
                        hash_end
                )},
		hash_end
	};
	
	backend_t *b_dat = backend_new(config);
		fail_unless(b_dat != NULL, "backend backend_mphf backend creating failed");

	// write array to file
	int      i,j;
	ssize_t  failed;
	size_t   ntests = NTESTS;
	char     test[8], test2[8];
	
	request_t r_write[] = {
		{ HK(action),  DATA_UINT32T(ACTION_CRWD_CREATE) },
		{ HK(buffer),  DATA_RAW(test2, sizeof(test2))   },
		
		{ HK(key),     DATA_PTR_STRING(test, 8)         },
		{ HK(ret),     DATA_PTR_SIZET(&ret)             },
                hash_end
	};
	
	memset(test, 'a', sizeof(test));
	test[7] = '\0';
	failed = 0;
	
	bench_t  *be = bench_start();
	for(i=0; i < ntests; i++){
		// inc test
		for(j=0;j<sizeof(test)-1;j++){
			if(++test[j] != 'z') break;
			test[j] = 'a';
		}
		
		if(backend_query(b_dat, r_write) < 0 || ret < 0)
			failed++;
	}
	bench_query(be);
	printf("mphf write: %u iters took %s sec; speed: %7lu iters/s (one: %10lu ns) failed: %d\n",
		(unsigned int)ntests,
		be->string,
		( ntests * 1000 / be->time_ms), 
		( be->time_us * 1000 / ntests ),
		(int)failed
	);
	
	bench_free(be);
	fail_unless(failed == 0, "backend backend_mphf: write array failed");
	
	// read test
	memset(test, 'a', sizeof(test));
	test[7] = '\0';
	failed = 0;
	
	be = bench_start();
	for(i=0; i < ntests; i++){
		// inc test
		for(j=0;j<sizeof(test)-1;j++){
			if(++test[j] != 'z') break;
			test[j] = 'a';
		}
		
		request_t r_read[] = {
			{ HK(action),  DATA_UINT32T(ACTION_CRWD_READ) },
			{ HK(key),     DATA_RAW(test,  sizeof(test))  },
			{ HK(buffer),  DATA_RAW(test2, sizeof(test2)) },
			{ HK(size),    DATA_SIZET(sizeof(test2))      },
			{ HK(ret),     DATA_PTR_SIZET(&ret)           },
                        hash_end
		};
		
		if(backend_query(b_dat, r_read) < 0 || ret < 0 || memcmp(test, test2, sizeof(test)) != 0){
			failed++;
			printf("failed: %.*s %.*s\n", (int)sizeof(test), test, (int)sizeof(test2), test2);
		}
	}
	bench_query(be);
	printf("mphf  read: %u iters took %s sec; speed: %7lu iters/s (one: %10lu ns) failed: %d\n",
		(unsigned int)ntests,
		be->string,
		( ntests * 1000 / be->time_ms), 
		( be->time_us / ntests * 1000 ),
		(int)failed
	);
	
	bench_free(be);
	fail_unless(failed == 0, "backend backend_mphf: read array failed");
	
	backend_destroy(b_dat);
}
END_TEST
REEEEGISTER_TEST(core, test_backend_mphf_speed)
*/
