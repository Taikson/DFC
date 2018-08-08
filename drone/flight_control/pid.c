/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "pid.h"
#include <stdlib.h>


#include <stdio.h>

extern struct pid_int_t * new_pid_int_zero(){
    return calloc(1, sizeof(struct pid_int_t));
}
extern int8_t parse_pid_config(struct pid_int_t *p, uint8_t *pkt){
    if(pkt[0]!=PID_PROTOCOL_CONFIG){return -1;}
    
    pkt[0]=0x00;
    //pkt[1] is for pid type, not managed here
    //assumes fixed len 16bytes (+2) 
    p->Kp=      (int64_t)(pkt[2]<<24)|(pkt[3]<<16)|(pkt[4]<<8)|(pkt[5]);
    p->Ki=      (int64_t)(pkt[6]<<24)|(pkt[7]<<16)|(pkt[8]<<8)|(pkt[9]);
    p->Kd=      (int64_t)(pkt[10]<<24)|(pkt[11]<<16)|(pkt[12]<<8)|(pkt[13]);
    p->Kimax=   (int64_t)(pkt[14]<<24)|(pkt[15]<<16)|(pkt[16]<<8)|(pkt[17]);
    
    printf(" P %lli I %lli D %lli max %lli \r\n",p->Kp,p->Ki,p->Kd,p->Kimax);fflush(stdout)    ;
    return 0;

}

extern void land_pid(struct pid_int_t *p ){
    p->d=0;
    p->d1=0;
    p->d2=0;
    p->dlfp=0;
    p->error=0;
    p->i=0;
    p->od=0;
    p->oi=0;
    p->op=0;
    p->output=0;
    p->p=0;

}

extern void pid_int_fixed_delta( struct pid_int_t *p, int64_t currAngle, int64_t currAngVel, int64_t targetAngle){
	int64_t error;	
        
        //currAngle*=100;
        //currAngVel*=100;
        //targetAngle*100;
        
	error=currAngle-targetAngle;
	if(p->Ki!=0){		
            p->i=p->i+error;
            if(p->Kimax>0){//antiwindump
                if(p->i<-p->Kimax){
                    p->i=-p->Kimax;
                }
                if(p->i>p->Kimax){
                    p->i=p->Kimax;
                }                
            }
            p->oi=p->Ki*p->i;
	}
	if(p->Kd!=0){

            //p->d=(int64_t)((currAngVel+p->d1+p->d2)/3); //2.8
            p->d=currAngVel;
            p->od=p->Kd*p->d;
            p->d2=p->d1;
            p->d1=p->d;
	}	
        p->op=p->Kp*error;
	p->output=(p->op+p->oi+p->od)/10;

	p->error=error;
		
}
