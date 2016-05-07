/*
 * cache.h - a simple cache
 */

/* $begin cache.h */
#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/*
 *  Structs for simulate cache
 *  part1 + part2 + part3
 */
// part1: cache line
// content: valid bit + LRU + tag + data 
typedef struct {
    int v; // 0-invalid, 1-valid
    char *tag;	 // using url as tag
    char *block; // cache block that used to store context
    int lru; // range from 0 ~ #lines/set
} Cache_Line;

// part2: cache set
typedef struct {
    Cache_Line *line;
} Cache_Set;

// part3: cache
typedef struct {
    Cache_Set *set;
    int num_lines;
    int num_sets;
} Cache;


/* global variables */
Cache cache;
sem_t mutex;
int isLineFull; // using for determin if eviction happens


/* functions define */
void initCache(Cache *cache);
int search_data_from_cache(char *url, char *cache_resp);
void updateLRU(Cache_Set *set, Cache_Line *currLine, int num_lines);
int getLRU(Cache_Set set, int num_lines);
void caching_from_server(char *url, char *server_resp);
int getEmptyLine(Cache_Set set, int num_lines);


#endif /* __CACHE_H__ */
/* $end cache.h */
