#include "imu.h"
#include "gy953.h"
#include "adc.h"
#include "pid.h"
#include "pwm.h"
#include "udpserver.h"
#include "80211beacons.h"
#include "imu.h"
//#include "deltatime.h"
#include <stdint.h>
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>


#define UDP_PORT 5000
#define MSG_PORT 5001
#define IMU_PORT "/dev/ttyS1"

#define LPF_PITCH 0.1 //0.1
#define LPF_ROLL 0.1 //0.1
#define LPF_YAW 0.1 //0.1


#define KP_RATE_PITCH       793
#define KI_RATE_PITCH       42
#define KD_RATE_PITCH       10118
#define KIMAX_RATE_PITCH    165 

#define KP_ANGLE_PITCH      1

#define KP_RATE_ROLL        793
#define KI_RATE_ROLL        42
#define KD_RATE_ROLL        10118
#define KIMAX_RATE_ROLL     165

#define KP_ANGLE_ROLL       1

#define KP_RATE_YAW         2100
#define KI_RATE_YAW         0
#define KD_RATE_YAW         6100
#define KIMAX_RATE_YAW      400

#define KP_ANGLE_YAW        1




//R:roll=alabeo | P:pitch cabeceo |G:gas |'0': switch0|a: potentiometer0|Y:yaw

#define INITIAL_WIFI_CHAN 13
//GLOBAL
const char * const  __MAPPING_PILOT_CHANNELS= "RPG0aYCWB  "; //8channels
int8_t _poweron=0;
uint8_t _pseudo_hz_tick=0;//current tick PSEUDO_HZ

uint8_t _current_channel=INITIAL_WIFI_CHAN;
uint8_t _current_wifi=2;

struct euler_int_t * _imu;    
struct pilot_t * _pilot; 
struct internal_int_t * _internal;  
    
    
    
struct pid_int_t * _pid_pitch;
struct pid_int_t * _pid_roll;
struct pid_int_t * _pid_yaw;


struct pid_int_t * _pid_rate_pitch;
struct pid_int_t * _pid_rate_roll;
struct pid_int_t * _pid_rate_yaw; 



int8_t apply_wifi_status(){
    if(_poweron!=0){return -1;}
    
    if(_current_wifi==_pilot->requested_wifi){return -2;}
    
    uint8_t disabled_default_radio=0;
    

    if(_pilot->requested_wifi==1){
        disabled_default_radio=1;
    }else{
        disabled_default_radio=0;
    }
    _current_wifi=_pilot->requested_wifi;
    printf("\r\n[W]Going to apply disabled_default_radio= %i",disabled_default_radio);fflush(stdout);
    
    uint8_t syscmd[5000];
    //close_80211beacons();
    sleep(1);
    if(disabled_default_radio!=0){disabled_default_radio=1;}
    sprintf(syscmd,"uci set wireless.default_radio0.disabled='%i';wifi",disabled_default_radio);
    system(syscmd);
    sleep(10);
    printf("\r\n[W]Going to exit. Reboot me!");fflush(stdout);
    system("killall imu");//TODO FIXME
    exit(-1);

};

int64_t range_pilot_value(int64_t raw_val, int64_t offset, uint8_t max_value, uint16_t total_value ){
    int8_t sign=1;
    if(offset<0){sign= -1;}
    
    int64_t raw_scale=abs(offset*2);
    
    //return (sign*(raw_val));
    int64_t absolute=((sign*(raw_val))*MAX_PWM)/raw_scale;
    
    return absolute*max_value/total_value;
    
}



void parse_battery(struct internal_int_t * internal, int32_t voltage_cent){
    internal->battery_volt=voltage_cent;
    if(voltage_cent>=MIN_VDC){
        internal->battery_percent=0xFF;
    }else{
        internal->battery_percent=((voltage_cent-MIN_VBAT)*100)/MAX_VBAT;
    }            
    
}




