#include "adc.h"
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define IIO_ADC_PATH "/sys/bus/iio/devices/iio:device0/"
#define IIO_ADC_SAMPLING_FREQ IIO_ADC_PATH"in_voltage_sampling_frequency"
#define IIO_BATTERY_RAW IIO_ADC_PATH"in_voltage0_raw"
#define IIO_BATTERY_SCALE IIO_ADC_PATH"in_voltage0_scale"

const char *  SAMPLING_FREQ="15";
const float VOLTAGE_DIVIDER_FACTOR=2.5; //5Vinput / 2Voutput
 

double _sensor_factor=0;
uint32_t _f_raw=0;

extern uint8_t init_adc_iio(){
    uint32_t fservice=open(IIO_ADC_SAMPLING_FREQ, O_WRONLY|O_NOCTTY);
    char strfactor[256]="0";
    write(fservice,SAMPLING_FREQ,sizeof(SAMPLING_FREQ));    
    close(fservice);
    
    sleep(1);
    
    fservice=open(IIO_BATTERY_SCALE, O_RDONLY|O_NOCTTY);
    read(fservice,strfactor,sizeof(strfactor));
    _sensor_factor=atof(strfactor);
    close(fservice);
    
    _f_raw=open(IIO_BATTERY_RAW, O_RDONLY|O_NOCTTY);
    return 1;
}

extern uint32_t get_adc_battery(){
    char strraw[256]="0";
    uint32_t raw=0;
    lseek(_f_raw, 0, SEEK_SET);
    read(_f_raw,strraw,sizeof(strraw));
    raw=atoi(strraw);
    return (uint32_t)raw*_sensor_factor*VOLTAGE_DIVIDER_FACTOR*100;
}