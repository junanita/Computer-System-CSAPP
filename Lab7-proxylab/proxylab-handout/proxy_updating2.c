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
	Cache_Line *line;
	int *lru_array; // from 0 ~ #lines, 0 is the most recently used
} Cache_Set;

// part3: cache
typedef struct {
	Cache_Set *set;
	int num_lines;
	int num_sets;
} Cache;





void initCache(Cache *cache);
void *thread(void *vargp);
void updateLRU(int *lru_array, int line_idx, int num_lines);
// static void updateLRU(Cache *cache, int set_idx, int line_idx);
int search_data_from_cache(char *tag, char *response);
int getLRU(Cache_Set set, int num_lines);
void caching_from_server(char *tag, char *response);


void doit(int fd);
void checkClientRequest(int fd, char *method, char *url, char *version);
int parse_uri(char *uri, char *hostname, int *port);
int powerten(int i);


static void createReqestHeader(char *buf, char *request2server, 
								char *hostname, char *uri);



/* helper funtions from tiny.c */
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, 
					 char *shortmsg, char *longmsg);


// global variables
sem_t mutex;
Cache cache;

/* main function */
int main(int argc, char **argv) {
    
	// ignore SIGPIPE signal
	Signal(SIGPIPE, SIG_IGN);

    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE], port[MAXLINE];
    pthread_t tid;// thread id

    /* Check command line args */
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(1);
    }

    // init mutex lock
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
	doit(connfd);
	Close(connfd);
	return NULL; 
}


/*
 *	Init Cache by passing cache pointer and Arg_param
 *	num_sets = 1, num_lines = 10
 */
void initCache(Cache *cache) {
	dbg_printf("--------In initCache function ---------\n");

	// init cache
	cache->num_sets = 1;
	cache->num_lines = 10;

	cache->set = (Cache_Set *)malloc(cache->num_sets * sizeof(Cache_Set));
	if (!cache->set) {
		printf("Init cache sets error");
		exit(-1);
	}

	// init set
	Cache_Set *set;
	for (int idx_set = 0; idx_set < cache->num_sets; idx_set++) {
		set = &cache->set[idx_set];
		set->line = (Cache_Line *)malloc(cache->num_lines * sizeof(Cache_Line));
		set->lru_array = malloc(cache->num_lines * sizeof(int));

		// init line
		for (int idx_line = 0; idx_line < cache->num_lines; 
				idx_line++) {

			set->lru_array[idx_line] = idx_line;
			set->line[idx_line].v = 0;
			// set->line[idx_line].tag = 0;
			set->line[idx_line].tag = malloc(MAXLINE);
			set->line[idx_line].block = malloc(MAX_OBJECT_SIZE);
			// set->line[idx_line].lru = 0;
		}
	}
    dbg_printf("--------In initCache function END ---------\n");
}


/* 
 * updateLRU
 * 
 */
