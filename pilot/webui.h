/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   webui.h
 * Author: tycoon
 *
 * Created on 13 de febrero de 2018, 13:43
 */

#ifndef WEBUI_H
#define WEBUI_H

#define TIMEOUT_MS_WWW 0
#define WWW_STREAM_LEN 10000

#include <stdint.h>

extern int8_t init_www(uint16_t port);
extern void close_www();
extern void poll_www(uint16_t poll_time);
extern void send_www(int8_t * ws_stream);
extern uint16_t recv_www(int8_t * poll_stream,uint16_t * poll_stream_len);
extern uint8_t send_www_channels(uint8_t * data  );
extern void *base64_encode(const uint8_t *in, uint8_t *encoded_data );
#endif /* WEBUI_H */

