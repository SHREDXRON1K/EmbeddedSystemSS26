/*
* Copyright (c) 2025 Hochschule Darmstadt
*
* This software is provided exclusively for the practical part of the
* course 'Embedded Systems' at 'Darmstadt University of Applied Sciences
* (Hochschule Darmstadt, h_da)'.
*
* Any use, distribution, or modification outside this context is prohibited
* without explicit permission.
*
* AUTHORS: David Heiß, Manfred Pester, Jens-Peter Akelbein
* FILE: matrix.c
* CONTENTS: implementation for all functions driving I2C LED matrix
*/


#include "matrix.h"


#include <cm3.h>
#include <sam.h>
#include <state.h>
#include <stdbool.h>


// static unsigned char buffer[] = {[0 ... 16] = 0};


// static unsigned char number_3[16] = {
//    0x00, 0x1C,
//    0x00, 0x04,
//    0x00, 0x04,
//    0x00, 0x1C,
//    0x00, 0x04,
//    0x00, 0x04,
//    0x00, 0x04,
//    0x00, 0x1C
// };


#define MY_LAB_NUMBER 9;


static unsigned char buffer[16] = { // es zeigt 3
  0x00, 0x1C,
  0x00, 0x04,
  0x00, 0x04,
  0x00, 0x1C,
  0x00, 0x04,
  0x00, 0x04,
  0x00, 0x04,
  0x00, 0x1C
};


static const unsigned char font[10][5] = {
   {0x07, 0x05, 0x05, 0x05, 0x07}, // 0
   {0x02, 0x06, 0x02, 0x02, 0x07}, // 1
   {0x07, 0x01, 0x07, 0x04, 0x07}, // 2
   {0x07, 0x01, 0x07, 0x01, 0x07}, // 3
   {0x05, 0x05, 0x07, 0x01, 0x01}, // 4
   {0x07, 0x04, 0x07, 0x01, 0x07}, // 5
   {0x07, 0x04, 0x07, 0x05, 0x07}, // 6
   {0x07, 0x01, 0x01, 0x01, 0x01}, // 7
   {0x07, 0x05, 0x07, 0x05, 0x07}, // 8
   {0x07, 0x05, 0x07, 0x01, 0x07}, // 9
};


// static const unsigned char font[10][8] = {
//     {0x07, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x07}, // 0
//     {0x02, 0x06, 0x02, 0x02, 0x02, 0x02, 0x02, 0x07}, // 1
//     {0x07, 0x01, 0x07, 0x04, 0x07}, // 2
//     {0x07, 0x01, 0x07, 0x01, 0x07}, // 3
//     {0x05, 0x05, 0x07, 0x01, 0x01}, // 4
//     {0x07, 0x04, 0x07, 0x01, 0x07}, // 5
//     {0x07, 0x04, 0x07, 0x05, 0x07}, // 6
//     {0x07, 0x01, 0x01, 0x01, 0x01}, // 7
//     {0x07, 0x05, 0x07, 0x05, 0x07}, // 8
//     {0x07, 0x05, 0x07, 0x01, 0x07}, // 9
// };while (!(TC2_SR2 & TC2_SR2_LDRBS));






static bool matrix_ready()
{
   // TODO
  
   // check previous transmission is complete (bus idle)
   if (!(TWI1_SR & TWI1_SR_TXCOMP))
       return false;


   // check no NACK from previous transmission
   if (TWI1_SR & TWI1_SR_NACK)
       return false;

    return true;

}


static bool matrix_write(const unsigned char *buf, unsigned int n)
{
   // TODO
  
   // BYTE TO BYTE TRANSFER
   // for (unsigned int i = 0; i < n; i++)
   // {
   //     // start condition
   //     if (i==0) TWI1_CR = TWI1_CR_START;


   //     // warten
   //     while (!(TWI1_SR & TWI1_SR_TXRDY));


   //     // check if slave sent NACK — address wrong or device not responding
   //     if (TWI1_SR & TWI1_SR_NACK)
   //         return false;


   //     // data schicken
   //     TWI1_THR = buf[i];
   // }




   // // STOP
   // TWI1_CR = TWI1_CR_STOP;
   // while (!(TWI1_SR & TWI1_SR_TXCOMP));


   // return true;


   // ==================DMA (Direct Memory Access)==================
   // setup DMA first
   TWI1_TPR = (uint32_t)buf;   // put buffer in Transfer Pointer Register
   TWI1_TCR = n;               // set the Transfer Counter Register with the length of buffer
   TWI1_PTCR = TWI1_PTCR_TXTEN;// set the Peripheral Transfer Control Register. Writing TXTEN activates the DMA channel for TWI1 transmission


   TWI1_CR = TWI1_CR_START;    // start
  
   while (!(TWI1_SR & TWI1_SR_TXRDY)); // wait for DMA to finish all but last byte


   // check NACK
   if (TWI1_SR & TWI1_SR_NACK) {
       TWI1_PTCR = TWI1_PTCR_TXTDIS;
       return false;
   }
  
   TWI1_CR = TWI1_CR_STOP;     // stop
  
   while (!(TWI1_SR & TWI1_SR_TXCOMP));    // wait to complete


   TWI1_PTCR = TWI1_PTCR_TXTDIS;   // disable DMAwhile (!(TC2_SR2 & TC2_SR2_LDRBS));
  
   return true;
}










