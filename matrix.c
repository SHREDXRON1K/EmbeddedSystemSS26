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

static unsigned char show_off[16] = { // shows "OFF"
    0x00, 0xDB,  // row 0: O FF top
    0x00, 0xD2,  // row 1
    0x00, 0xDB,  // row 2: middle bar of F
    0x00, 0xD2,  // row 3
    0x00, 0xD2,  // row 4
    0x00, 0x00,  // row 5 blank
    0x00, 0x00,  // row 6 blank
    0x00, 0x00,  // row 7 blank
};

static unsigned char show_questionMark[16] = { // shows "?"
    0x00, 0x18,  // row 0: .XX.  top curve
    0x00, 0x24,  // row 1: X..X  right side
    0x00, 0x04,  // row 2: ...X  curl inward
    0x00, 0x08,  // row 3: ..X.  blank gap
    0x00, 0x08,  // row 4: ..X.  dot
    0x00, 0x00,  // row 5: ....  blank
    0x00, 0x08,  // row 6: ..X.  dot
    0x00, 0x00,  // row 7: ....  dot
};


// static const unsigned char font[10][5] = {
//    {0x07, 0x05, 0x05, 0x05, 0x07}, // 0
//    {0x02, 0x06, 0x02, 0x02, 0x07}, // 1
//    {0x07, 0x01, 0x07, 0x04, 0x07}, // 2
//    {0x07, 0x01, 0x07, 0x01, 0x07}, // 3
//    {0x05, 0x05, 0x07, 0x01, 0x01}, // 4
//    {0x07, 0x04, 0x07, 0x01, 0x07}, // 5
//    {0x07, 0x04, 0x07, 0x05, 0x07}, // 6
//    {0x07, 0x01, 0x01, 0x01, 0x01}, // 7
//    {0x07, 0x05, 0x07, 0x05, 0x07}, // 8
//    {0x07, 0x05, 0x07, 0x01, 0x07}, // 9
// };


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
    if (!matrix_ready()) return false;
  
    // ===== BYTE TO BYTE TRANSFER =====
    // for (unsigned int i = 0; i < n; i++)
    // {
    //     // start condition
    //     if (i==0) TWI1_CR = TWI1_CR_START;

    //     // wait
    //     while (!(TWI1_SR & TWI1_SR_TXRDY));

    //     // check if slave sent NACK — address wrong or device not responding
    //     if (TWI1_SR & TWI1_SR_NACK)
    //         return false;

    //     // send data
    //     TWI1_THR = buf[i];
    // }

    // // STOP
    // TWI1_CR = TWI1_CR_STOP;
    // while (!(TWI1_SR & TWI1_SR_TXCOMP));

    // return true;

    // ===== BYTE TO BYTE TRANSFER =====


    // ==================DMA (Direct Memory Access)==================
    // setup DMA first
    TWI1_TPR = (uint32_t)buf;   // put buffer in Transfer Pointer Register
    TWI1_TCR = n;               // set the Transfer Counter Register with the length of buffer
    TWI1_PTCR = TWI1_PTCR_TXTEN;// set the Peripheral Transfer Control Register. Writing TXTEN activates the DMA channel for TWI1 transmission

    TWI1_CR = TWI1_CR_START;    // start

    // wait for DMA to push all bytes — but bounded, not infinite
    uint32_t timeout = 100000;
    while (!(TWI1_SR & TWI1_SR_TXRDY)) {
        if (--timeout == 0) {
            // slave is stuck/not responding — abort cleanly
            TWI1_CR = TWI1_CR_STOP;
            timeout = 100000;
            while (!(TWI1_SR & TWI1_SR_TXCOMP)) {
                if (--timeout == 0) break; // give up even on STOP, don't freeze
            }
            TWI1_PTCR = TWI1_PTCR_TXTDIS;
            return false;
        }
    }

    // check NACK — and ALWAYS issue STOP regardless of NACK or not,
    // so the bus is left in a clean state either way
    bool nacked = (TWI1_SR & TWI1_SR_NACK) != 0;
    TWI1_CR = TWI1_CR_STOP;

    timeout = 100000;
    while (!(TWI1_SR & TWI1_SR_TXCOMP)) {
        if (--timeout == 0) break; // bounded wait here too
    }

    TWI1_PTCR = TWI1_PTCR_TXTDIS;

    return !nacked;
}



// static void matrix_write_char(const unsigned char *glyph) // not from Baustein, as helper function
// {
//     buffer[0] = 0x00; // register address
//     for (int row = 0; row < 5; row++) {
//         buffer[row * 2 + 1] = glyph[row];
//     }
//     // blank remaining rows
//     for (int row = 5; row < 8; row++) {
//         buffer[row * 2 + 1] = 0x00;
//     }
// }






static bool matrix_distance(int dist)
{
    if (!matrix_ready())
        return false;

    for (unsigned int i = 0; i < sizeof(buffer); i++)
        buffer[i] = 0;  // clean buffer

    int n;
    bool negative = false;  // flag for negative value

    if (state.sonic == SONIC_STATE_ON) {
        n = dist / 10;
    } else {
        matrix_write(show_off, sizeof(show_off));
        return true;
    }

    // --- Negative: overshot target, too close ---
    if (n < 0) {
        negative = true;
        n = -n;
    }

    // --- Overflow: show '?' ---
    if (n > 64) {
        unsigned char blink_cmd = 0x81;
        while (!matrix_write(&blink_cmd, 1));
        matrix_write(show_questionMark, sizeof(show_questionMark));
        return true;
    }

    // --- Blink if negative ---
    unsigned char blink_cmd = negative ? 0x83 : 0x81;
    while (!matrix_write(&blink_cmd, 1));

    // --- Build dot pattern ---
    buffer[0] = 0x00;
    int64_t temp = 0;
    for (int i = 0; i < n; i++) {
        temp |= (int64_t)1 << i;
    }

    for (int i = 0; i < 8; i++) {
        buffer[i * 2]     = 0x00;
        buffer[i * 2 + 1] = (unsigned char)(temp >> (i * 8));
    }

    matrix_write(buffer, sizeof(buffer));
    return true;
}


void matrix_init(void)
{
    PMC_PCER0 = PMC_PCER0_PID23; // turn on clock for twi1 c
  
    PIOB_PDR |= (1<<12) | (1<<13);
    PIOB_ABSR &= ~((1 << 12) | (1 << 13));

    TWI1_CR = TWI1_CR_SWRST;

    // give the matrix chip's power rail/oscillator a moment to settle
    for (volatile uint32_t i = 0; i < 50000; i++);

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


    for (unsigned int i = 0; i < sizeof(init_sequence); i++) { // without this, minicom will hang // also cause the problem where the motor could not do reverse
        uint32_t timeout = 1000000;
        while (!matrix_write(init_sequence + i, 1)) {
            if (--timeout == 0) break;  // give up, don't hang forever
        }
    }
       


    uint32_t timeout = 1000000;
    while (!matrix_write(buffer, sizeof(buffer))) {
        if (--timeout == 0) break;
    }

}


void matrix_loop(void)
{
   matrix_distance(value.sonic.distance - value.goal); 
   // matrix_write(buffer, 16); // example number // Already called in matrix distance
} 





