/*
 * Name: Ningna Wang
 * AndrewID: ningnaw
 * 
 * Proxy Lab: 
 *          A simple HTTP proxy that caches web objects (Simple version 
 * please checkout tiny.c), which acts as an intermediary between clients 
 * making requests to access resources and the servers that satisfy those 
 * requests by serving content.
 * 
 * Attention:
 * 1. usage: ./proxy <port> 
 * 2. only accept GET request
 * 3. support multi-request using thread
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
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* Using following macro to debug output */
// #define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/* functions definds */
void doit(int fd);
void *thread(void *vargp);
void parse_url(char *url, char *hostname, char *port);
void checkClientRequest(int fd, char *method, char *url, char *version);
void createReqestHeader(char *buf, char *request2server, char *hostname);
void forwardRequest2Server(int fd, char *request2server, 
                            char *hostname, char *uri, char *port);

/* helper funtions from tiny.c */
void read_requesthdrs(rio_t *rp);
void clienterror(int fd, char *cause, char *errnum, 
                     char *shortmsg, char *longmsg);

/* global variables */
sem_t mutex;



/* 
 * main function 
 */
int main(int argc, char **argv) {
    
    // ignore SIGPIPE signal
    Signal(SIGPIPE, SIG_IGN);

    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
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
        // doit(connfd, &cache);    //line:netp:tiny:doit

        /* create thread to process request */
        Pthread_create(&tid, NULL, thread, connfdp);
    }
}


/* 
 * thread - using for process request from client
 * (please see slides of Lecture: Concurrency page 46 for more detail)
 */
void *thread(void *vargp) {
    int connfd = *((int *)vargp); 
    Pthread_detach(pthread_self()); // detach! important
    Free(vargp);
    doit(connfd); // see tiny.c for more info
    Close(connfd);
    return NULL; 
}


/*
 * doit - handle one HTTP request/response transaction
 *		  (please refer tiny.c for this part)
 *
 * Pamameters:
 *		fd:		file descriptor
 */
void doit(int fd) {

    dbg_printf("--------In doit function--------\n");

    char buf[MAXLINE];
    char method[MAXLINE], url[MAXLINE], version[MAXLINE], uri[MAXLINE];         
    char *port;
    char *default_port;
    char hostname[MAXBUF], request2server[MAXLINE];          
    rio_t rio;             
    
    /* set the default HTTP port, which is port 80 */
    // port = malloc(sizeof(int));
    port = malloc(sizeof(char));
    default_port = "80";                      

    /* receive request from client */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) { //read request
        dbg_printf("Error: cannot receive request\n");
        return;
    }
    dbg_printf("Receive request from client: %s", buf);

    /* parse request */
    sscanf(buf, "%s %s %s", method, url, version);  

    /* check correctness of client reqeust */
    checkClientRequest(fd, method, url, version);

    /* read request headers */
    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    parse_url(url, hostname, port);  
    dbg_printf("port after parse: %s\n", port);
    /* if no port specified, use "80" */
    if (strcmp(port, "") == 0) { 
        port = default_port;
    }

    strcpy(uri, url);     
    sprintf(request2server, "%s %s %s\r\n", method, uri, version);
    dbg_printf("request from proxy to server: %s\n", request2server);

    /* generate request to server */
    createReqestHeader(buf, request2server, hostname);

    /* check cache before send request to server */
    char cache_resp[MAX_OBJECT_SIZE];   // store response from cache
    /* load data from cache */
    if (search_data_from_cache(uri, cache_resp)) {
        dbg_printf("found data in cache! \n");
        Rio_writen(fd, cache_resp, sizeof(cache_resp));
        memset(cache_resp, 0, sizeof(cache_resp)); 
        dbg_printf("finish response from cache!\n");
    }
    /* no data in cache, send reqeust to server */
    else { 
    	dbg_printf("No data in cache, send request to server! \n");  
    	/* forward request to server */
    	forwardRequest2Server(fd, request2server, hostname, url, port);       
    }

    dbg_printf("--------In doit function END--------\n");
}


/*
 * forwardRequest2Server - forward request from proxy to server
 *							when cannot find data in cache
 *
 * Pamameters:
 *		fd:					file descriptor
 *	request2server:			store the request that will send to server
 *		
 */