void parse_pilot(struct pilot_t *pilot,int16_t * raw_channels,  int8_t  * mapping, int32_t half_range){
    uint8_t ac=0;
    uint8_t len=strlen((const char *)mapping);
    uint8_t new_channel=0;
    //mirar jscal en bash para calibrar el joystick
    for(ac=0; ac<len;ac++){
        switch(mapping[ac]){
            case 'P':
                pilot->pitch=range_pilot_value(raw_channels[ac],half_range, MAX_DEGREES_PILOT_PITCHROLL,359);
            break;
            case 'R':
                pilot->roll=range_pilot_value(raw_channels[ac],half_range, MAX_DEGREES_PILOT_PITCHROLL,359);
            break;
            case 'Y':
                pilot->yaw=range_pilot_value(raw_channels[ac],half_range, MAX_DEGREES_PILOT_YAW,359);
            break;
            case 'G':
                pilot->gas=range_pilot_value(raw_channels[ac]+half_range,half_range, MAX_PERCENT_PILOT_GAS,100);   
            break;
            case 'a':
                pilot->auxA=range_pilot_value(raw_channels[ac],half_range, 100,100);   
            break;        
            case '0':
                pilot->auxB=raw_channels[ac];
            break;
            case 'C':
                new_channel=(raw_channels[ac]>>8);
                
                if((new_channel>0)&&(new_channel<=13)){
                    pilot->requested_channel=(int8_t)new_channel;
                    //printf("\r\n Channel %i, curr %i ",new_channel,_current_channel);fflush(stdout);
                }
            break; 
            case 'W':
                pilot->requested_wifi=(raw_channels[ac]>>8);
            break;
 
            
            
        }
    
    }
    pilot->ticks_since_recv=0;
    

}

void apply_pilot(uint8_t * cmdrecvudp)
{
    int8_t parse_raws_res=0;

    int16_t raw_channels[20]={0,0,0,0,0,0,0,0,0,0,0,0,0};
    
    parse_raws_res+=parse_raw_protocol_0x11_0x12(cmdrecvudp, raw_channels);

    if(parse_raws_res>0){
        parse_pilot(_pilot,raw_channels,(int8_t *)__MAPPING_PILOT_CHANNELS,32767);
        if((_pilot->gas<MAX_POWERON_GAS)){
           
            if(_pilot->yaw>MIN_POWERON_YAW){
                printf("\r[I]Engines started          \r\n");fflush(stdout);
                _poweron=1;
            }
            if(_pilot->yaw<-MIN_POWERON_YAW){
                _poweron=0;
                printf("\r[I]Engines stopped          \r\n");fflush(stdout);
            }
        }else{
            //limitedYaw=pilot->yaw;
            //limitedYaw=0;
        }
        
        if(_pilot->requested_channel!=_current_channel){
            printf("\r\n[D] Requested change channel to %i",_pilot->requested_channel);fflush(stdout);
            if(set_channel_nl80211(_pilot->requested_channel)==0){
                _current_channel=_pilot->requested_channel;
                printf("\r\n[D] Changed channel to %i",_pilot->requested_channel);fflush(stdout);
            }
        }
        apply_wifi_status();
    } 
    if(cmdrecvudp[0]==PID_PROTOCOL_CONFIG){
        //printf("\r\n recv command ");fflush(stdout);
        switch(cmdrecvudp[1]){
            case 0x01:printf("\r\n rate pitch");fflush(stdout);parse_pid_config(_pid_rate_pitch, cmdrecvudp); break;
            case 0x02:printf("\r\n rate roll");fflush(stdout);parse_pid_config(_pid_rate_roll , cmdrecvudp); break;
            case 0x03:printf("\r\n rate yaw");fflush(stdout);parse_pid_config(_pid_rate_yaw , cmdrecvudp); break;
            

            case 0x04:printf("\r\napitch");fflush(stdout);parse_pid_config(_pid_pitch, cmdrecvudp); break;
            case 0x05:printf("\r\naroll");fflush(stdout);parse_pid_config(_pid_roll , cmdrecvudp); break;
            case 0x06:printf("\r\nayaw");fflush(stdout);parse_pid_config(_pid_yaw , cmdrecvudp); break;
        }

        cmdrecvudp[0]=0x00;
    }

}
void *pilot_recv(){
    int8_t recv_command[5000];
    int64_t seqnum=0;
    int32_t recv_len=0;
    uint8_t radiotap_channel=0;
    while(1){

        recv_80211beacon(recv_command,&recv_len,&seqnum,&radiotap_channel);
        //printf("\r\n radio %i",radiotap_channel);fflush(stdout);
        apply_pilot((uint8_t *)recv_command);
        usleep(10000);
    }    
}
void send_telemetry(int8_t * beacon, int8_t * message){
   int8_t stream[5000];
   uint32_t len=0;  
   
    
   
    stream[len] =0x20; len+=1; //protocol preamble
    
    stream[len] =(int8_t)(_imu->pitch>>24)&0x000000FF;len+=1;
    stream[len] =(int8_t)(_imu->pitch>>16)&0x000000FF;len+=1;
    stream[len] =(int8_t)(_imu->pitch>>8) &0x000000FF;len+=1;
    stream[len] =(int8_t)(_imu->pitch)    &0x000000FF;len+=1;

    stream[len] =(int8_t)(_imu->roll>>24) &0x000000FF;len+=1;
    stream[len] =(int8_t)(_imu->roll>>16) &0x000000FF;len+=1;
    stream[len] =(int8_t)(_imu->roll>>8)  &0x000000FF;len+=1;
    stream[len] =(int8_t)(_imu->roll)     &0x000000FF;len+=1;

    stream[len] =(int8_t)(_imu->yaw>>24)  &0x000000FF;len+=1;
    stream[len] =(int8_t)(_imu->yaw>>16)  &0x000000FF;len+=1;
    stream[len]= (int8_t)(_imu->yaw>>8)   &0x000000FF;len+=1;
    stream[len]= (int8_t)(_imu->yaw)      &0x000000FF;len+=1;  
    
    stream[len] =(int8_t)(_internal->battery_percent)  &0x000000FF;len+=1;
    stream[len] =(int8_t)(_internal->battery_volt>>8)  &0x000000FF;len+=1;
    stream[len]= (int8_t)(_internal->battery_volt   )  &0x000000FF;len+=1;
 
    uint64_t mts[4];
    get_pwm(&mts[0],&mts[1],&mts[2],&mts[3]);
    
    stream[len] =(int8_t)((mts[0]*255)/MAX_PWM)  &0x000000FF;len+=1;
    stream[len] =(int8_t)((mts[1]*255)/MAX_PWM)  &0x000000FF;len+=1;
    stream[len]= (int8_t)((mts[2]*255)/MAX_PWM)  &0x000000FF;len+=1;    
    stream[len]= (int8_t)((mts[3]*255)/MAX_PWM)  &0x000000FF;len+=1;    
    
    stream[len]= (int8_t)_current_channel;len+=1;  
    
    stream[len]= (int8_t)_current_wifi;len+=1;  
    
    stream[len]= (int8_t)_pilot->gas;len+=1;  
    stream[len]= (int8_t)_pilot->pitch;len+=1;  
    stream[len]= (int8_t)_pilot->roll;len+=1;  
    stream[len]= (int8_t)_pilot->yaw;len+=1;  
    
    uint8_t * msgsent=&stream[len];
    strncpy(&stream[len],message,GCRY_PAYLOAD-len-1);
    if(GCRY_PAYLOAD-len-1>strlen(message)){
        len+=strlen(message);
    }else{
        len=GCRY_PAYLOAD;
    }

    send_80211beacon(beacon, stream, len);
    //send_80211beacon("INTERCEPTOR","CACA", 4);
}








