#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>



#define GY953_CMD_LEN 3
#define GY953_CMD_UART_115200 0xAF
#define GY953_CMD_50_HZ 0xA4
#define GY953_CMD_100_HZ 0xA5
#define GY953_CMD_200_HZ 0xA6
#define GY953_CMD_EULER 0x45
#define GY953_CMD_QUATERNION 0x65 //not sure
#define GY953_EULER_LEN 11

#define GY953_UART_RATE 115200




uint32_t _f_serie;

int64_t _cache_pitch;
int64_t _cache_roll;
int64_t _cache_yaw;

int64_t _cache_dtpitch;
int64_t _cache_dtroll;
int64_t _cache_dtyaw;

float _lpf_pitch;
float _lpf_roll;
float _lpf_yaw;

int8_t send_command(char command){
    
    uint8_t buffer[GY953_CMD_LEN]={0xA5, 0x00,0x00};
    buffer[1]=command;
    buffer[2]=(uint8_t)(buffer[0]+buffer[1]);
    
    //printf("\r\n send command %x\t%x\t%x",buffer[0], buffer[1], buffer[2]);
    
    return write(_f_serie,buffer,GY953_CMD_LEN);
    
}
extern int8_t get_euler_sample(int64_t *pitch, int64_t *roll, int64_t *yaw,int64_t *diff_pitch, int64_t *diff_roll, int64_t *diff_yaw,int64_t *dt_diff_pitch, int64_t *dt_diff_roll, int64_t *dt_diff_yaw){
    uint8_t buffer[GY953_EULER_LEN];
    int16_t got=0;
    int8_t ac=0;
    uint8_t checksum=0;
    int64_t yaw_sensor=0;
    lseek(_f_serie, 0, SEEK_SET);
    got=read(_f_serie, buffer, GY953_EULER_LEN);
    
    if(got != GY953_EULER_LEN){
        printf("\r\n[W]GY953 Bad length : %i",got);
        return -1;
    }
    
    if((buffer[0]!=0x5A)||(buffer[1]!=0x5A)||(buffer[2]!=0x45)||(buffer[3]!=0x06)){
        printf("\r\n[W]GY953 Bad header");
        return -2;
    }
    for(ac=0;ac<GY953_EULER_LEN-1;ac++){
        
        checksum+=buffer[ac];
    }
    if(checksum!=buffer[10]){
        printf("\r\n[W]GY953 Bad checksum");
        return -3;
    }

    
    

    *pitch=(int16_t)((int16_t)(buffer[4]<<8|buffer[5])*_lpf_pitch)+(_cache_pitch*(1-_lpf_pitch));
    *roll=(int16_t)((int16_t)(buffer[6]<<8|buffer[7])*_lpf_roll)+(_cache_roll*(1-_lpf_roll));
    yaw_sensor=(int16_t)((int16_t)(buffer[8]<<8|buffer[9]));
    
    
    
    *diff_pitch=*pitch-_cache_pitch;
    *diff_roll=*roll-_cache_roll;
    *diff_yaw=yaw_sensor-_cache_yaw;
    

    
    if(abs(*diff_yaw)>30000){
       *diff_yaw=_cache_dtyaw;
        //printf("\r\n yaw %lli %lli!!!!!!!!!!!!!!!!!!!", *yaw, *diff_yaw );fflush(stdout);
    }else{
        *diff_yaw=(int64_t)((*diff_yaw*_lpf_yaw)+(_cache_dtyaw*(1-_lpf_yaw)));
         //printf("\r\n yaw %lli %lli ", *yaw, *diff_yaw );fflush(stdout);
    }
    
    *yaw+=*diff_yaw;
    
    _cache_pitch=*pitch;
    _cache_roll=*roll;
    _cache_yaw=yaw_sensor;

    
    *dt_diff_pitch=*diff_pitch-_cache_dtpitch;
    *dt_diff_roll=*diff_roll-_cache_dtroll;
    *dt_diff_yaw=*diff_yaw-_cache_dtyaw;
    
    _cache_dtpitch=*diff_pitch;
    _cache_dtroll=*diff_roll;
    _cache_dtyaw=*diff_yaw;    
    
    //printf("\r\n OK");
    
    return got;

}
extern int8_t init_gy953(char * serialpath, uint16_t rateHZ, char  qore ,float lpf_pitch, float lpf_roll, float lpf_yaw){   
    
    char cmd[128];
    
    sprintf(cmd, "stty -F %s %i", serialpath, GY953_UART_RATE);
    system(cmd);
    sprintf(cmd, "stty -F %s raw", serialpath);
    system(cmd);

    _lpf_pitch=lpf_pitch;
    _lpf_roll=lpf_roll;
    _lpf_yaw=lpf_yaw;
    
    
    _f_serie=open(serialpath, O_RDWR|O_NOCTTY|O_NONBLOCK);
    int8_t got;
    if (_f_serie==-1){
        return -1;
    }    
       
    int8_t ac=0;
    
    int64_t dummy_pitch=0;
    int64_t dummy_roll=0;
    int64_t dummy_yaw=0;
    
    int64_t dummy_dtpitch=0;
    int64_t dummy_dtroll=0;
    int64_t dummy_dtyaw=0;
    
    _cache_pitch=0;
    _cache_roll=0;
    _cache_yaw=0;
    
    if(rateHZ<=50){
        send_command(GY953_CMD_50_HZ);
    }
    if((rateHZ>=100)&&(rateHZ<200)){
        send_command(GY953_CMD_100_HZ);
    }
    if(rateHZ>=200){
        send_command(GY953_CMD_200_HZ);
    }
    
    while((ac<10)&&(got!=GY953_EULER_LEN)){
        send_command(GY953_CMD_EULER);
        usleep(100000);
        got=get_euler_sample(&_cache_pitch,&_cache_roll,&_cache_yaw,&dummy_pitch,&dummy_roll,&dummy_yaw,&dummy_dtpitch,&dummy_dtroll,&dummy_dtyaw);
        
        ac++;
    }
    close(_f_serie);    
    if (got!=GY953_EULER_LEN){
        return -2;
    }
    _f_serie=open(serialpath, O_RDONLY|O_NOCTTY);
    return 0;
}

extern void close_gy953(){
    close(_f_serie);
}



