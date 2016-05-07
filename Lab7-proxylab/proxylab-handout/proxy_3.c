/*
 * Name: Ningna Wang
 * AndrewID: ningnaw
 * 
 * Simple version please checkout tiny.c
 * 
 */


#include <stdio.h>

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <strings.h>
#include <string.h>

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


/* Using following macro to debug output */
#define DEBUG

#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif



/*
 *	Structs for simulate cache
 *	part1 + part2 + part3
 */
// part1: cache line
// content: valid bit + LRU + tag + data 
typedef struct {
	int v; // 0-invalid, 1-valid
	// unsigned long long tag; // 64-bit hexadecimal memory address.
	char *tag;
	char *block; // cache block that used to store context
	int lru; // range from 0 ~ #lines/set
} Cache_Line;

// part2: cache set
typedef struct {
	Cache_Line *lines;
	int *lru_array; // from 0 ~ #lines, 0 is the most recently used
} Cache_Set;

// part3: cache
typedef struct {
	Cache_Set *sets;
	int num_lines;
	int num_sets;
} Cache;



/* Struct for parsed user request */
typedef struct {
	char *url;
	char *method;
	char *hostname;
	char *uri;
	char *port;
	char *version;
	char *header;
}Req_param;


/* define functions */
void initCache(Cache *cache);
void updateLRU(Cache *cache, int set_idx, int line_idx);
int search_data_from_cache(Cache *cache, char *uri, char *cache_resp);
void caching_from_server(Cache *cache, char *uri, char *server_resp);
int getLRU(Cache_Set set, int num_lines);
void parse_uri(Req_param *req_p);
void checkClientRequest(int fd, Req_param *req_p);
void createReqestHeader(char *request2server, char *buf, char *uri);
// void createReqestHeader(char *request2server, char *buf, char *hostname);
void doit(int fd, Cache *cache);
void forwardRequest2Server(int fd, Cache *cache, Req_param req_p, 
							char *request2server, char *uri);
void *thread(void *vargp);


/* helper funtions from tiny.c */
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, 
					 char *shortmsg, char *longmsg);



/* global variable */
sem_t mutex;
Cache cache;  // cache using for caching data from server 
Req_param req_p;



/*
 *	Init Cache by passing cache pointer and Arg_param
 *	num_sets = 1, num_lines = 10
 */
void initCache(Cache *cache) {
	dbg_printf("--------In initCache function ---------\n");

	// init cache
	cache->num_lines = 10;
	cache->num_sets = 1;
	cache->sets = (Cache_Set *)malloc(cache->num_sets * sizeof(Cache_Set));
	if (!cache->sets) {
		printf("Init cache error");
		exit(-1);
	}

	// init set
	Cache_Set *set;
	for (int idx_set = 0; idx_set < cache->num_sets; idx_set++) {
		set = &cache->sets[idx_set];
		set->lines = (Cache_Line *)malloc(cache->num_lines * sizeof(Cache_Line));
		set->lru_array = malloc(cache->num_lines * sizeof(int));

		// init line
		for (int idx_line = 0; idx_line < cache->num_lines; 
				idx_line++) {

			set->lru_array[idx_line] = idx_line;
			set->lines[idx_line].v = 0;
			// set->lines[idx_line].tag = 0;
			set->lines[idx_line].tag = malloc(MAXLINE);
			set->lines[idx_line].block = malloc(MAX_OBJECT_SIZE);
			// set->lines[idx_line].lru = 0;
		}
	}
	dbg_printf("--------In initCache function END ---------\n");
}


/* 
 * updateLRU
 * 
 */
void updateLRU(Cache *cache, int set_idx, int line_idx) {	

	dbg_printf("--------In updateLRU function ---------\n");
	int *lru_array = cache->sets[set_idx].lru_array;
	int num_lines = cache->num_lines;

	int i;
    for(i = 0; i < num_lines; i++) {
        if(lru_array[i] == line_idx) {
             break;
        }
    }
    for(int j = i; j > 0; j--) {
        lru_array[j] = lru_array[j - 1];
    }                               
    lru_array[0] = line_idx;

    dbg_printf("--------In updateLRU function END ---------\n");

}


