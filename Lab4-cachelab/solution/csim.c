/*
 *	A Cache Simulator: 
 *	takes a valgrind memory trace as input, simulates 
 *	the hit/miss behavior of a cache memory on this trace, 
 *	and outputs the total number of hits, misses, and evictions.
 *
 *
 *	Name: Ningna Wang
 *	Andrew ID: ningnaw
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
 *	Structs for simulate cache
 *	part1 + part2 + part3
 */
// part1: cache line
// content: valid bit + LRU + tag + data 
typedef struct {
	int v; // 0-invalid, 1-valid
	unsigned long long tag; // 64-bit hexadecimal memory address.
	int lru; // range from 0 ~ #lines/set
} Line;

// part2: cache set
typedef struct {
	Line *lines;
} Set;

// part3: cache
typedef struct {
	Set *sets;
	int num_lines;
	int num_sets;
} Cache;



/*
 *	Parameters:
 *	part4 + part5 + part6
 */
// part4: Input arguments: -t, -E, -b
typedef struct {
	int s; // num of set index bits
	long long S; // 2^s = num of sets
	int b; // block offset
	long long B; // 2^b = bytes per cache block (per line)
	int E; // num of lines
	char *t; // -t <tracefile>: 
	int verbose; // -v, Optional flag that displays trace info
} Arg_param;

// part5: Tracking through caching process
typedef struct {
	int hits;
	int misses;
	int evictions;
} Track_param;

// part6: 
// Address of word: t bits + s bits + b bits
typedef struct {
	unsigned long long tag;
	unsigned long long setIndex;
	unsigned long long blockOffset;
} Address_param;




/*
 *	Init Arg_param & Track_param 
 *	by parsing command-line arguments
 */
void initParam(int argc, char **argv, Arg_param *arg, 
				Track_param *track) {

	bzero(arg, sizeof(*arg));
	int opt;
	// char *trace_file;
	while(-1 != (opt = getopt(argc, argv, "s:E:b:t:"))) {
		switch(opt) { 
			case 'v':
				arg->verbose = 1;
				printf("Print Usage\n");
				exit(0); 
			case 'h':
				printf("Print Usage\n");
				exit(0);
			case 's':
				arg->s = atoi(optarg);
				break;
			case 'E':
				arg->E = atoi(optarg);
				break;
			case 'b':
				arg->b = atoi(optarg);
				break;
			case 't':
				arg->t = optarg;
				break;
			default:
				printf("wrong argument\n");
				exit(-1); 
			}
	} 

	if (0 == arg->s || 0 == arg->E ||
		0 == arg->b || NULL == arg->t) {
		printf("Miss parameters\n");
		exit(-1);
	}

	arg->S = pow(2.0, arg->s); // S = 2^s
	arg->B = 1 << arg->b; // B = 2^b

	// init track
	track->hits = 0;
	track->misses = 0;
	track->evictions = 0;
}


/*
 *	Init a Address_param 
 *	given 64-bit hexadecimal memory address 
 *	Address of word: t bits + s bits + b bits
 */
void initAddress(unsigned long long address, Arg_param arg,
					Address_param *addr) {

	// int t_bit = 64 - (arg.s + arg.b);
	addr->tag = address >> (arg.s + arg.b);
	// unsigned long long temp = address << t_bit;
	// int rightShift = t_bit + arg.b;
	// addr->setIndex = temp >> rightShift;
	addr->setIndex = (address >> arg.b) % (1 << arg.s);
}


/*
 *	Init Cache by passing cache pointer and Arg_param
 *	num_sets = S, num_lines = E
 */
void initCache(Cache *cache, Arg_param arg) {

	// init cache
	// Cache cache;
	cache->num_lines = arg.E;
	cache->num_sets = arg.S;
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
		for (int idx_line = 0; idx_line < cache->num_lines; 
				idx_line++) {

			set->lines[idx_line].v = 0;
			set->lines[idx_line].tag = 0;
			set->lines[idx_line].lru = 0;
		}
	}
}


/*
 *	Free memory for storing cache
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
 *	Return the empty line in a given set
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
 *	Update LRU of each line 
 *	Result: the line with line.lru == 1 
 *			 should be the next eviction line
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
 *	Return the index of LRU line,
 *	the line with line.lru == 1 
 *	see implementation of updateLRU() function
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
 *	Simulate caching process:
 *	Steps:	1. check all lines in target set 
 *			2. if hit, update hits and LRU
 *			3. if miss, update misses
 *	 		=> check miss type:
 *				3.1. if cold miss, save data to empty line
 *				3.2. if eviction: update evictions
 *	 		=> update LUR
 */
void caching(Cache *cache, Track_param *track, Address_param addr) {

	Set *currSet = &cache->sets[addr.setIndex];
	int prevHit = track->hits;
	int isLineFull = 1;

	// update hit
	for (int idx_line = 0; idx_line < cache->num_lines; idx_line++) {
		Line *currLine = &currSet->lines[idx_line];
		// has invalid line
		if (0 == currLine->v && isLineFull == 1) { 
			isLineFull = 0;
		}
		else if (currLine->v && currLine->tag 
					== addr.tag) { // hit
			track->hits++;
			updateLRU(currSet, currLine, cache->num_lines);
		}
	}

	// update miss
	if (track->hits == prevHit) {
		track->misses++; // not find in cache
	}
	else {
		return; // hit occur and found in cache
	}

	// cold miss or evictions
	Line *currLine;

	if (isLineFull) { // eviction
		track->evictions++;
		int idx_lru = getLRU(*currSet, cache->num_lines);
		currLine = &currSet->lines[idx_lru];
	}
	else { // there's an empty line
		int idx_empty_line = 
				getEmptyLine(*currSet, cache->num_lines);
		if (-1 == idx_empty_line) {
			printf("Line is full, something wrong\n");
			exit(-1);
		}
		currLine = &currSet->lines[idx_empty_line];
	}
	currLine->tag = addr.tag;
	currLine->v = 1;
	updateLRU(currSet, currLine, cache->num_lines);

	return;
}



 
/*
 *	Main function to read arguments from command line and simulate 
 *	caching operation
 */
int main(int argc, char **argv) {

	Arg_param arg;
	Track_param track;
	Address_param addr;
	Cache cache;
	initParam(argc, argv, &arg, &track);
	initCache(&cache, arg);

	/* read file line by line */
	char operation;
	unsigned long long address;
	int size;
	FILE *p_file = fopen(arg.t, "r"); 
	if (NULL != p_file) {
		while (3 == fscanf(p_file, " %c %llx,%d", 
				&operation, &address, &size)) {
			switch(operation) {
			 case 'I':
		 		break;
			 case 'L':
			 	initAddress(address, arg, &addr);
			    caching(&cache, &track, addr);
			 	break;
			 case 'S':
		 		initAddress(address, arg, &addr);
			    caching(&cache, &track, addr);
		 		break;
			 case 'M': 
		 		initAddress(address, arg, &addr);
			    caching(&cache, &track, addr); // load
			    caching(&cache, &track, addr); // store
		 		break;
			 default:
		 		break;
       }
		}
	}

	printSummary(track.hits, track.misses, track.evictions);
	fclose(p_file);

	// clean memory
	cleanCache(cache);
	
	return 0;
}