void forwardRequest2Server(int fd, char *request2server, 
                            char *hostname, char *uri, char *port) {
    dbg_printf("--------In forwardRequest2Server function--------\n");

    char cache_resp[MAX_OBJECT_SIZE];   // store response from cache
    char server_resp[MAX_OBJECT_SIZE];  // store response from server
    rio_t rio_proxy_ser; 
    unsigned int obj_size, cnt;

    /* open a socket descriptor ready to read and write */
    int clientfd = Open_clientfd(hostname, port); 
    Rio_readinitb(&rio_proxy_ser, clientfd);

    /* send connection to server */
    dbg_printf("send connection to server: \n");
    Rio_writen(clientfd, request2server, strlen(request2server));

    /* receive data from server */
    dbg_printf("start receiving data from server:\n");
    // memset(cache_resp, 0, sizeof(cache_resp));
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
    if (obj_size <= MAX_OBJECT_SIZE) {
        P(&mutex);
        caching_from_server(uri, cache_resp);
        V(&mutex);
    }

    /* close proxy connection with server */
    Close(clientfd);
    dbg_printf("--------In forwardRequest2Server function END--------\n");
}


/*
 * checkClientRequest - check the correctness of request from client
 *
 * Pamameters:
 *		fd:		file descriptor
 *	 method:	GET method
 *		url: 	parsed url
 *	 version: 	HTTP/1.0
 *		
 */
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
 *	
 * Usage:	1. Host header 
 *			2. User-Agent header
 *			3. Connection header
 *			4. Proxy-Connection header
 *					
 */
void createReqestHeader(char *buf, char *request2server, 
                                char *hostname) {
    dbg_printf("--------In createReqestHeader function--------\n");
    
    /* Host header */
    if (strcmp(buf, "Host")) {
        char host_str[MAXLINE];
        strcat(host_str, "Host: ");
        strcat(host_str, hostname);
        strcat(host_str, "\r\n");

        strcat(request2server, host_str);
    }
    /* User-Agent header */
    if (strcmp("User-Agent", buf)) {
        strcat(request2server, user_agent_hdr);
    }
    /* Connection header */
    if (strcmp("Connection", buf)) {
        strcat(request2server, "Connection: close\r\n");
    }
    /* Proxy-Connection header */
    if (strcmp("Proxy-Connection", buf)) {
        strcat(request2server, "Proxy-Connection: close\r\n");
    }

    strcat(request2server, "\r\n");
    dbg_printf("createReqestHeader: \n'%s'", request2server);

    dbg_printf("--------In createReqestHeader function END--------\n");
}


/*
 * parse_url - parse url into uri and CGI args
 *             only care about static request
 */
void parse_url(char *url, char *hostname, char *port) {

    dbg_printf("--------In parse_uri function --------\n");
    dbg_printf("request url is: %s\n", url);

    /* 
     * should handle following urls:
     * eg:  http://localhost:15213/home.html
     *      http://www.cmu.edu:8080/hub/index.html
     *      http://www.cmu.edu/hub/index.html
     *      http://www.cmu.edu
     */ 

    char *url_prt;  // url pointer 
    char *url_end;  // the end of url
    char port_tmp[MAXLINE]; // temp port 

    /* initialization */
    url_prt = Malloc(sizeof(char) * MAXLINE);
    strncpy(url_prt, url, MAXLINE);
    url_end = url_prt + strlen(url_prt);

    /* start parsing url */
    for (url_prt += 7; /* since "http://" has 7 characters */
            url_prt < url_end; url_prt++) {

        /* parse port if exist */
        if (*url_prt == ':') {  
            url_prt++; 
            
            while (url_prt < url_end && *url_prt != '/') {
                sprintf(port_tmp, "%s%c", port_tmp, *url_prt);
                dbg_printf("port_tmp is '%s' \n", port_tmp);
                url_prt++; 
            }

            strcpy(port, port_tmp);
            dbg_printf("request port is '%s' \n", port);
        }

        /* parse hostname */
        if (*url_prt == '/') {
        	strcat(hostname, "\0");
        	dbg_printf("request hostname is '%s' \n", hostname);
        	strcpy(url, url_prt);
        	dbg_printf("request uri is '%s'\n", url);
        	break;
        }
        /* continue loop */
        else { 
            sprintf(hostname, "%s%c", hostname, *url_prt);
        }
    }

    /* handle hostname or url not been updated */
    strcat(hostname, "");
    strcat(url, "");

    dbg_printf("request hostname is '%s' \n", hostname);
    dbg_printf("request uri is '%s'\n", url);
    dbg_printf("request port is '%s' \n", port);

    dbg_printf("--------In parse_uri function END--------\n");
}


/**********************************************************************
 ********************** Helper function from tiny.c *******************
 *********************************************************************/
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