void updateLRU(int *lru_array, int line_idx, int num_lines) {

	dbg_printf("--------In updateLRU function ---------\n");
	// int *lru_array = cache->sets[set_idx].lru_array;
	// int num_lines = cache->num_lines;

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
int search_data_from_cache(char *uri, char *cache_resp) {
   dbg_printf("--------In search_data_from_cache function ---------\n");

	int set_idx, line_idx;
	Cache_Set *set;

	set_idx = 0;
	// for (set_idx = 0; set_idx < cache->num_sets; set_idx++) {
		set = &cache.set[set_idx];
	    for (line_idx = 0; line_idx < cache.num_lines; line_idx++) {
	       
	       // search in cache line
	    	if(set->line[line_idx].v && 
	          (strcmp(set->line[line_idx].tag, uri) == 0)) {

	          	dbg_printf("set->line[line_idx].tag: %s\n", 
	          				set->line[line_idx].tag);
	          	dbg_printf("uri: %s\n", uri);

	          	// lock to prevent race condition
	            P(&mutex);
	            updateLRU(set->lru_array, line_idx, cache.num_lines);
	            V(&mutex);

	            // load response from cache
	            strcpy(cache_resp, set->line[line_idx].block);
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
void caching_from_server(char *uri, char *server_resp) {
    dbg_printf("--------In caching_from_server function ---------\n");
    
    int set_idx;
    set_idx = 0;

    int idx_lru = getLRU(cache.set[set_idx], cache.num_lines);
    Cache_Set *set = &cache.set[set_idx];

    strcpy(set->line[idx_lru].tag, uri);
    strcpy(set->line[idx_lru].block, server_resp);

    dbg_printf("uri: %s\n", uri);
    dbg_printf("set->line[idx_lru].tag: %s\n", set->line[idx_lru].tag);

    if (set->line[idx_lru].v == 0) {
        set->line[idx_lru].v = 1;
    }

    // updateLRU(cache, set_idx, idx_lru);
	updateLRU(set->lru_array, idx_lru, cache.num_lines);

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

/* $begin doit */
void doit(int fd) {

	dbg_printf("--------In doit function ---------\n");

	char buf[MAXLINE];
	char method[MAXLINE], url[MAXLINE], version[MAXLINE];
	char request2server[MAXLINE];
	char uri[MAXLINE], hostname[MAXLINE];
	rio_t rio;

    int *port;
    char port2[MAXLINE];
    port = malloc(sizeof(int));

    /* receive request from client */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) { //read request
    	printf("Error: cannot receive request\n");
        return;
    }
    printf("Receive request from client: %s", buf);

    /* parse request */
    sscanf(buf, "%s %s %s", method, url, version);  
	

	/* check correctness of client reqeust */
	checkClientRequest(fd, method, url, version);   
    
	/* read request headers */
    read_requesthdrs(&rio);


    /* Parse URI from GET request */
    parse_uri(url, hostname, port);   
    strcpy(uri, url);    
    // strcpy(filename, url);
    sprintf(request2server, "%s %s %s\r\n", method, uri, version);
    printf("proxy to server: %s\n", request2server);

    // request header
    // createReqestHeader(buf, request2server, hostname);

    /* generate request to server */
    createReqestHeader(buf, request2server, hostname, uri);

    /* forward request to server */
    // forwardRequest2Server(fd, cache, req_p, request2server, uri);



    char cache_resp[MAX_OBJECT_SIZE]; 	// store response from cache
    char server_resp[MAX_OBJECT_SIZE];  // store response from server
    rio_t rio_proxy_ser; 
    unsigned int obj_size, cnt;
    
    /* load data from cache */
    if (search_data_from_cache(uri, cache_resp) == 1) {
    	dbg_printf("found data in cache! \n");
    	Rio_writen(fd, cache_resp, sizeof(cache_resp));      
        memset(cache_resp, 0, sizeof(cache_resp)); 
        dbg_printf("finish response from cache!\n");
    }

    /* no data in cache, send reqeust to server */
    else {   
       	dbg_printf("No data in cache, send request to server! \n");

        sprintf(port2, "%d", *port);

        /* open a socket descriptor ready to read and write */
        int clientfd = Open_clientfd(hostname, port2); 
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
        	Rio_writen(fd, server_resp, cnt);
        	memset(server_resp, 0, sizeof(server_resp));
        }


        /* check if data can be cached */
        if (obj_size < MAX_OBJECT_SIZE) {
        	P(&mutex);
        	caching_from_server(uri, cache_resp);
        	V(&mutex);
        }

        /* close proxy connection with server */
        Close(clientfd);
    }
}



void checkClientRequest(int fd, char *method, char *url, char *version) {

	/* check request method, only accept GET method for now */    
	if (strcasecmp(method, "GET")) {                     
	    clienterror(fd, method, "501", "Not Implemented",
	                "Proxy does not implement this method");
	    return;
	}  

	/* check reqeust url */   
	char url_prefix[7]; // using for checking "http://"
	strncpy(url_prefix, url, 7);
	if (strcasecmp(url_prefix, "http://")) {
		clienterror(fd, url, "400", "Bad Request",
					"Proxy cannot understand this url");
		return;
	}

	/* check request version, should use "HTTP/1.0" */
	if (strcasecmp(version, "HTTP/1.0")) {
		dbg_printf("Attention: only accept HTTP/1.0, change to HTTP/1.0\n");
		strcpy(version, "HTTP/1.0");
	}

}



/*
 * createReqestHeader - create request header that will send to server
 */
void createReqestHeader(char *buf, char *request2server, 
					 	char *hostname,	char *uri) {

	dbg_printf("--------In createReqestHeader function--------\n");
    
    // if(strcmp(buf, "Host"))
    // {
    //       strcat(request2server, "Host: ");
    //       strcat(request2server, hostname);
    //       strcat(request2server, "\r\n");
    // }
    // if(strcmp(buf, "User-Agent:")) {
    //     strcat(request2server, user_agent_hdr);
    // }
    // if(strcmp(buf, "Proxy-Connection:")) {
    //     strcat(request2server, "Proxy-Connection: close\r\n");
    // }
    // if(strcmp(buf, "Connection:")) {
    //     strcat(request2server, "Connection: close\r\n");
    // }
    // strcat(request2server, "\r\n");

    if (strcmp(buf, "Host")) {
    	char host_str[MAXLINE];
    	strcat(host_str, "Host: ");
    	strcat(host_str, hostname);
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


    dbg_printf("--------In createReqestHeader function END--------\n");
}



/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {      
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *hostname, int *port) 
{
    // in this lab all requests are static 

    char tmp[MAXLINE];          // holds local copy of uri
    char *buf;                  // ptr that traverses uri
    char *endbuf;               // ptr to end of the cmdline string
    int port_tmp[10];
    int i, j;                   // loop
    char num[2];                // store port value

    buf = tmp;
    for (i = 0; i < 10; i++) {
        port_tmp[i] = 0;
    }
    (void) strncpy(buf, uri, MAXLINE);
    endbuf = buf + strlen(buf);
    buf += 7;                   // 'http://' has 7 characters
    while (buf < endbuf) {
    // take host name out
        if (buf >= endbuf) {
            strcpy(uri, "");
            strcat(hostname, " ");
            // no other character found
            break;
        }
        if (*buf == ':') {  // if port number exists
            buf++;
            *port = 0;
            i = 0;
            while (*buf != '/') {
                num[0] = *buf;
                num[1] = '\0';
                port_tmp[i] = atoi(num);
                buf++;
                i++;
            }
            j = 0;
            while (i > 0) {
                *port += port_tmp[j] * powerten(i - 1);
                j++;
                i--;
            }
        }
        if (*buf != '/') {

            sprintf(hostname, "%s%c", hostname, *buf);
        }
        else { // host name done
            strcat(hostname, "\0");
            strcpy(uri, buf);
            break;
        }
        buf++;
    }
    return 1;
}
/* $end parse_uri */

/*
 * powerten - return ten to the power of i
 */
int powerten(int i) {
    int num = 1;
    while (i > 0) {
        num *= 10;
        i--;
    }
    return num;
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}