int main() {
    printf("\r\n[I]INIT*** IMU TCC Interceptor");fflush(stdout);
    
    int8_t imu_read_res=0;
    int64_t limitedYaw=0;
    
    
    _imu=calloc(1, sizeof(struct euler_int_t));      
    _pilot=calloc(1, sizeof(struct pilot_t));
    _internal=calloc(1, sizeof(struct internal_int_t));  
    
    
    
    _pid_pitch=new_pid_int_zero();
    _pid_roll=new_pid_int_zero();
    _pid_yaw=new_pid_int_zero();
    
    
    _pid_rate_pitch=new_pid_int_zero();
    _pid_rate_roll=new_pid_int_zero();
    _pid_rate_yaw=new_pid_int_zero();    
    
    
    _pid_rate_pitch->Kp=KP_RATE_PITCH;
    _pid_rate_pitch->Ki=KI_RATE_PITCH;
    _pid_rate_pitch->Kd=KD_RATE_PITCH;
    _pid_rate_pitch->Kimax=KIMAX_RATE_PITCH;
    
    _pid_pitch->Kp=KP_ANGLE_PITCH;
            
    _pid_rate_roll->Kp=KP_RATE_ROLL;
    _pid_rate_roll->Ki=KI_RATE_ROLL;
    _pid_rate_roll->Kd=KD_RATE_ROLL;
    _pid_rate_roll->Kimax=KIMAX_RATE_ROLL;
    
    _pid_roll->Kp=KP_ANGLE_ROLL;
    
    _pid_rate_yaw->Kp=KP_RATE_YAW;
    _pid_rate_yaw->Ki=KI_RATE_YAW;
    _pid_rate_yaw->Kd=KD_RATE_YAW;
    _pid_rate_yaw->Kimax=KIMAX_RATE_YAW;
    
    _pid_yaw->Kp=KP_ANGLE_YAW;
           
    _pilot->ticks_since_recv=FAILSAFE_TRIGGER_TICKS;
    
    struct sched_param schedule;
    schedule.sched_priority=sched_get_priority_min(SCHED_FIFO);
    sched_setscheduler((int32_t)pthread_self(), SCHED_FIFO,&schedule);       
    printf("\r\n[I]Sched set FIFO");fflush(stdout);

    init_adc_iio();
    parse_battery(_internal, get_adc_battery());    
  
    printf("\r\n[I]ADC IIO initialized, V=%f P=%i",(float)_internal->battery_volt/100, _internal->battery_percent);fflush(stdout);
   
    init_gy953(IMU_PORT,100, 'E',LPF_PITCH,LPF_ROLL,LPF_YAW);
    printf("\r\n[I]GY953 initialized");fflush(stdout);

    init_pwm();

    printf("\r\n[I]PWM initialized");fflush(stdout);
    


    
    uint32_t udpsock=init_udp_sock(UDP_PORT);
    uint32_t msgsock=init_udp_sock(MSG_PORT);
    uint8_t cmdrecvudp[1024]={0x00,0x00};
    uint8_t message_telemetry[UDP_MEM_LEN]="";

    
    land_pid(_pid_pitch);
    land_pid(_pid_roll);
    land_pid(_pid_yaw);
    
    land_pid(_pid_rate_pitch);
    land_pid(_pid_rate_roll);
    land_pid(_pid_rate_yaw);
    
    
    int8_t str_beacon[BUFSIZ]="INTERCEPTOR";  //DEMO, to be upgraded with dynamic beacon selection
    int8_t str_wifi[BUFSIZ]="2";
    
    printf("\r\n[I]Getting first beacon frame...\r\n");fflush(stdout);
   //ILE *fbeacon = popen("tcpdump -i mon0 type mgt subtype beacon -c100  2>/dev/null |grep Beacon|grep -v '()'|grep '('|grep ')'|tcpdumpcut -d '(' -f2|cut -d')' -f1|head -n1","r");
    FILE *fwifi=popen("iwinfo|grep ESSID|wc -l","r");
    if(fwifi){
        fgets((char *)str_wifi, sizeof(str_wifi), fwifi);
        if(strlen(str_wifi)>0){_pilot->requested_wifi=atoi(str_wifi);}
        _current_wifi=_pilot->requested_wifi;
    }
    //apply_wifi_status();
    
    if(init_80211beacons(BEACONS_INTERCEPTOR_DRONE)==1){
        printf("\r\n[I]802.11 Beacon Frame tx/rx initialized");fflush(stdout);
    }else{
        printf("\r\n[I]802.11 Beacon Frame tx/rx not available");fflush(stdout);
        exit(-1);
    }        
    
    printf("\r\n[I]Transmitting with SSID %s",str_beacon);fflush(stdout);
    
    
    pthread_t pilot_pthread;
    if(pthread_create(&pilot_pthread, NULL,pilot_recv , NULL)) {

        fprintf(stderr, "\r\n[E]Error creating thread\n");
        exit(-1);

    }     
    
    printf("\r\n[I]IMU starts...\r\n");fflush(stdout);

    while (1){
        
        _pilot->ticks_since_recv++;
        if(_pilot->ticks_since_recv>=FAILSAFE_TRIGGER_TICKS){
            _pilot->ticks_since_recv=FAILSAFE_TRIGGER_TICKS;
        }
        if(_pseudo_hz_tick%3==1){
            memset(message_telemetry,0x00,sizeof(message_telemetry));
            recv_udp(msgsock, message_telemetry);
     
            
            send_telemetry(str_beacon, message_telemetry);
        }        
        if(recv_udp(udpsock,cmdrecvudp)>0)  {apply_pilot(cmdrecvudp);}
        
        imu_read_res=get_euler_sample(&_imu->pitch, &_imu->roll, &_imu->yaw,
                         &_imu->rate_pitch,&_imu->rate_roll,&_imu->rate_yaw,
                         &_imu->acc_pitch,&_imu->acc_roll,&_imu->acc_yaw);
        
        _pseudo_hz_tick++;
        if(_pseudo_hz_tick>=PSEUDO_HZ){_pseudo_hz_tick=0;}
        
        if(imu_read_res<0){
            printf("\r\n[I]BATT %f",(float)_internal->battery_volt/100);
            _poweron=0;
            set_pwm(0,0,0,0);
            
            printf("\r\n[E]Exiting due to IMU failure ");fflush(stdout);
            system("killall imu");
            exit(-1);
            //usleep)10000);
            //close_gy953();
            land_pid(_pid_pitch);
            land_pid(_pid_roll);
            land_pid(_pid_yaw);
            land_pid(_pid_rate_pitch);
            land_pid(_pid_rate_roll);  
            land_pid(_pid_rate_yaw);
            
            //init_gy953(IMU_PORT,100, 'E',LPF_PITCH,LPF_ROLL, LPF_YAW);
        }
        
        if(_pseudo_hz_tick==0){parse_battery(_internal, get_adc_battery());}
        
        
        //printf("\r\n%lli      ",-_pilot->yaw);fflush(stdout);
        if(_poweron==1){
            if(_pilot->ticks_since_recv>=FAILSAFE_TRIGGER_TICKS){
                _pilot->pitch=0;
                _pilot->roll=0;
                _pilot->yaw=0;
                if(_pilot->gas>MAX_FAILSAFE_GAS){
                    _pilot->gas=MAX_FAILSAFE_GAS;
                }

                printf("\r\n[W]Failsafe Fired");fflush(stdout); 
                reset_recv_seqnum();
            }
            if(_pilot->ticks_since_recv>=CUTDOWN_TRIGGER_TICKS){
                _pilot->pitch=0;
                _pilot->roll=0;
                _pilot->yaw=0;
                _pilot->gas=0;
                _poweron=0;
                set_pwm(0,0,0,0);
                printf("\r\n[W]Cutdown Fired");fflush(stdout);
            }
            if(_pseudo_hz_tick%3){
                if(_pilot->gas>MAX_POWERON_GAS){
                    _pilot->yaw_abs+=_pilot->yaw/5;
                }else{
                    _pilot->yaw_abs=_imu->yaw;
                }
            }
            
            //compute absolute angles
            pid_int_fixed_delta(_pid_pitch, _imu->pitch,    _imu->rate_pitch,  -_pilot->pitch);
            pid_int_fixed_delta(_pid_roll,  _imu->roll,     _imu->rate_roll,    _pilot->roll);      
            pid_int_fixed_delta(_pid_yaw,   _imu->yaw,      _imu->rate_yaw,     _pilot->yaw_abs);     
            
            //compute angular rates
            pid_int_fixed_delta(_pid_rate_pitch, _imu->rate_pitch,  _imu->acc_pitch, -_pid_pitch->output); 
            pid_int_fixed_delta(_pid_rate_roll,  _imu->rate_roll,   _imu->acc_roll,  -_pid_roll->output); 
            pid_int_fixed_delta(_pid_rate_yaw, _imu->rate_yaw,  _imu->acc_yaw, -_pid_yaw->output);// -_pilot->yaw);
            
            limitedYaw=-_pid_rate_yaw->output;

		  /* 
		  3    2P		
		    \  /

		    /  \
		  1P    0
		  */
            set_pwm(
                (((_pid_rate_pitch->output) +( _pid_rate_roll->output) +(-limitedYaw) +(_pilot->gas))),//0        
                (((-_pid_rate_roll->output) +( _pid_rate_pitch->output) +(limitedYaw) +(_pilot->gas))), //1
                (((_pid_rate_roll->output)  +(-_pid_rate_pitch->output) +(limitedYaw) +(_pilot->gas))), //2 
                (((-_pid_rate_pitch->output)+(-_pid_rate_roll->output) +(-limitedYaw) +(_pilot->gas)))  //3        
            );

        }else{
            set_pwm(0,0,0,0);
            _pilot->yaw_abs=_imu->yaw;
            land_pid(_pid_pitch);
            land_pid(_pid_roll);

            land_pid(_pid_rate_pitch);
            land_pid(_pid_rate_roll);            
        }

        
    }

  return(0);
}

