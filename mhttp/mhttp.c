/*
 * copied and amended from a simple http socket program:
 * "Simple Internet client program - by Dan Drown <abob@linux.com>"
 * available (last I looked) at: 
 *      http://linux.omnipotent.net/article.php?article_id=5424
 */

#include "mhttp.h"

#ifdef DOHERROR
void herror(char * str);

void herror(char *str){
  fprintf(stderr, "herror: %s\n", str);
}
#endif

bool mhttp_lets_debug;        /* global debugging flag           */
bool mhttp_body_set_flag;     /* global body set flag            */

int  mhttp_hcnt,
     mhttp_rcode,
     mhttp_response_length;

char *mhttp_body,
     *mhttp_response,
     *mhttp_reason,
     mhttp_resp_headers[MAX_STR],
     *mhttp_headers[MAX_HEADERS],
     *mhttp_buffers[MAX_BUFFERS];

char *mhttp_last_host = NULL;
int mhttp_last_port = 0;
int mhttp_last_socket = 0;
int mhttp_protocol = 0;


void mhttp_switch_debug(int set)
{
    if (set > 0)
    {
        mhttp_lets_debug = true;
        mhttp_debug("%s", "switched on debugging...");
    } else {
        mhttp_lets_debug = false;
    }

}


void mhttp_set_protocol(int proto)
{

     mhttp_protocol = proto;

}


int mhttp_get_status_code(void)
{

    return mhttp_rcode;

}


int mhttp_get_response_length(void)
{

    return mhttp_response_length;

}


char *mhttp_get_reason(void)
{

  if (mhttp_reason != NULL){
    return strdup(mhttp_reason);
  } else {
    return NULL;
  }

}


char *mhttp_get_response(void)
{

    return mhttp_response;

}


char *mhttp_get_response_headers(void)
{

    return strdup(mhttp_resp_headers);

}


void mhttp_reset(void)
{

  int i;

  if (mhttp_response != NULL)
  {
      free(mhttp_response);
      mhttp_response = NULL;
      mhttp_debug("reset the response");
  }
  mhttp_response_length = 0;
  if (mhttp_reason != NULL)
  {
      free(mhttp_reason);
      mhttp_reason = NULL;
      mhttp_debug("reset the reason");
  }
  if (mhttp_body_set_flag)
      free(mhttp_body);
  mhttp_body_set_flag = false;
  mhttp_rcode = 0;
  mhttp_debug("finished reset");
}


void mhttp_init(void)
{

  int i;

  for (i = 0; i < mhttp_hcnt; i++)
  {
      free(mhttp_headers[i]);
      mhttp_debug("freeing header");
      mhttp_headers[i] = NULL;
  }
  mhttp_hcnt = 0;
  mhttp_lets_debug = false;
  mhttp_protocol = 0;
  mhttp_reset();
  mhttp_debug("finished init");
}


void mhttp_add_header(char *hdr)
{

    mhttp_headers[mhttp_hcnt++] = strdup(hdr);
    mhttp_debug("request header %s", mhttp_headers[mhttp_hcnt - 1]);
    mhttp_headers[mhttp_hcnt] = NULL;

}


void mhttp_set_body(char *bdy)
{

    mhttp_body = strdup(bdy);
    mhttp_debug("setting body: %s", mhttp_body);
    mhttp_body_set_flag = true;

}


