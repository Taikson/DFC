/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   udpserver.h
 * Author: tycoon
 *
 * Created on 14 de enero de 2018, 4:44
 */

#ifndef UDPSERVER_H
#define UDPSERVER_H
#include <stdint.h>
#define UDP_MEM_LEN 1024
extern uint32_t init_udp_sock(uint16_t port);
extern int32_t recv_udp(uint32_t sock,uint8_t *buffer);
extern uint8_t parse_raw_protocol_0x11_0x12(uint8_t * data, int16_t raw_channels[20] );
#endif /* UDPSERVER_H */

