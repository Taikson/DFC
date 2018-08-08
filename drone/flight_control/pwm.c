/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "pwm.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint32_t f_pwm0;
uint32_t f_pwm1;
uint32_t f_pwm2;
uint32_t f_pwm3;


uint64_t cache_pwm0=0;
uint64_t cache_pwm1=0;
uint64_t cache_pwm2=0;
uint64_t cache_pwm3=0;

int8_t set_channel(char * ch_path, uint64_t val){
    uint32_t f=open(ch_path, O_WRONLY); 
    char buffer[32];
    
    sprintf(buffer, "%llu", val); //convert to ns
    
    if (f<=0){return -1;}
    int8_t bytes=write(f, buffer,strlen(buffer));
    close(f);
    if(bytes<=0){return -2;}
    return 0;
};

extern void init_pwm(){
    
    set_channel(PWM_EXPORT,0);
    set_channel(PWM_EXPORT,1);
    set_channel(PWM_EXPORT,2);
    set_channel(PWM_EXPORT,3);
    
    set_channel(PWM_ENA_0,0);
    set_channel(PWM_ENA_1,0);
    set_channel(PWM_ENA_2,0);
    set_channel(PWM_ENA_3,0);
    
    set_channel(PWM_PERIOD_0,MAX_PWM);
    set_channel(PWM_PERIOD_1,MAX_PWM);
    set_channel(PWM_PERIOD_2,MAX_PWM);
    set_channel(PWM_PERIOD_3,MAX_PWM);

    f_pwm0=open(PWM_DUTY_0,O_WRONLY);
    f_pwm1=open(PWM_DUTY_1,O_WRONLY);
    f_pwm2=open(PWM_DUTY_2,O_WRONLY);
    f_pwm3=open(PWM_DUTY_3,O_WRONLY);     
    
    set_pwm(0,0,0,0);
    
    set_channel(PWM_ENA_0,1);
    set_channel(PWM_ENA_1,1);
    set_channel(PWM_ENA_2,1);
    set_channel(PWM_ENA_3,1);
    
}

extern void set_pwm(uint64_t chan0, uint64_t chan1, uint64_t chan2, uint64_t chan3){
    char buffer[32];    
    sprintf(buffer, "%llu", chan0);
    write(f_pwm0,buffer,strlen(buffer));

    sprintf(buffer, "%llu", chan1);
    write(f_pwm1,buffer,strlen(buffer));
    
    sprintf(buffer, "%llu", chan2);
    write(f_pwm2,buffer,strlen(buffer));

    sprintf(buffer, "%llu", chan3);
    write(f_pwm3,buffer,strlen(buffer));
    
    cache_pwm0=chan0;
    cache_pwm1=chan1;
    cache_pwm2=chan2;
    cache_pwm3=chan3;
    
    //printf("\r 3:%llu\t\t 2:%llu\t\t 1:%llu\t\t 0:%llu",chan0,chan1,chan2,chan3);fflush(stdout);

}

extern void get_pwm(uint64_t *chan0, uint64_t *chan1, uint64_t *chan2, uint64_t *chan3){
    *chan0=cache_pwm0;
    *chan1=cache_pwm1;
    *chan2=cache_pwm2;
    *chan3=cache_pwm3;
    
}

extern void close_pwm(){
    set_pwm(0,0,0,0);
    close(f_pwm0);
    close(f_pwm1);
    close(f_pwm2);
    close(f_pwm3);

}