/*
 * search_data_from_cache - get data from cache
 */
int search_data_from_cache(Cache *cache, char *uri, char *cache_resp) {
	dbg_printf("--------In search_data_from_cache function ---------\n");

    int set_idx, line_idx;
    Cache_Set *set;

    set_idx = 0;
    // for (set_idx = 0; set_idx < cache->num_sets; set_idx++) {
    	set = &cache->sets[set_idx];
	    for (line_idx = 0; line_idx < cache->num_lines; line_idx++) {
	       
	       // search in cache line
	    	if(set->lines[line_idx].v && 
	          (strcmp(set->lines[line_idx].tag, uri) == 0)) {

	          	dbg_printf("set->lines[line_idx].tag: %s\n", 
	          				set->lines[line_idx].tag);
	          	dbg_printf("uri: %s\n", uri);

	          	// lock to prevent race condition
	            P(&mutex);
	            updateLRU(cache, set_idx, line_idx);
	            V(&mutex);

	            // load response from cache
	            strcpy(cache_resp, set->lines[line_idx].block);
	            return 1;
	        }
	    }
	// }
	return 0;
    dbg_printf("--------In search_data_from_cache function END ---------\n");
}


/* 
 * caching_from_server
 */ 
void caching_from_server(Cache *cache, char *uri, char *server_resp) {

	dbg_printf("--------In caching_from_server function ---------\n");
    
    int set_idx;
    set_idx = 0;

    int idx_lru = getLRU(cache->sets[set_idx], cache->num_lines);
    Cache_Set *set = &cache->sets[set_idx];

    strcpy(set->lines[idx_lru].tag, uri);
    strcpy(set->lines[idx_lru].block, server_resp);

    dbg_printf("uri: %s\n", uri);
    dbg_printf("set->lines[idx_lru].tag: %s\n", set->lines[idx_lru].tag);


    if (set->lines[idx_lru].v == 0) {
        set->lines[idx_lru].v = 1;
    }
    updateLRU(cache, set_idx, idx_lru);

    dbg_printf("--------In caching_from_server function END ---------\n");
}


/*
 *	Return the index of LRU line, using for eviction
 * 	it should be the last element in the lru_array
 */
int getLRU(Cache_Set set, int num_lines) {
	return set.lru_array[num_lines - 1];
}



/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd, Cache *cache) {
    
    dbg_printf("--------In doit function ---------\n");

    // initialize request parameters
    // Req_param req_p = {.url="", .method="", .hostname="", 
    // 					.uri="", .port="", .version="", .header=""};

    char buf[MAXLINE];
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char request2server[MAXLINE];
    char uri[MAXLINE];
    rio_t rio;

    memset(uri, 0, sizeof(uri));

    /* receive request from client */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) { //read request
    	printf("Error: cannot receive request\n");
        return;
    }
    printf("Receive request from client: %s", buf);
   
   	/* parse request */
    sscanf(buf, "%s %s %s", method, url, version);  
    // req_p.method = method;
    // req_p.url = url;
    // req_p.version = version;
    req_p.method = Malloc(strlen(method)+1);
    strcpy(req_p.method, method);

    req_p.url = Malloc(strlen(url)+1);
    strcpy(req_p.url, url);

    req_p.version = Malloc(strlen(version)+1);
    strcpy(req_p.version, version);

    /* check correctness of client reqeust */
    checkClientRequest(fd, &req_p);

    /* read request headers */
    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    parse_uri(&req_p);
    sprintf(request2server, "%s %s %s\r\n", 
    		req_p.method, req_p.uri, req_p.version);
    strcpy(uri, req_p.uri);
    dbg_printf("request from proxy to server: %s\n", request2server);


    /* generate request to server */
    // createReqestHeader(request2server, buf, &req_p);
    createReqestHeader(request2server, buf, uri);


    /* forward request to server */
    forwardRequest2Server(fd, cache, req_p, request2server, uri);

   dbg_printf("--------In doit function END--------\n");

}


