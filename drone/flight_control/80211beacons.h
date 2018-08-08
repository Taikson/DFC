/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   80211beacons.h
 * Author: tycoon
 *
 * Created on 14 de febrero de 2018, 17:02
 */
#ifndef _INTERCEPTOR_BEACONS_H
#define _INTERCEPTOR_BEACONS_H
#include <stdint.h>
enum beacons_op_mode {BEACONS_INTERCEPTOR_PILOT, BEACONS_INTERCEPTOR_DRONE};
#define GCRY_KEYLEN 16
#define GCRY_IVLEN 16
//#define CHECKSUM_LEN 4
#define CHECKSUM_LEN 32
#define SEQ_LEN 8
#define GCRY_PAYLOAD 512


extern int8_t init_80211beacons(enum beacons_op_mode mode);
extern void close_80211beacons();
extern void send_80211beacon(int8_t * ssid_tx, int8_t * stream, uint16_t len);
extern void recv_80211beacon(int8_t * data, int32_t * data_len, int64_t * seqnum,uint8_t * radiotap_channel);
extern int16_t set_channel_nl80211(uint8_t channel);
extern void  print_vector(char * vector, uint16_t len);
extern void reset_recv_seqnum();
#endif