int mhttp_call(char *paction, char *purl)
{
    bool found_hdrs;        /* found the end of headers flag   */
    bool rcode_flag;        /* found return code flag          */
    bool newcon_flag;       /* new connection flag             */
    bool chunked;           /* transfer encoding is chunked    */
    char *action, *url, *host, *ptr, *eomsg, *clptr;
    int  port,
         socket_descriptor,
         returnval,
	 len,
	 curr_len,
	 buffer_size,
	 this_chunk,
	 i,
	 pos;
    char str[MAX_STR],
         surl[MAX_STR];


    memset(mhttp_resp_headers, 0, MAX_STR);

    if(strlen(paction) == 0)
    {
        mhttp_debug("must supply an action");
        return -2;
    }
    action = strdup(paction);

    if(strlen(purl) == 0)
    {
        mhttp_debug("must supply a url");
        return -3;
    }
    url = strdup(purl);

    mhttp_debug("action: %s url: %s", action, url);

    if (strcmp(action, "GET") != 0 && 
        strcmp(action, "POST") != 0 && 
        strcmp(action, "PUT") != 0 &&
        strcmp(action, "DELETE") != 0 &&
        strcmp(action, "HEAD") != 0)
    {
        mhttp_debug("must supply an action of GET, PUT, POST, DELETE, or HEAD");
        return -1;
    }

    mhttp_debug("action is: %s", action);
    mhttp_debug("url is: %s", url);

    // make sure that the url starts with http://
    if (strncmp(url, "http://", 7) != 0){
        mhttp_debug("url must start with http:// - yep we dont support https\n");
        return -4;
    }

    host = url+7;

    memset(surl,0,sizeof(surl));

  // hunt for the beginning of the uri
  ptr = strstr(host, "/");
  if (ptr != NULL){
      *ptr = '\0';
      ptr++;
      sprintf(surl, "/%s", ptr);
  } else {
      sprintf(surl, "/");
  }
  mhttp_debug("hostname is: %s", host);
  mhttp_debug("uri is: %s\n", surl);

  // hunt for the beginning of the port
  ptr = strstr(host, ":");
  if (ptr != NULL){
      *ptr = '\0';
      ptr++;
      port = atoi(ptr);
      mhttp_debug("detected port: %d", port);
  } else {
      port = 80;
  }
  mhttp_debug("port number is: %d", port);

  newcon_flag = false;
  //hostandsocket = malloc(128);
  if (mhttp_last_host != NULL &&
     strcmp(host, mhttp_last_host) == 0 &&
     mhttp_last_port == port){
     socket_descriptor = mhttp_last_socket;
     mhttp_debug("using the cached connection");
  } else {
    if (mhttp_last_host != NULL){
      close(mhttp_last_socket);
      mhttp_last_socket = 0;
      mhttp_last_port = 0;
      free(mhttp_last_host);
    }
    newcon_flag = true;
    socket_descriptor = -1;
    mhttp_debug("didnt find connection - creating a new one: %s/%d - %d ", host, port, socket_descriptor);
    if( (socket_descriptor = mhttp_connect_inet_addr(host, port)) < 0) {
         //mhttp_debug("could not create a new socket");
         mhttp_debug("could not create a new socket: %d", socket_descriptor);
         return socket_descriptor;
     }
  }

  mhttp_debug("socket descriptor is: %d ", socket_descriptor);

  //socket_descriptor = mhttp_connect_inet_addr(host, port);

  len = 0;

  // construct the query string
  memset(str, 0, sizeof(str));
  strcpy(str, action);
  //strcpy(str+strlen(str), " http://localhost");
  strcpy(str+strlen(str), " ");
  strcpy(str+strlen(str), surl);
  //strcpy(str+strlen(str), " HTTP/1.1\r\n");
  sprintf(str+strlen(str), " HTTP/1.%d\r\n", mhttp_protocol);
  mhttp_debug("adding on the headers: %d", mhttp_hcnt);
  for(i = 0; i < mhttp_hcnt; i++)
  {
      // make sure that we dont exceed the buffer
      if ((strlen(str) + strlen(mhttp_headers[i])) > MAX_BUFFERS - 1)
          break;
      mhttp_debug("adding header: %s", mhttp_headers[i]);
      sprintf(str+strlen(str), "%s\r\n", mhttp_headers[i]);
  }
  // if this is a post - add the Content-Length header
  if (mhttp_body_set_flag)
  {
      sprintf(str+strlen(str), 
              "Content-Length: %d\r\n\r\n", strlen(mhttp_body));
  } else {
      strcpy(str+strlen(str), "\r\n\r\n");
  }
  mhttp_debug("query string + headers are: %s", str);

  returnval = write(socket_descriptor, str, strlen(str));
  if(returnval < 0)
  {
    /* write returns -1 on error */
    perror("write(query string) error");
    return -5;
  }

  if(returnval < strlen(str))
  {
    /* I'm not dealing with this error, regular programs should. */
    perror("the query string write was short\n");
    return -6;
  }

  // if this is a PUT or POST - write the body
  if (mhttp_body_set_flag)
  {
      // XXX we probably have a problem where 1024 is exceeded and need to rewrite
      mhttp_debug("this is a %s.... writing the body...", action);
      mhttp_debug("writing data: %s", mhttp_body);
      returnval = write(socket_descriptor, mhttp_body, strlen(mhttp_body));
      if(returnval < strlen(mhttp_body))
      {
          /* I'm not dealing with this error, regular programs should. */
          fprintf(stderr, "the write of %s data was short\n", action);
          return -6;
      }
      returnval = write(socket_descriptor, "\r\n", 2);
      if(returnval < 2)
      {
          /* I'm not dealing with this error, regular programs should. */
          fprintf(stderr, "the write of %s data - last line failed\n", action);
          return -7;
      }
  }


  /* read off the response and split out headers and content */
  mhttp_debug("starting output:");
  found_hdrs = false;
  rcode_flag = false;
  len = 0;
  curr_len = 0;
  chunked = false;

  while((returnval = read(socket_descriptor, str, 80)) > 0)
  {
       *(str+returnval) = '\0';
       //mhttp_debug("Line %d: %s", returnval, str);

      /* detect the end of the headers */
      if (!found_hdrs){
	  sprintf(mhttp_resp_headers+strlen(mhttp_resp_headers), "%s", str);
          if ((ptr = strstr(mhttp_resp_headers, "\r\n\r\n")) ||
	      (ptr = strstr(mhttp_resp_headers, "\n\n"))){
              found_hdrs = true;
	      *ptr = '\0';
	      mhttp_debug("found end of headers at: %d", strlen(mhttp_resp_headers));
	      mhttp_debug("headers are: %s", mhttp_resp_headers);
	      if (strncmp(ptr,"\0\n\r\n",4) == 0){
	          // how far along the current buffer is the eoh marker
                  curr_len = (strlen(mhttp_resp_headers) + 4) - curr_len;
                  ptr+=4;
	      } else {
                  curr_len = (strlen(mhttp_resp_headers) + 2) - curr_len;
                  ptr+=2;
	      }

	      // determine the Content-Length header
              if ((clptr = strstr(mhttp_resp_headers, "Content-Length:")) ||
                  (clptr = strstr(mhttp_resp_headers, "Content-length:"))){
		  mhttp_debug("found content-length");
		  clptr += 16;
                  mhttp_response_length = atoi(clptr);
                  mhttp_debug("content length: %d", mhttp_response_length);
                  mhttp_response = malloc(mhttp_response_length + 2);
	          if (mhttp_response_length >= (returnval - curr_len)){
		      mhttp_debug("copying the initial part of the body");
                      memcpy(mhttp_response, ptr, returnval - curr_len);
		      len = returnval - curr_len;
                  } else {
		      // serious error - cant determine length properly
		      mhttp_debug("serious error - cant determine length properly");
	              len = 0;
                      return -8;
	          }
	      // look for Transfer-Encoding: chunked
              } else if ((clptr = strstr(mhttp_resp_headers, "Transfer-Encoding:")) ||
                         (clptr = strstr(mhttp_resp_headers, "Transfer-encoding:"))){
		  clptr += 19;
		  if (strncmp(clptr, "chunked",7) == 0){
		    mhttp_debug("found Transfer-Encoding: chunked");
		    chunked = true;
		    // must find the end of the chunked length line 
		    // in the remainding buffer
                    if (clptr = strstr(ptr, "\r\n")){
		         if (sscanf(ptr, "%x\r\n", &this_chunk) != 1){
		             mhttp_debug("count not the chunked first amount - something broken");
			     break;
			 }
			 // shift past the chunk size
			 *clptr = '\0';
			 curr_len += strlen(ptr) + 2;
			 clptr += 2;
		         mhttp_debug("Transfer-Encoding: chunked first buffer is %d - %d bytes left", this_chunk, returnval - curr_len);
                         mhttp_response = malloc(this_chunk + 2);
                         memcpy(mhttp_response, clptr, returnval - curr_len);
                         len += returnval - curr_len;
		         buffer_size = this_chunk + 2;
			 ptr = clptr;
		    } else {
		      // give up? XXX - not enuf string left to find chunk in buffer?
		         // get another line and try again
		         mhttp_debug("getting another line");
                         i = returnval;
                         if ((returnval = read(socket_descriptor, str+returnval, 80)) > 0){
			     returnval += i;
                             *(str+returnval) = '\0';
			     mhttp_debug("looking for first chunk at: %s", ptr);
                             if (clptr = strstr(ptr, "\r\n")){
		                 if (sscanf(ptr, "%x\r\n", &this_chunk) != 1){
			             // cant find the next chunk
			             mhttp_debug("could not find first chunk - something broken");
			             break;
			         } else {
			             // sort out the next chunk
				     if (this_chunk == 0){
				         // final chunk
				         mhttp_debug("the first chunk is the final chunk");
				         break;
				     } else {
			                 // shift past the chunk size
			                 *clptr = '\0';
			                 curr_len += strlen(ptr) + 2;
			                 clptr += 2;
		                         mhttp_debug("Transfer-Encoding: chunked first buffer is %d - %d bytes left", this_chunk, returnval - curr_len);
                                         mhttp_response = malloc(this_chunk + 2);
                                         memcpy(mhttp_response, clptr, returnval - curr_len);
                                         len += returnval - curr_len;
		                         buffer_size = this_chunk + 2;
			                 ptr = clptr;
				     }
			        }
			    } else {
			       mhttp_debug("cannot find \\r\\n after first chunked marker - time to give up");
			       break;
			    }
		        } else {
			    // XXX no more data to read
			}
		    }
		  }
              } else {
		  mhttp_debug("didnt find content-length - must use realloc");
                  mhttp_response_length = 0;
                  mhttp_response = malloc(MAX_STR);
                  memcpy(mhttp_response, ptr, returnval - curr_len);
                  len += returnval - curr_len;
		  buffer_size = MAX_STR;
	      }
          } else {
	      curr_len += 80;
	  }
      } else {
          // we have the headers - this is the body
          if (mhttp_response_length > 0){
              // make sure that it does not overflow the buffer
              if (mhttp_response_length >= (len + returnval)){
                  memcpy(mhttp_response+len, str, returnval);
              }
	  } else {
	      // we dont know how big it should be so compensate
              //mhttp_debug("buffer remaining is: %d", buffer_size - len);
	      if (chunked){
	          if (len + returnval >= buffer_size - 2){
		     mhttp_debug("reached end of buffer - find another buffer? #%s", str+((buffer_size - 2) - len));
		     mhttp_debug("remainder of line is: %d - trimming is: %d", (len + returnval) - (buffer_size - 2), (buffer_size - 2) - len);
                     memcpy(mhttp_response+len, str, (buffer_size - 2) - len);
		     pos = (buffer_size - 2) - len;
		     len += pos;

		     // find the next chunk
		     mhttp_debug("looking for the next chunk");
		     if (sscanf(str+pos, "\r\n%d\r\n", &this_chunk) != 1){
		         // get another line and try again
		         mhttp_debug("getting another line");
                         i = returnval;
                         if ((returnval = read(socket_descriptor, str+returnval, 80)) > 0){
			     returnval += i;
                             *(str+returnval) = '\0';
			     mhttp_debug("looking for last chunk at: %s", str+pos);
		             if (sscanf(str+pos, "\r\n%x\r\n", &this_chunk) != 1){
			         // cant find the next chunk
			         mhttp_debug("could not find next chunk and we are out of data");
			         break;
			     } else {
			         // sort out the next chunk
				 if (this_chunk == 0){
				     // final chunk
				     mhttp_debug("we found the final chunk");
				     break;
				 } else {
				     // resize and paste on remainder
			             mhttp_debug("resize and paste on remainder for next chunk processing");
	                             mhttp_response = realloc(mhttp_response, buffer_size + this_chunk);
		                     buffer_size += this_chunk;
				     if (clptr = strstr(str+(pos + 5),"\n")){
				         *clptr = '\0';
				         clptr += 1;
					 i = strlen(str+pos);
					 i += 1 + pos;
                                         memcpy(mhttp_response+len, clptr, returnval - i);
				     } else {
				         mhttp_debug("did not find end of line of new chunk value");
					 break;
				     }
				 }
			    }
                         } else {
		             // bad problem with trying to get a final line
			     mhttp_debug("read for final line failed");
		             break;
		         }
                     } else {
		       // allready have the chunk 
		       mhttp_debug("found the next chunk marker");
			   if (this_chunk == 0){
			       // final chunk
			       mhttp_debug("this is the last chunk");
			       break;
			   } else {
			       // resize and paste on remainder
			       mhttp_debug("resize and paste on remainder for next chunk processing");
	                       mhttp_response = realloc(mhttp_response, buffer_size + this_chunk);
		               buffer_size += this_chunk;
			      if (clptr = strstr(str+(pos + 5),"\n")){
			          *clptr = '\0';
			          clptr += 1;
				  i = strlen(str+pos);
				  i += 1 + pos;
                                  memcpy(mhttp_response+len, clptr, returnval - i);
			      } else {
			          mhttp_debug("did not find end of line of new chunk value");
				  break;
			      }
			  }
		     }
	          } else {
		    // not end of chunk yet
                    memcpy(mhttp_response+len, str, returnval);
		  }
	      } else {
	          // else this is not a chunked read - so realloc when necessary
	          if (len + returnval > buffer_size){
	             mhttp_response = realloc(mhttp_response, buffer_size + MAX_STR);
		     buffer_size += MAX_STR;
	          }
                  memcpy(mhttp_response+len, str, returnval);
	      }
	  }
          len += returnval;
      }

      /* find the return code        */
      if (!rcode_flag &&
          strncmp(str, "HTTP/",5) == 0 && 
          (strncmp(str+5, "0.9 ",4) == 0 ||
	   strncmp(str+5, "1.0 ",4) == 0 ||
	   strncmp(str+5, "1.1 ",4) == 0 ) ){
          ptr = str+9;
	  *(ptr+3) = '\0';
          mhttp_rcode = atoi(ptr);
	  rcode_flag = true;
	  ptr+=4;
	  /* find the status reason */
	  //eomsg = NULL;
          if ((eomsg = strstr(ptr, "\r\n")) || (eomsg = strstr(ptr, "\n"))){
	      mhttp_debug("found end of reason line");
	      *eomsg = '\0';
	      mhttp_reason = strdup(ptr);
	  }
          mhttp_debug("detected return code: %d - %s", mhttp_rcode, mhttp_reason);
      }

      // lets get out of here if we have read enough
      if (mhttp_response_length > 0 && len >= mhttp_response_length )
          break;

      //if (returnval < 80)
      //    break;

  }
  //mhttp_debug("last line: %s ", str);
  mhttp_debug("content length actually copied: %d (may include \\r\\n)", len);
  mhttp_response_length = len;

  /* it will be closed anyway when we exit */
  //close(socket_descriptor);
  if (mhttp_protocol == 0 ||
      (clptr = strstr(mhttp_resp_headers, "Connection: close")) ||
      (clptr = strstr(mhttp_resp_headers, "Connection: Close")) ){
      close(socket_descriptor);
      if (mhttp_last_host != NULL){
          mhttp_last_socket = 0;
          mhttp_last_port = 0;
          free(mhttp_last_host);
          mhttp_debug("removed socket name");
      }
      mhttp_debug("Closed the connection");
  } else {
      if (newcon_flag){
          mhttp_last_socket = socket_descriptor;
          mhttp_last_port = port;
          mhttp_last_host = strdup(host);
          mhttp_debug("Caching the connection");
      } else {
          mhttp_debug("connection allready cached");
      }
  }

  mhttp_debug("all done");
  /* just to be pedantic... */
  return 1;
}


