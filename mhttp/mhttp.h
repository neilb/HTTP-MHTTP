/*
 * copied and amended from a simple http socket program:
 * "Simple Internet client program - by Dan Drown <abob@linux.com>"
 * available (last I looked) at: 
 *      http://linux.omnipotent.net/article.php?article_id=5424
 */

#include <sys/types.h>  /* for socket,connect */
#include <sys/socket.h> /* for socket,connect */
#include <netinet/in.h> /* for htons */
#include <netdb.h>      /* for gethostbyname */
#include <string.h>     /* for memcpy */
#include <stdio.h>      /* for perror */
#include <stdlib.h>     /* for exit */
#include <unistd.h>     /* for read,write */
#include <stdarg.h>     /* needed for argument processing in debug */
#include <time.h>       /* needed for getting timestamp in debug */

// how big is everything
#define MAX_STR 1025
#define MAX_BUFFERS 1025
#define MAX_HEADERS 257

// fake up a definition of bool if it doesnt exist
#ifndef bool
typedef unsigned char    bool;
#endif

// create my true and false
#ifndef false
typedef enum { false, true } mybool;
#endif

int mhttp_build_inet_addr(struct sockaddr_in *addr, const char *hostname, unsigned short int port);

int mhttp_connect_inet_addr(const char *hostname, unsigned short int port);

void mhttp_switch_debug(int set);

void mhttp_reset(void);

void mhttp_init(void);

void mhttp_add_header(char *hdr);

void mhttp_set_protocol(int proto);

void mhttp_set_body(char *bdy);

char *mhttp_get_response_headers(void);

char *mhttp_get_reason(void);

char *mhttp_get_response(void);

int mhttp_call(char *paction, char *purl);

int mhttp_get_status_code(void);

int mhttp_get_response_length(void);

void mhttp_debug(const char *msgfmt, ...);