void forwardRequest2Server(int fd, Cache *cache, Req_param req_p, 
							char *request2server, char *uri) {

	dbg_printf("--------In forwardRequest2Server function--------\n");

	dbg_printf("req_p.hostname: %s\n", req_p.hostname);
	dbg_printf("req_p.uri: %s\n", req_p.uri);
	dbg_printf("uri: %s\n", uri);
	dbg_printf("req_p.port: %s\n", req_p.port);


	char cache_resp[MAX_OBJECT_SIZE]; 	// store response from cache
	char server_resp[MAX_OBJECT_SIZE];  // store response from server
	rio_t rio_proxy_ser; 
	unsigned int obj_size, cnt;

	memset(cache_resp, 0, sizeof(cache_resp)); 

	/* load data from cache */
	if (search_data_from_cache(cache, uri, cache_resp)) {
		dbg_printf("found data in cache! \n");
		Rio_writen(fd, cache_resp, sizeof(cache_resp));
		memset(cache_resp, 0, sizeof(cache_resp)); 
		dbg_printf("finish response from cache!\n");
	}

	/* no data in cache, send reqeust to server */
	else {
		dbg_printf("No data in cache, send request to server! \n");

		/* open a socket descriptor ready to read and write */
		int clientfd = Open_clientfd(req_p.hostname, req_p.port); 
		Rio_readinitb(&rio_proxy_ser, clientfd);

		/* send connection to server */
		dbg_printf("send connection to server: \n");
		Rio_writen(clientfd, request2server, strlen(request2server));

		/* receive data from server */
		dbg_printf("start receiving data from server:\n");
		obj_size = 0;
		while ((cnt = Rio_readnb(&rio_proxy_ser, 
				server_resp, sizeof(server_resp))) > 0) {
			
			/* accumulate cache object size */
			obj_size += cnt;
			strcat(cache_resp, server_resp);

			dbg_printf("server response: \n '%s' \n", server_resp);
			Rio_writen(fd, server_resp, strlen(server_resp));
			memset(server_resp, 0, sizeof(server_resp));
		}

		/* check if data can be cached */
		if (obj_size < MAX_OBJECT_SIZE) {
			P(&mutex);
			caching_from_server(cache, uri, cache_resp);
			V(&mutex);
		}

		/* close proxy connection with server */
		Close(clientfd);
	}

	dbg_printf("--------In forwardRequest2Server function END--------\n");
}


/*
 * createReqestHeader - create request header that will send to server
 */
void createReqestHeader(char *request2server, char *buf, char *uri) {

	dbg_printf("--------In createReqestHeader function--------\n");

	dbg_printf("req_p.uri: %s\n", req_p.uri);
	
	if (strcmp(buf, "Host")) {
		char host_str[MAXLINE];
		strcat(host_str, "Host: ");
		strcat(host_str, req_p.hostname);
		strcat(host_str, "\r\n");

		strcat(request2server, host_str);

		dbg_printf("after add 'Host':\n");
		dbg_printf("uri: %s\n", uri);

	}

	if (strcmp("User-Agent", buf)) {
		strcat(request2server, user_agent_hdr);
		dbg_printf("after add 'User-Agent':\n");
		dbg_printf("uri: %s\n", uri);
	}

	if (strcmp("Connection", buf)) {
		strcat(request2server, "Connection: close\r\n");
		dbg_printf("after add 'Connection': \n");
		dbg_printf("uri: %s\n", uri);
	}

	if (strcmp("Proxy-Connection", buf)) {
		strcat(request2server, "Proxy-Connection: close\r\n");
		dbg_printf("after add 'Proxy-Connection':\n");
		dbg_printf("uri: %s\n", uri);
	}

	strcat(request2server, "\r\n");
	dbg_printf("createReqestHeader: \n'%s'", request2server);

	dbg_printf("uri: %s\n", uri);


	dbg_printf("--------In createReqestHeader function END--------\n");
}