int mhttp_connect_inet_addr(const char *hostname, unsigned short int port)
{
  int inet_socket; /* socket descriptor */
  struct sockaddr_in inet_address; /* IP/port of the remote host to connect to */

  if ( mhttp_build_inet_addr(&inet_address, hostname, port) < 0 )
      return -1;

  /* socket(domain, type, protocol) */
  inet_socket = socket(PF_INET, SOCK_STREAM, 0);

  mhttp_debug("socket no: %d", inet_socket);
  /* domain is PF_INET(internet/IPv4 domain) *
   * type is SOCK_STREAM(tcp) *
   * protocol is 0(only one SOCK_STREAM type in the PF_INET domain
   */

  if (inet_socket < 0)
  {
    /* socket returns -1 on error */
    perror("socket(PF_INET, SOCK_STREAM, 0) error");
    mhttp_debug("socket(PF_INET, SOCK_STREAM, 0) error");
    //exit(5);
    return -2;
  }

  /* connect(sockfd, serv_addr, addrlen) */
  if(connect(inet_socket, (struct sockaddr *)&inet_address, sizeof(struct sockaddr_in)) < 0)
  {
    /* connect returns -1 on error */
    perror("connect(...) error");
    mhttp_debug("connect(...) error");
    //exit(6);
    return -3;
  }

  return inet_socket;
}


