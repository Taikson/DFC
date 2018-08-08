

/* 
 * File:   gy953.h
 * Author: tycoon
 *
 * Created on 10 de enero de 2018, 12:29
 */

#include <stdint.h>

#ifndef GY953_H
#define GY953_H


/*rateHZ: 50, 100, 200 */
/*qoure: use quaternions: 'Q', or Euler 'E'*/
extern int8_t init_gy953(char * serialpath, uint16_t rateHZ, char  qore, float lpf_pitch, float lpf_roll, float lpf_yaw );
extern void close_gy953();

extern int8_t get_euler_sample(int64_t *pitch, int64_t *roll, int64_t *yaw,int64_t *diff_pitch, int64_t *diff_roll, int64_t *diff_yaw,int64_t *dt_diff_pitch, int64_t *dt_diff_roll, int64_t *dt_diff_yaw); //blocking operation
//extern int8_t get_quaternion_sample();

#endif /* GY953_H */

