
#ifndef _IMU_H_INTERCEPTOR
#define _IMU_H_INTERCEPTOR


#define MAX_PERCENT_PILOT_GAS 80
#define MAX_DEGREES_PILOT_PITCHROLL 5
#define MAX_DEGREES_PILOT_YAW 5

#define MAX_PERCENT_POWERON_GAS 10
#define MAX_DEGREES_POWERON_YAW 1

#define MAX_PERCENT_FAILSAFE_GAS 40

#define MAX_POWERON_GAS  (MAX_PWM*MAX_PERCENT_POWERON_GAS)/100
#define MIN_POWERON_YAW ((MAX_PWM*MAX_DEGREES_POWERON_YAW)/359)/2
#define MAX_FAILSAFE_GAS (MAX_PWM*MAX_PERCENT_FAILSAFE_GAS)/100



#define MAX_VBAT 430 // *100
#define MIN_VBAT 300 // *100
#define MIN_VDC  470 // *100





#define PSEUDO_HZ 100 //gy953 output rate per second
#define FAILSAFE_TRIGGER_TICKS PSEUDO_HZ*1 // 1 second to fire failsafe
#define CUTDOWN_TRIGGER_TICKS PSEUDO_HZ*2 // 2 second to fire cutdown
#include <stdint.h>

struct pilot_t{
    int64_t gas;

    int64_t pitch;
    int64_t roll;
    int64_t yaw; 
    
    int64_t yaw_abs;
    
    int64_t auxA;
    int64_t auxB;
    int64_t auxC;    

    int8_t requested_channel;
    int8_t requested_wifi;
    int16_t ticks_since_recv;
};

struct euler_int_t{
    int64_t pitch; //cabeceo
    int64_t roll;  //alabeo 
    int64_t yaw;   //guiñada 
    
    int64_t gas;    
    
    int64_t rate_pitch;
    int64_t rate_roll;
    int64_t rate_yaw;
    
    int64_t acc_pitch;
    int64_t acc_roll;
    int64_t acc_yaw;    
    
    int64_t rate_gas;  
    
};


struct internal_int_t{
    int16_t battery_volt;
    int8_t battery_percent;
};

#endif