int mhttp_build_inet_addr(struct sockaddr_in *addr, const char *hostname, unsigned short int port)
{
  struct hostent *host_entry;

  /* gethostbyname(name) */
  host_entry = gethostbyname(hostname);

  if(host_entry == NULL)
  {
    /* gethostbyname returns NULL on error */
    herror("gethostbyname failed");
    mhttp_debug("gethostbyname failed");
    //exit(7);
    return -1;
  }

  /* memcpy(dest, src, length) */
  memcpy(&addr->sin_addr.s_addr, host_entry->h_addr_list[0], host_entry->h_length);
  /* copy the address to the sockaddr_in struct. */

  /* set the family type (PF_INET) */
  addr->sin_family = host_entry->h_addrtype;

  /* addr->sin_port = port won't work because they are different byte
   * orders
   */
  addr->sin_port = htons(port);

  /* just to be pedantic... */
  return 1;
}

/* debug logging */
void mhttp_debug(const char *msgfmt, ...)
{
    va_list ap;
    char *pos, message[MAX_STR];
    int sz;
    time_t t;

    if (!mhttp_lets_debug)
        return;

    /* timestamp */
    t = time(NULL);
    pos = ctime(&t);
    sz = strlen(pos);
    /* chop off the \n */
    pos[sz-1]='\0';

    /* insert the header */

    snprintf(message, MAX_STR, "mhttp debug:%s: ", pos);
        
    /* find the end and attach the rest of the msg */
    for (pos = message; *pos != '\0'; pos++); //empty statement
    sz = pos - message;
    va_start(ap, msgfmt);
    vsnprintf(pos, MAX_STR - sz, msgfmt, ap);
    fprintf(stderr,"%s", message);
    fprintf(stderr, "\n");
    fflush(stderr);
}