void checkClientRequest(int fd, Req_param *req_p) {

	/* check request method, only accept GET method for now */    
	if (strcasecmp(req_p->method, "GET")) {                     
	    clienterror(fd, req_p->method, "501", "Not Implemented",
	                "Proxy does not implement this method");
	    return;
	}  

	/* check reqeust url */   
	char url_prefix[7]; // using for checking "http://"
	strncpy(url_prefix, req_p->url, 7);
	if (strcasecmp(url_prefix, "http://")) {
		clienterror(fd, req_p->url, "400", "Bad Request",
					"Proxy cannot understand this url");
		return;
	}

	/* check request version, should use "HTTP/1.0" */
	if (strcasecmp(req_p->version, "HTTP/1.0")) {
		dbg_printf("Attention: only accept HTTP/1.0, change to HTTP/1.0\n");
		strcpy(req_p->version, "HTTP/1.0");
	}

}



/* main function */
int main(int argc, char **argv) {

	// ignore SIGPIPE signal
	Signal(SIGPIPE, SIG_IGN);

    int listenfd, *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid; // thread id

    /* Check command line args */
    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
    }

    // init 
    sem_init(&mutex, 0, 1);

    // init cache
    initCache(&cache);

    listenfd = Open_listenfd(argv[1]);
    while (1) {
		clientlen = sizeof(clientaddr);

		//accept request from client
		connfdp = Malloc(sizeof(int)); // avoid unintended sharing 
		*connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		// doit(connfd, &cache);	//line:netp:tiny:doit

		/* create thread to process request */
		Pthread_create(&tid, NULL, thread, connfdp);
    }
}


/* thread using for process request from client */
/* please see slides of Lecture: Concurrency page 46 for more detail */
void *thread(void *vargp) {
	int connfd = *((int *)vargp); 
	Pthread_detach(pthread_self()); 
	Free(vargp);
	doit(connfd, &cache);
	Close(connfd);
	return NULL; 
}



/*
 * parse_uri - parse URI into filename and CGI args
 *             only care about static request
 */
void parse_uri(Req_param *req_p) {
	dbg_printf("--------In parse_uri function --------\n");

	char *temp, *temp2, *domain, *p; // using for parsing url string
	char *url = req_p->url;
	dbg_printf("request url is: %s\n", url);

	/* 
	 * should handle following urls:
	 * eg: 	http://localhost:15213/home.html
	 *		http://www.cmu.edu:8080/hub/index.html
	 *		http://www.cmu.edu/hub/index.html
	 *		http://www.cmu.edu
	 */ 

	// parse "http:"
	p = strtok_r(url, "//", &temp);

	// parse domain when port is in client request
	if((domain = strtok_r(NULL, "/", &temp))) {
		dbg_printf("request initial domain is '%s' \n", domain);

		// parse port
		if ((p = strtok_r(domain, ":", &temp2))) {

			dbg_printf("request hostname is '%s'\n", p);
			req_p->hostname = Malloc(strlen(p) + 1);
			req_p->hostname = p;

			// port exist
			if ((p = strtok_r(NULL, "/", &temp2))) {
				req_p->port = Malloc(strlen(p) + 1);
				req_p->port = p;
				dbg_printf("request port is '%s' \n", req_p->port);
			}
		}

		// parse uri
		if ((p = strtok_r(NULL, " ", &temp))) {
			char temp3[MAXLINE];
			strcat(temp3, "/");
			strcat(temp3, p);

			req_p->uri = Malloc(strlen(temp3) + 1);
			req_p->uri = temp3;
			dbg_printf("request uri is '%s'\n", req_p->uri);
		}

	}

	dbg_printf("--------In parse_uri function END--------\n");
}



/* helper funtions from tiny.c */


/*
 * read_requesthdrs - read HTTP request headers
 */
void read_requesthdrs(rio_t *rp) {

	dbg_printf("--------In read_requesthdrs function ---------\n");
   
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);

    // read client request line by line
    while(strcmp(buf, "\r\n")) {          
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
    }

    dbg_printf("after while loop: \n");
    dbg_printf("rp->rio_buf: %s\n", rp->rio_buf);
    dbg_printf("ro->rio_cnt: %d\n", rp->rio_cnt);

    dbg_printf("--------In read_requesthdrs function END---------\n");

    return;
}


/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
					 char *shortmsg, char *longmsg) {

    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}



