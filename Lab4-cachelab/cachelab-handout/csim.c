/*
	Name: Ningna Wang
	Andrew ID: ningnaw
*/

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <strings.h>
#include <string.h>


#include "cachelab.h"


/*
	Struct for cache line
	content: valid bit + LRU + tag + data 
*/
typedef struct {
	int v; // valid bit, 0-invalid, 1-valid
	unsigned long long tag; // 64-bit hexadecimal memory address.
	int lru; // range from 0 ~ #lines/set
} Line;


typedef struct {
	Line *lines;
} Set;

typedef struct {
	Set *sets;
	int num_lines;
	int num_sets;
} Cache;


/*
	Input arguments: -t, -E, -b
*/
typedef struct {
	int s; // num of set index bits
	long long S; // 2^s num of sets
	int b; // block offset
	long long B; // 2^b bytes per cache block (in per line)
	int E; // num of lines
	char *t; // -t <tracefile>: Name of the valgrind trace to replay
	int verbose; // -v, Optional verbose flag that displays trace info
} Arg_param;

typedef struct {
	int hits;
	int misses;
	int evictions;
} Track_param;

/*
	Address of word: t bits + s bits + b bits
*/
typedef struct {
	unsigned long long tag;
	unsigned long long setIndex;
	unsigned long long blockOffset;
} Address_param;



/*
	Init param by parsing command-line arguments
*/
void initParam(int argc, char **argv, Arg_param *arg_param, Track_param *track_param) {

	bzero(arg_param, sizeof(*arg_param));
	int opt;
	// char *trace_file;
	while(-1 != (opt = getopt(argc, argv, "s:E:b:t:"))) {
		switch(opt) { 
			case 'v':
				arg_param->verbose = 1;
				printf("Print Usage\n");
				exit(0); 
			case 'h':
				printf("Print Usage\n");
				exit(0);
			case 's':
				arg_param->s = atoi(optarg);
				break;
			case 'E':
				arg_param->E = atoi(optarg);
				break;
			case 'b':
				arg_param->b = atoi(optarg);
				break;
			case 't':
				arg_param->t = optarg;
				break;
			default:
				printf("wrong argument\n");
				exit(-1); 
			}
	} 

	if (0 == arg_param->s || 0 == arg_param->E ||
			0 == arg_param->b || NULL == arg_param->t) {
		printf("Miss parameters\n");
		exit(-1);
	}

	arg_param->S = pow(2.0, arg_param->s); // S = 2^s
	arg_param->B = 1 << arg_param->b; // B = 2^b


	// init track_param
	track_param->hits = 0;
	track_param->misses = 0;
	track_param->evictions = 0;
}


/*
	Init a Address_param given 64-bit hexadecimal memory address 
*/
void initAddress(unsigned long long address, Arg_param arg_param,
									Address_param *addr_param) {

	// int t_bit = 64 - (arg_param.s + arg_param.b);
	addr_param->tag = address >> (arg_param.s + arg_param.b);
	// unsigned long long temp = address << t_bit;
	// int rightShift = t_bit + arg_param.b;
	// addr_param->setIndex = temp >> rightShift;
	addr_param->setIndex = (address >> arg_param.b) % (1 << arg_param.s);
}


/*
	Init Cache by passing cache pointer and arg_param
	num_sets = S, num_lines = E
*/
void initCache(Cache *cache, Arg_param arg_param) {

	// init cache
	// Cache cache;
	cache->num_lines = arg_param.E;
	cache->num_sets = arg_param.S;
	cache->sets = (Set *)malloc(cache->num_sets * sizeof(Set));
	if (!cache->sets) {
		printf("Init cache error");
		exit(-1);
	}
	// init set
	Set *set;
	for (int idx_set = 0; idx_set < cache->num_sets; idx_set++) {
		set = &cache->sets[idx_set];
		set->lines = (Line *)malloc(cache->num_lines * sizeof(Line));
		// init line
		for (int idx_line = 0; idx_line < cache->num_lines; idx_line++) {
			set->lines[idx_line].v = 0;
			set->lines[idx_line].tag = 0;
			set->lines[idx_line].lru = 0;
		}
	}
}


