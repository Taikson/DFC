/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   pid.h
 * Author: tycoon
 *
 * Created on 11 de enero de 2018, 20:52
 */

#ifndef PID_H
#define PID_H
#include <stdint.h>
#define PID_PROTOCOL_CONFIG 0x0F
struct pid_int_t{
    int64_t output;
    int64_t error;
    int64_t d;
    int64_t i;
    int64_t p;
    int64_t d1;
    int64_t d2;
    int64_t dlfp;
    int64_t op;
    int64_t oi;
    int64_t od;
    
    int64_t Kp;
    int64_t Ki;
    int64_t Kd;
    int64_t Kimax;    
};

extern struct pid_int_t * new_pid_int_zero();

extern void pid_int_fixed_delta(struct pid_int_t *p, int64_t currAngle, int64_t currAngVel, int64_t targetAngle);
  
extern int8_t parse_pid_config(struct pid_int_t *p, uint8_t *pkt);

extern void land_pid(struct pid_int_t *p );




#endif /* PID_H */

