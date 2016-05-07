/*
 * Name: Ningna Wang
 * AndrewID: ningnaw
 * 
 * cache.c - A simple cache implementation
 *           using for proxy.c 
 *
 * Hierachy:   Cache -> Cache_Set -> Cache_Line
 * Attention:  using LRU (Lease Recently Used) when update cache
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
    isLineFull = 1;

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

        /* init each line */
        for (int idx_line = 0; idx_line < cache->num_lines; 
                idx_line++) {

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

            Cache_Line *currLine = &set->line[line_idx];
            // has invalid line
            if (0 == currLine->v && isLineFull == 1) { 
                isLineFull = 0;
            }
           
            // search in cache line
            if(set->line[line_idx].v && 
              (strcmp(set->line[line_idx].tag, url) == 0)) {

                dbg_printf("set->line[line_idx].tag: %s\n", 
                            set->line[line_idx].tag);
                dbg_printf("url: %s\n", url);

                // lock to prevent race condition
                P(&mutex);
                updateLRU(set, currLine, cache.num_lines);
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
 *      url:            using for matching tag in cache
 *      server_resp:    using for store reponse from server
 */ 
void caching_from_server(char *url, char *server_resp) {
    dbg_printf("--------In caching_from_server function ---------\n");

    int set_idx = 0;
    Cache_Set *currSet = &cache.set[set_idx];
    Cache_Line *currLine;

    /* consider eviction */
    if (isLineFull) { 
        int idx_lru = getLRU(*currSet, cache.num_lines);
        currLine = &currSet->line[idx_lru];
    }
    /* there's an empty line, no eviction */
    else { 
        int idx_empty_line = getEmptyLine(*currSet, cache.num_lines);
        if (-1 == idx_empty_line) {
            printf("Line is full, something wrong\n");
            exit(-1);
        }
        currLine = &currSet->line[idx_empty_line];
    }

    /* update cache line based on data from server */
    strcpy(currLine->tag, url);
    strcpy(currLine->block, server_resp);

    dbg_printf("url: %s\n", url);
    dbg_printf("set->line[idx_lru].tag: %s\n", currLine->tag);

    if (currLine->v == 0) {
        currLine->v = 1;
    }
    updateLRU(currSet, currLine, cache.num_lines);

    dbg_printf("--------In caching_from_server function END ---------\n");
}


/*
 *  getEmptyLine - Return the empty line in a given set
 */
int getEmptyLine(Cache_Set set, int num_lines) {

    for (int idx_line = 0; idx_line < num_lines; idx_line++) {
        if (set.line[idx_line].v == 0) {
            return idx_line;
        }
    }
    return -1; // set is full
}


/*
 *  updateLRU - Update LRU of each line 
 *
 *  Usage: the line with line.lru == 1 
 *           should be the next eviction line
 */
void updateLRU(Cache_Set *set, Cache_Line *currLine, int num_lines) {

    dbg_printf("--------In updateLRU function ---------\n");

    int cur_lru = currLine->lru;

    for(int idx_line= 0; idx_line < num_lines; idx_line++) {
        if (1 == set->line[idx_line].v &&
            set->line[idx_line].lru > cur_lru) {
            set->line[idx_line].lru--;
        }
    }

    currLine->lru = num_lines;
    dbg_printf("--------In updateLRU function END---------\n");
}


/*
 *  getLRU - Return the index of LRU line,
 *  the line with line.lru == 1 
 *  see implementation of updateLRU() function
 */
int getLRU(Cache_Set set, int num_lines) {

    for(int idx_line= 0; idx_line < num_lines; idx_line++) {
        if (1 == set.line[idx_line].lru) {
            return idx_line;
        }
    }
    printf("getLRU error");
    exit(-1);
}
