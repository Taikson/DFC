
/* 
 * File:   main.c
 * Author: tycoon
 *
 * Created on 13 de enero de 2018, 2:08
 */


//#define _POSIX_C_SOURCE 199309L

#include <unistd.h>

#include "deltatime.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <stdint.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include "../../IMU/iio/src/80211beacons.h"


#include <pthread.h>

#include "webui.h"

/*
 * 
 */
#define BYTES_PER_CHANNEL 0x01
#define RAW_PROTOCOL_CHAR 0x10+BYTES_PER_CHANNEL

#define TIMEOUT_US 1000


#define JOY_PATH "/dev/input/js0"


#define DEFAULT_UDP_IP "127.0.0.1"
#define DEFAULT_UDP_PORT 5000


#define DEFAULT_HTTP_PORT 8080
#define DEFAULT_INITIAL_CHANNEL 13

uint8_t _current_channel=DEFAULT_INITIAL_CHANNEL;
uint8_t _requested_channel=DEFAULT_INITIAL_CHANNEL;
uint8_t _targeted_channel=DEFAULT_INITIAL_CHANNEL;

uint8_t _requested_wifi=0x02;


int8_t * _www_stream;


void target_to_channel(uint8_t change_to_channel){
    
    if(_requested_wifi!=0x01){
        return;
    }
    
    if((abs(_current_channel-change_to_channel)>1)){
        printf("\r\n[W]Illegal change on channel from %i to %i mode(%i)",_current_channel,change_to_channel,_requested_wifi);fflush(stdout);
        return;
    }
    if((change_to_channel!=_current_channel)&&(change_to_channel>0)&&(change_to_channel<=13)){
        if(set_channel_nl80211(change_to_channel)==0){
            printf("\r\n[I]Changed Channel from %i to %i",_current_channel,change_to_channel);
            _current_channel=change_to_channel;
            _requested_channel=change_to_channel;
        }
    }
    if((_targeted_channel!=_requested_channel)&&(_requested_channel==_current_channel)){
        if(_targeted_channel<_requested_channel){
            _requested_channel--;
        }else{
            _requested_channel++;
        }
    }else{
        //printf("\r\n Reached targeted channel %i ",_targeted_channel);fflush(stdout);
    }      


}
void parse_www_command(int8_t * command){
    int8_t * targetChannel="targetChannel=";
    int8_t * wifiMode="wifiMode=";
    int32_t val=0;
    if(command[0]==0x00){return;}

    if(strncmp(wifiMode, command, strlen(wifiMode)) == 0){
        val=atoi(&command[strlen(wifiMode)]);
        if((val>0)&&(val<=2)){
            _requested_wifi=val;
            if(_requested_wifi==2){
                _requested_channel=DEFAULT_INITIAL_CHANNEL;
                _targeted_channel=DEFAULT_INITIAL_CHANNEL;
                _current_channel=DEFAULT_INITIAL_CHANNEL;
                set_channel_nl80211(DEFAULT_INITIAL_CHANNEL);
            }
        }
    
    }
    if((strncmp(targetChannel, command, strlen(targetChannel)) == 0)&&(_requested_wifi==1)){
        val=atoi(&command[strlen(targetChannel)]);
        if((val>0)&&(val<=13)){
            _targeted_channel=val;
            reset_recv_seqnum();
   
        }
    
    }
    command[0]=0x00;
}
void *telemetry_recv(){
    uint8_t recv_command[5000];
    uint8_t str_websock[10000];
    uint8_t str_message[10000];
    int64_t seqnum=0;
    int16_t recv_len=0;
    uint8_t radiotap_channel=0;
    uint32_t www_stream_len=0;
    int8_t drone_wifi=0;
    
    if(init_www(DEFAULT_HTTP_PORT)==0){
        printf("\r\n[I]Started Web server on port %i",DEFAULT_HTTP_PORT);fflush(stdout);
    }else{
        printf("\r\n[W]Web server not started");fflush(stdout);
    }    
    
    
             
    //recv_www(www_stream,&www_stream_len);
    //send_www_channels(data_stream);
    
    
    
    while(1){
        memset(recv_command,0x00,sizeof(recv_command));
        memset(str_websock,0x00,sizeof(str_websock));
        memset(str_message,0x00,sizeof(str_message));
        recv_80211beacon(recv_command,&recv_len,&seqnum,&radiotap_channel);
        uint16_t idx=0;
        if((recv_len>11)&&(recv_command[0]==0x20)){
            recv_len=0;
            idx=1;
            
            int32_t pitch=(recv_command[idx++]<<24)|(recv_command[idx++]<<16)|(recv_command[idx++]<<8)|recv_command[idx++];
            int32_t roll =(recv_command[idx++]<<24)|(recv_command[idx++]<<16)|(recv_command[idx++]<<8)|recv_command[idx++];
            int32_t yaw  =(recv_command[idx++]<<24)|(recv_command[idx++]<<16)|(recv_command[idx++]<<8)|recv_command[idx++];
            
            int8_t  bperc=(recv_command[idx++]);
            int32_t bvolt=(recv_command[idx++]<<8)|(recv_command[idx++]);
            

            
            uint32_t mts[4];
            
            mts[0]=(recv_command[idx++]);
            mts[1]=(recv_command[idx++]);
            mts[2]=(recv_command[idx++]);
            mts[3]=(recv_command[idx++]);

            int8_t  change_to_channel=(recv_command[idx++]);
            
            int8_t drone_wifi=(recv_command[idx++]);
            
            int8_t pilot_gas=(recv_command[idx++]);
            int8_t pilot_pitch=(recv_command[idx++]);
            int8_t pilot_roll=(recv_command[idx++]);
            int8_t pilot_yaw=(recv_command[idx++]);
            
            target_to_channel(change_to_channel);
            
            base64_encode(&recv_command[idx],str_message);
            
            snprintf(str_websock,sizeof(str_websock),"{\"imu\":{\"pitch\":%i,\"roll\":%i,\"yaw\":%i}, \"adc\":[%i,0],\"motors\":[%i,%i,%i,%i],\"channels\":{\"current\":%i,\"requested\":%i, \"received\":%i},\"wifi\":%i, \"pilot\": {\"pitch\":%i,\"roll\":%i,\"yaw\":%i,\"gas\":%i,\"multiplier\":10}, \"seqnum\":%li ,\"message\":\"%s\" }",pitch,roll,yaw,bvolt,mts[0],mts[1],mts[2],mts[3], _current_channel,_requested_channel,change_to_channel,drone_wifi,pilot_pitch, pilot_roll, pilot_yaw, pilot_gas,seqnum,str_message);
            
            
            send_www(str_websock);
            
            if(_requested_channel==_current_channel){
                usleep(10000);
            }else{
                usleep(5000);
            }
            
            //printf("\n\r %s",str_websock); fflush(stdout);
            //printf("\r\n pitch %x %x %x %x",(recv_command[0]),(recv_command[1]),(recv_command[2]),recv_command[3]);
        }
        //if(_requested_channel==_current_channel){
            poll_www(TIMEOUT_MS_WWW);
            recv_www(_www_stream,&www_stream_len);
            _www_stream[www_stream_len]=0x00;
            parse_www_command(_www_stream);
        //}
    }
    
}




