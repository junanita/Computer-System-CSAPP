/*
 * Name: Ningna Wang
 * AndrewID: ningnaw
 * 
 * cache.c - A simple cache implementation
 *           using for proxy.c 
 *
 * Hierachy:   Cache -> Cache_Set -> Cache_Line
 * 
 */
#include "cache.h"

/* Using following macro to debug output */
// #define DEBUG

#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/*
 *  initCache - init cache by passing cache pointer
 *  initial status: num_sets = 1, num_lines = 10
 *
 * Pamameters:
 * 		cache: pointer that point to global varialb cache
 */
void initCache(Cache *cache) {
    dbg_printf("--------In initCache function ---------\n");

    /* init cache */
    cache->num_sets = 1;
    cache->num_lines = 10;

    /* init set */
    cache->set = (Cache_Set *)malloc(cache->num_sets * sizeof(Cache_Set));
    if (!cache->set) {
        printf("Init cache sets error");
        exit(-1);
    }

    /* init each set */
    Cache_Set *set;
    for (int idx_set = 0; idx_set < cache->num_sets; idx_set++) {
        set = &cache->set[idx_set];
        set->line = (Cache_Line *)malloc(cache->num_lines * sizeof(Cache_Line));
        set->lru_array = malloc(cache->num_lines * sizeof(int));

        /* init each line */
        for (int idx_line = 0; idx_line < cache->num_lines; 
                idx_line++) {

            set->lru_array[idx_line] = idx_line;
            set->line[idx_line].v = 0;
            // set->line[idx_line].tag = 0;
            set->line[idx_line].tag = malloc(MAXLINE);
            set->line[idx_line].block = malloc(MAX_OBJECT_SIZE);
            set->line[idx_line].lru = 0;
        }
    }
    dbg_printf("--------In initCache function END ---------\n");
}

/*
 * search_data_from_cache - get data from cache
 *
 * Pamameters:
 * 		url:			using for matching tag in cache
 *		cache_resp:		using for store reponse from cache
 * 
 * Return:	1 - found in cache
 *			0 - not found in cache
 */
int search_data_from_cache(char *url, char *cache_resp) {
   dbg_printf("--------In search_data_from_cache function ---------\n");

    int set_idx, line_idx;
    Cache_Set *set;

    for (set_idx = 0; set_idx < cache.num_sets; set_idx++) {
        set = &cache.set[set_idx];
        for (line_idx = 0; line_idx < cache.num_lines; line_idx++) {
           
           // search in cache line
            if(set->line[line_idx].v && 
              (strcmp(set->line[line_idx].tag, url) == 0)) {

                dbg_printf("set->line[line_idx].tag: %s\n", 
                            set->line[line_idx].tag);
                dbg_printf("url: %s\n", url);

                // lock to prevent race condition
                P(&mutex);
                updateLRUArray(set->lru_array, line_idx, cache.num_lines);
                V(&mutex);

                // load response from cache
                strcpy(cache_resp, set->line[line_idx].block);
                return 1;
            }
        }
    }
    return 0;

    dbg_printf("--------In search_data_from_cache function END ---------\n");
}

/* 
 * caching_from_server - caching data received from server
 * 
 * Pamameters:
 * 		url:			using for matching tag in cache
 *		server_resp:	using for store reponse from server
 */ 
void caching_from_server(char *url, char *server_resp) {
    dbg_printf("--------In caching_from_server function ---------\n");
    
    int set_idx;
    set_idx = 0;

    int idx_lru = getLRU(cache.set[set_idx], cache.num_lines);
    Cache_Set *set = &cache.set[set_idx];

    strcpy(set->line[idx_lru].tag, url);
    strcpy(set->line[idx_lru].block, server_resp);

    dbg_printf("url: %s\n", url);
    dbg_printf("set->line[idx_lru].tag: %s\n", set->line[idx_lru].tag);

    if (set->line[idx_lru].v == 0) {
        set->line[idx_lru].v = 1;
    }

    // updateLRUArray(cache, set_idx, idx_lru);
    updateLRUArray(set->lru_array, idx_lru, cache.num_lines);

    dbg_printf("--------In caching_from_server function END ---------\n");
}

/*
 *  getLRU - return the index of LRU line, using for eviction
 *  
 *  usage: it should be the last element in the lru_array
 */
int getLRU(Cache_Set set, int num_lines) {
    return set.lru_array[num_lines - 1];
}

/* 
 * updateLRUArray - update the least recently used array in cache_set
 *
 * Pamameters:
 *      lru_array:  lru array in cache that using for manipulating lru
 *      line_idx:   using for find target line in cache
 *      num_lines:  tota num of lines in cache
 * 
 */
void updateLRUArray(int *lru_array, int line_idx, int num_lines) {

    dbg_printf("--------In updateLRUArray function ---------\n");

    int i;
    for(i = 0; i < num_lines; i++) {
        /* find the target line index */
        if(lru_array[i] == line_idx) {
             break;
        }
    }

    /* update the lru array */
    for(int j = i; j > 0; j--) {
        lru_array[j] = lru_array[j - 1];
    }     

    /* the most recently used line_idx should always be the first element*/
    lru_array[0] = line_idx;

    dbg_printf("--------In updateLRUArray function END ---------\n");
}