static bool matrix_distance(int dist)
{
   if (!matrix_ready())
       return false;


   /*
    * Beispiel
    * Nur jeder zweiter Eintrag wird auf der Matrix angezeigt.
    * Der Kontroller ist für zwei Matrizen ausgelegt.
    */


   for (unsigned int i = 0; i < sizeof(buffer); i++) {
       buffer[i] = 0;
   }


   int n;

   if (state.sonic == SONIC_STATE_ON) {
       n = dist/10;   // show distance
   } else {
       n = MY_LAB_NUMBER;  // show your lab computer number from Aufgabe 3
   }


   // SHOW DIGITS
   // // ====================INHALT VON BUFFER MANIPULIEREN====================
   // if (n < 0)  n = 0;
   // if (n > 99) n = 99;


   // int tens = n / 10;   // left digit
   // int ones = n % 10;   // right digit


   // buffer[0] = 0x00;    // register address


   // for (int row = 0; row < 5; row++) {
   //     unsigned char left  = font[tens][row] << 4;  // shift to bits 6-4
   //     unsigned char right = font[ones][row] << 0;  // shift to bits 2-0
   //     buffer[1 + row * 2] = left | right;
   // }
   // // remaining rows blank
   // for (int row = 5; row < 8; row++)
   //     buffer[1 + row * 2] = 0x00;


   // // ========================================================================
   //
  
   // TODO turn on each LED based on dist
   if (n < 0)  n = 0;  // invert bit when minus // absolut
   if (n > 64) n = 64;
   buffer[0] = 0x00;    // register address


   // Build the bit
   int64_t temp=0;
   for (int i = 0; i < n; ++i) {
       temp|= (1 <<i) ;
   }


   // Put the bits from temp in the buffer, only in odd, the rest even will be written as 0x00
   for(int i =0; i<8; ++i) {
       buffer[i*2] = 0x00;
       buffer[i*2 + 1] = ( temp >> (i*8) ) ;   // buffer is unsigned char with 8 bit length, so only the first 8 bit will be taken
       // the bits in temp will be
   }


   matrix_write(buffer, sizeof(buffer));
   return true;
}


void matrix_init(void)
{
   // TODO
   PMC_PCER0 = PMC_PCER0_PID23; // turn on clock for twi1 c
  
   PIOB_PDR |= (1<<12) | (1<<13);
   PIOB_ABSR &= ~((1 << 12) | (1 << 13));


   TWI1_CR = TWI1_CR_SWRST;


  // Config mode master
  TWI1_CR = TWI1_CR_MSEN; // cest le master ic qui sendet les daten au slave led


  // Config
  TWI1_CWGR = (13 << 0) | (13 << 8) | (5 << 16);


  // Adresse von matrix
//    TWI1_MMR = (0x70 << 16);  // adresse I2C
   TWI1_MMR = (0x70 << 16) & ~TWI1_MMR_MREAD;
 
 
   unsigned char init_sequence[] = {
       0x21, // internal system clock enable
       0xA0, // row output pin set
       0xE0, // dimming set
       0x81, // blinking set
   };


   for (unsigned int i = 0; i < sizeof(init_sequence); i++)
       while (!matrix_write(init_sequence + i, 1));


   while (!matrix_write(buffer, sizeof(buffer)));
 
  // TODO

}


void matrix_loop(void)
{
   matrix_distance(value.sonic.distance - value.goal); 
   // matrix_write(buffer, 16); // beispiel von zahl // Already called in matrix distance
} 





