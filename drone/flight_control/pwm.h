/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   pwm.h
 * Author: tycoon
 *
 * Created on 12 de enero de 2018, 1:43
 */

#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#define MAX_PWM 100000 //us


#define OFF_PWM 0

#define PWM_FOLD "/sys/class/pwm/pwmchip0/"
#define PWM_EXPORT PWM_FOLD"export"
#define PWM_PATH PWM_FOLD"pwm"

#define PWM_DUTY_0 PWM_PATH"0/duty_cycle"
#define PWM_DUTY_1 PWM_PATH"1/duty_cycle"
#define PWM_DUTY_2 PWM_PATH"2/duty_cycle"
#define PWM_DUTY_3 PWM_PATH"3/duty_cycle"

#define PWM_PERIOD_0 PWM_PATH"0/period"
#define PWM_PERIOD_1 PWM_PATH"1/period"
#define PWM_PERIOD_2 PWM_PATH"2/period"
#define PWM_PERIOD_3 PWM_PATH"3/period"

#define PWM_ENA_0 PWM_PATH"0/enable"
#define PWM_ENA_1 PWM_PATH"1/enable"
#define PWM_ENA_2 PWM_PATH"2/enable"
#define PWM_ENA_3 PWM_PATH"3/enable"

extern void init_pwm();

extern void set_pwm(uint64_t chan0, uint64_t chan1, uint64_t chan2, uint64_t chan3);
extern void get_pwm(uint64_t *chan0, uint64_t *chan1, uint64_t *chan2, uint64_t *chan3);
extern void close_pwm();

#endif /* PWM_H */