int main(int argc, char** argv) {
    uint8_t axes=0;
    printf("\r\n[I]Init Joystick control by @taiksontexas. v.0.2");fflush(stdout);

    

    char jpath[128]= JOY_PATH;
    uint16_t udp_port=DEFAULT_UDP_PORT;
    char ip[20]=DEFAULT_UDP_IP;
    uint32_t send_interval=TIMEOUT_US;

    
    char cmd=' ';
    while ((cmd = getopt (argc, argv, "j:p:d:i:w:")) != -1)
      switch (cmd)
        {
        case 'j':
            printf("\r\n[I]Changed joystick path to %s",optarg);
            sprintf(jpath,"%s",optarg);
          break;
        case 'p':
            
            udp_port=atoi(optarg);
            printf("\r\n[I]Changed destination port to %i",udp_port);
          break;
        case 'd':
            sprintf(ip,"%s",optarg);
            printf("\r\n[I]Changed destination IP to %s",ip);
          break;
        case 'i':
            send_interval=atoi(optarg);
            printf("\r\n[I]Changed send interval to %i us",send_interval);
          break; 
/*        case 'w':  
            http_port=atoi(optarg);
            printf("\r\n[I]Changed http port to %i ",http_port);
          break;*/  
        default:
            printf("\r\njoystick [-j joypath][-p port][-d ip][ -i interval(us)]");
          abort ();

    }
    
    
    uint16_t sock;
   
    struct hostent *hp;
    struct sockaddr_in server;

    sock= socket(AF_INET, SOCK_DGRAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(udp_port);
    hp = gethostbyname(ip);
    bcopy((char *)hp->h_addr_list[0], (char *)&server.sin_addr,hp->h_length);
    
    pthread_t telemetry_pthread;
    
    uint32_t f_joy=open(jpath, O_RDONLY);
    struct js_event jsdata; 
    
    uint8_t data_stream[2046];

    

 
    _www_stream=(int8_t *)calloc(1000,sizeof(int8_t));
    
    printf("\r\n[I]Opened %s",jpath);fflush(stdout);
    if(f_joy<=0){
        printf("\r\n[E]No %s found",jpath);fflush(stdout);
        exit(EXIT_FAILURE);
    }
    printf("\r\n[I]Querying number of axes");fflush(stdout);
    ioctl( f_joy, JSIOCGAXES, &axes );
    
    if (axes==0){
        printf("\r\n[E]No axes found.");fflush(stdout);
        exit(EXIT_FAILURE);
    }else{
        printf("\r\n[I]Found %i axes",axes);fflush(stdout);
    }

    data_stream[0]=RAW_PROTOCOL_CHAR;
    data_stream[1]=axes;
    
    fd_set read_fds;

    FD_ZERO(&read_fds);
    FD_SET(f_joy, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = send_interval;    
    

    printf("\r\n[I]Starting capture..\r\n\r\n");fflush(stdout);
    
    deltatime_init();

    uint64_t nselapsed=0;
    uint64_t seqnum=0;
    int16_t got=0;
    uint16_t sel=0;
  
    uint16_t data_len=0;
    

    init_80211beacons(BEACONS_INTERCEPTOR_PILOT);
    
    if(pthread_create(&telemetry_pthread, NULL,telemetry_recv , NULL)) {

        fprintf(stderr, "\r\n[E]Error creating thread\n");
        exit(-1);

    }    
    _requested_channel=_current_channel;
    _targeted_channel=_current_channel;
    
  
    
    set_channel_nl80211(_current_channel);

    while (1) {
        
        if (sel=select(f_joy + 1, &read_fds, NULL, NULL, &timeout) == 1){
            got=read (f_joy, &jsdata, sizeof(struct js_event));
            if(got<=0){
                printf("\r\n[E]Joystick disconnected, exiting");fflush(stdout);
                usleep(100000);
                exit(EXIT_FAILURE);
            }else{
                //printf("\r\n Axis %i Val %i type %i got %i",jsdata.number,jsdata.value,jsdata.type,got);fflush(stdout);
                data_stream[(jsdata.number*BYTES_PER_CHANNEL)+2]=(jsdata.value>>8)&0x00FF; //truncated to 1 byte resolution
                if(BYTES_PER_CHANNEL==2){
                    data_stream[(jsdata.number*BYTES_PER_CHANNEL)+3]=(jsdata.value)&0x00FF;
                }    
            }
        } 

        timeout.tv_sec = 0;
        timeout.tv_usec = send_interval;


        FD_ZERO(&read_fds); 
        FD_SET(f_joy, &read_fds);  
        
        nselapsed += deltatime_get_nanoseconds();

        data_len=(axes*BYTES_PER_CHANNEL)+1;
        
        if(BYTES_PER_CHANNEL==2){
            data_stream[data_len-1]=_requested_channel;
            data_len++;
        }
        data_stream[data_len-1]=_requested_channel;
        data_len++;
        
        if(BYTES_PER_CHANNEL==2){
            data_stream[data_len-1]=_requested_wifi;
            data_len++;
        }
        data_stream[data_len-1]=_requested_wifi;        
        
        
        data_len+=2;
        
        //print_vector(data_stream,data_len);

        if((nselapsed)>send_interval){
            nselapsed=0;
            send_80211beacon("PILOTO", data_stream, data_len);
            seqnum++;   
             //printf("\r[I]Seq num: %lu    ",seqnum);fflush(stdout);
        }
    }
    return (EXIT_SUCCESS);
}

