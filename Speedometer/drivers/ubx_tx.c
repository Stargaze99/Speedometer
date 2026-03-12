/*
 * ubx_tx.c
 *
 * Created: 2026-03-12 10:20:21
 *  Author: Admin
 */ 

#include "ubx_tx.h"

static void ubx_checksum(uint8_t a, uint8_t *cka, uint8_t *ckb){
    
    *cka = (uint8_t) (*cka + a);
    *ckb = (uint8_t) (*cka + *ckb);
}

/*Exmpel UBX: B5 62 <CLS> <ID> <LEN_L> <LEN_H> <PAYLOAD...> <CK_A> <CK_B>*/
void ubx_send(uint8_t cls, uint8_t id, uint16_t len, const uint8_t *payload){
    
    /*init checksum*/
    uint8_t cka = 0;
    uint8_t ckb = 0;
    
    /*Start sync*/
    uart0_write(0xB5);
    uart0_write(0x62);
    
    uart0_write(cls);
    ubx_checksum(cls, &cka, &ckb);
    
    uart0_write(id);
    ubx_checksum(id, &cka, &ckb);
    
    /*Låg*/
    uart0_write((uint8_t) (len & 0xFF));
    ubx_checksum((uint8_t) (len & 0xFF), &cka, &ckb);
    
    /*hög*/
    uart0_write((uint8_t) (len >> 8));
    ubx_checksum((uint8_t) (len >> 8), &cka, &ckb);
    
    for (uint16_t i = 0; i < len; i++)
    {
        uart0_write(payload[i]);
        ubx_checksum(payload[i], &cka, &ckb);
    }
    
    uart0_write(cka);
    uart0_write(ckb);
    
}

