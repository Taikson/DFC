#include "udpserver.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdint.h>
#include <strings.h>



extern uint32_t init_udp_sock(uint16_t port){
	
   uint32_t sock, length;

   struct sockaddr_in server;


   sock=socket(AF_INET, SOCK_DGRAM, 0);
   length = sizeof(server);
   bzero(&server,length);
   server.sin_family=AF_INET;
   server.sin_addr.s_addr=INADDR_ANY;
   server.sin_port=htons(port);
   bind(sock,(struct sockaddr *)&server,length);
   //fromlen = sizeof(struct sockaddr_in);

   //printf("#Cmd:Esperando conexiones. Socket %i ",sock);
   return sock;

   
}

extern int32_t recv_udp(uint32_t sock,uint8_t *buffer){
    int32_t len=recv(sock, buffer, UDP_MEM_LEN, MSG_DONTWAIT);
    if(len<=0){buffer[0]=0x00;}
    return len;
}


extern uint8_t parse_raw_protocol_0x11_0x12(uint8_t * data, int16_t raw_channels[20] ){
    uint8_t protocol=data[0];
    
    if((protocol!=0x11)&&(protocol!=0x12)){return 0;}
    data[0]=0x00;
    if(data[1]<4){return -1;}

    uint8_t ac=0;
    
    for(ac=0;ac<data[1];ac++){
        if(protocol==0x12){//2 bytes per channel
            raw_channels[ac]=(int16_t)((int16_t)(data[(ac*2)+2]<<8)|(data[(ac*2)+3]));
        }else{
            raw_channels[ac]=(int16_t)((int16_t)(data[(ac)+2])<<8);
        }
    
    }    
    return 1;
}