/*
	free memory for storing cache
*/
void cleanCache(Cache cache) {

	for (int idx_set = 0; idx_set < cache.num_sets; idx_set++) {
		if (NULL != cache.sets[idx_set].lines) {
			free(cache.sets[idx_set].lines);
		}
	}

	if (NULL != cache.sets) {
		free(cache.sets);
	}
}


/*
	return the empty line in a given set
*/
int getEmptyLine(Set set, int num_lines) {

	for (int idx_line = 0; idx_line < num_lines; idx_line++) {
		if (set.lines[idx_line].v == 0) {
			return idx_line;
		}
	}
	return -1; // set is full
}


/*
	return the index of LRU line 
*/
int getLRU(Set set, int num_lines) {

	for(int idx_line= 0; idx_line < num_lines; idx_line++) {
		if (1 == set.lines[idx_line].lru) {
			return idx_line;
		}
	}
	printf("getLRU error");
	exit(-1);
}


/*
	 update LRU of each line 
	 result: the line with line.lru == 1 should be the next
	 eviction line
*/
void updateLRU(Set *set, Line *currLine, int num_lines) {
	int cur_lru = currLine->lru;

	for(int idx_line= 0; idx_line < num_lines; idx_line++) {
		if (1 == set->lines[idx_line].v &&
				set->lines[idx_line].lru > cur_lru) {
			set->lines[idx_line].lru--;
		}
	}

	currLine->lru = num_lines;
}



/*
	Simulate caching process
*/
void caching(Cache *cache, Track_param *track_param, Address_param addr_param) {

	Set *currSet = &cache->sets[addr_param.setIndex];
	int prevHit = track_param->hits;
	int isLineFull = 1;

	// update hit
	for (int idx_line = 0; idx_line < cache->num_lines; idx_line++) {
		Line *currLine = &currSet->lines[idx_line];
		
		if (0 == currLine->v && isLineFull == 1) { // has invalid line
			isLineFull = 0;
		}
		else if (currLine->v && currLine->tag == addr_param.tag) { // hit
			track_param->hits++;
			updateLRU(currSet, currLine, cache->num_lines);
		}
	}

	// update miss
	if (track_param->hits == prevHit) {
		track_param->misses++; // not find in cache
	}
	else {
		return; // hit occur and found in cache
	}

	// cold miss || evictions
	Line *currLine;

	if (isLineFull) { // eviction
		track_param->evictions++;
		int idx_lru = getLRU(*currSet, cache->num_lines);
		currLine = &currSet->lines[idx_lru];
	}
	else { // there's an empty line
		int idx_empty_line = getEmptyLine(*currSet, cache->num_lines);
		if (-1 == idx_empty_line) {
			printf("Line is full, something wrong\n");
			exit(-1);
		}
		currLine = &currSet->lines[idx_empty_line];
	}
	currLine->tag = addr_param.tag;
	currLine->v = 1;
	updateLRU(currSet, currLine, cache->num_lines);

	return;
}



 
/*
	Main func to init Arg_param
*/
int main(int argc, char **argv) {

	Arg_param arg_param;
	Track_param track_param;
	Cache cache;
	initParam(argc, argv, &arg_param, &track_param);
	initCache(&cache, arg_param);

	// read file line by line
	char operation;
	unsigned long long address;
	int size;
	Address_param addr_param;

	FILE *p_file = fopen(arg_param.t, "r"); 
	if (NULL != p_file) {
		while (3 == fscanf(p_file, " %c %llx,%d", &operation, &address, &size)) {
			switch(operation) {
			 case 'I':
			 		break;
			 case 'L':
			 		initAddress(address, arg_param, &addr_param);
			    caching(&cache, &track_param, addr_param);
			 		break;
			 case 'S':
			 		initAddress(address, arg_param, &addr_param);
			 		caching(&cache, &track_param, addr_param);
			 		break;
			 case 'M':
			 		initAddress(address, arg_param, &addr_param);
			    caching(&cache, &track_param, addr_param); // load 
			    caching(&cache, &track_param, addr_param); // store
			 		break;
			 default:
			 		break;
       }
		}
	}

	
	printSummary(track_param.hits, track_param.misses, track_param.evictions);
	fclose(p_file);

	// clean memory
	cleanCache(cache);
	
	return 0;
}



