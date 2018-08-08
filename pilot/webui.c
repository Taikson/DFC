#include "webui.h"
#include <signal.h>
#include <stdint.h>
#include "../mongoose/mongoose.h"


#define HTML_LINE_LEN 30000
#define LS_OUTPUT_LEN 60000
#define PORT_CHAR_LEN 6
#define MAX_REQ_POOL  300



struct mg_mgr mgr;
struct mg_connection *nc; 
struct sockaddr_in sockAddr;
int8_t s_http_port[PORT_CHAR_LEN] = "65500";
int8_t root_document[LS_OUTPUT_LEN] = "/home/tycoon/TCC-FILES/Interceptor/www_interceptor/.";

static struct mg_serve_http_opts s_http_server_opts;  

struct websocket_message websocket_msg;

uint16_t ws_keepalive=0;
uint16_t request_files=0;
uint16_t connected=0;
uint16_t failed=0;



#include <stdint.h>
#include <stdlib.h>


static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int mod_table[] = {0, 2, 1};

static int is_websocket(const struct mg_connection *nc) {
  return nc->flags & MG_F_IS_WEBSOCKET;
}

static void ws_broadcast(struct mg_mgr *m, char * buf) {
  struct mg_connection *c;
  for (c = mg_next(m, NULL); c != NULL; c = mg_next(m, c)) {
    if (c->flags & MG_F_IS_WEBSOCKET) {
      mg_printf_websocket_frame(c, WEBSOCKET_OP_TEXT, "%s", buf);
    }
  }
}

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
   
  switch (ev) {
      case MG_EV_HTTP_REQUEST:{          
        struct http_message *hm = (struct http_message *) p;
        char uri[HTML_LINE_LEN]="";
        sprintf(uri, "%.*s",(uint16_t)hm->uri.len,hm->uri.p);
        request_files=1000;
        
        mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
        break;   
      }
      case MG_EV_WEBSOCKET_FRAME:{
          ws_keepalive=300;
          struct websocket_message * ws=(struct websocket_message *) p;
          realloc(websocket_msg.data, ws->size);
          memcpy(websocket_msg.data, ws->data, ws->size);
          
          websocket_msg.size=ws->size;
          break;
      }
      
      case MG_EV_HTTP_CHUNK:{
          request_files=1000;
          break;
      }
      case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
      /* New websocket connection.*/
          ws_keepalive=300;
          //fprintf(stderr, "\r\n ws request");
        
      break;
    }
  }
}


extern int8_t init_www(uint16_t port){

    if (port==0){
        return -1;
    }
    
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN); 

    mg_mgr_init(&mgr, NULL);

    sprintf(s_http_port,"%i",port);    
    nc = mg_bind(&mgr, s_http_port, ev_handler);
    
    if (nc == NULL) {
        return -1;
    }
    
    websocket_msg.data=(int8_t *)calloc(HTML_LINE_LEN, sizeof(int8_t));
    websocket_msg.size=0;
    
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = root_document;  // Serve current directory
    s_http_server_opts.enable_directory_listing = "yes";
 
    return 0;
}
extern void *base64_encode(const uint8_t *in, uint8_t *encoded_data ) {

    size_t input_length=strlen(in);
    size_t output_length=4 * ((input_length + 2) / 3);


    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)in[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)in[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)in[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';


}


extern void poll_www(uint16_t poll_time){
    mg_mgr_poll(&mgr, poll_time);

}

extern uint16_t recv_www(int8_t * poll_stream,uint16_t * poll_stream_len){
    *poll_stream_len=websocket_msg.size;
    websocket_msg.size=0;
    
    if(*poll_stream_len>0){
        realloc(poll_stream,*poll_stream_len*sizeof(int8_t));
        memcpy(poll_stream,(int8_t *)websocket_msg.data,*poll_stream_len);
    }
    return *poll_stream_len;
}
extern void send_www(int8_t * ws_stream){    
    ws_broadcast(&mgr,ws_stream);
}


extern uint8_t send_www_channels(uint8_t * data  ){
    uint8_t protocol=data[0];
    int16_t raw=0;
    int8_t str_ws[1000]="{\"channels\":[";
    if((protocol!=0x11)&&(protocol!=0x12)){return 0;}
    
    if(data[1]<4){return -1;}

    uint8_t ac=0;
    
    for(ac=0;ac<data[1];ac++){
        if(protocol==0x12){//2 bytes per channel
            raw=(int16_t)((int16_t)(data[(ac*2)+2]<<8)|(data[(ac*2)+3]));
        }else{
            raw=(int16_t)((int16_t)(data[(ac)+2])<<8);
        }
        sprintf(str_ws,"%s%i",str_ws,raw);
        if(ac<(data[1]-1)){
            sprintf(str_ws,"%s,",str_ws);
        }
    
    }    
    sprintf(str_ws,"%s]}",str_ws);
    //printf("\r\n %s", str_ws);fflush(stdout);
    send_www(str_ws);
    return 1;
}

extern void close_www(){
    mg_mgr_free(&mgr);
}