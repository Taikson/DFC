/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   adc.h
 * Author: tycoon
 *
 * Created on 5 de febrero de 2018, 19:38
 */


#include <stdint.h>

#ifndef ADC_H
#define ADC_H



extern uint8_t init_adc_iio();

extern uint32_t get_adc_battery();
#endif /* ADC_H */

