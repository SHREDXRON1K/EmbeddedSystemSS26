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
 * FILE: led.c
 * CONTENTS: Source for Due Onboard LED control
 */

#include <led.h>

#include <stdbool.h>
#include <state.h>
#include <sam.h>

// BUILTIN-LED on Arduino Due Board on PIO Port B Bit 27
#define DUE_LED_BUILTIN 0x08000000

// return state of LED
static bool led_get(void)
{
    // TODO
    // T1_A1
    // unsigned int status = PIOB_ODSR;   // Output Data Set Regiter
    // return status;
    // if 0 then I/O is 0

    return (PIOB_ODSR & (1<<27)); // Mask the other bits, only read 27
    // if another bits are not masked, then will always true if
}

// turn on/off LED
static void led_set(bool led_on)
{
    // TODO
    // T1_A1
    led_on ? (PIOB_SODR = (1<<27)) : (PIOB_CODR = (1<<27)); // true -> on
}

// toogle LED
static void led_toggle(void)
{
    led_set(!led_get());
}

// LED is toggling after a number of calls
static void led_blinking(void)
{
    static unsigned int last_update = 0;
    if (value.clock.time - last_update >= 500)
    {
        led_toggle();
        last_update = value.clock.time;
    }
}

// initial config for using LED
void led_init(void)
{
    // T1_A1 initialize LED
    PIOB_PER = (1 << 27)  | 1; // Peripherie Enable Register
    PIOB_OER = (1 << 27)  | 1; // Output Enable Register

    // T1_A2 Erweiterung input
    PMC_PCER0 = PMC_PCER0_PID14; // (1<<14) turn on PIO D in PMC to activate the clock

    PIOD_PER = 0x7;
    PIOD_ODR = PIOD_ODR_P0 | PIOD_ODR_P1 | PIOD_ODR_P2; // Initialize PIOD as input for pin 0-2

    NVIC->ISER[0] |= 1 << PIOD_ID;
    PIOD_IER = 0x7;
    
    // just test on off on off ....
    // PIOB_SODR = (1 << 27);
    // PIOB_CODR = (1 << 27);
     
    // PIOB_SODR = (1 << 27);
    // PIOB_CODR = (1 << 27);

    // PIOB_SODR = (1 << 27);
    // PIOB_CODR = (1 << 27);
    // PIOB_SODR = (1 << 27);
    // PIOB_CODR = (1 << 27);
    // PIOB_SODR = (1 << 27);
    // PIOB_CODR = (1 << 27);
    // PIOB_SODR = (1 << 27);
    // PIOB_CODR = (1 << 27);


    // TODO
}

void PIOD_Handler(void) {

    volatile unsigned int status = PIOD_ISR;

    if (status & 1) state.led = LED_STATE_ON;
    if (status & 2) state.led = LED_STATE_OFF;
    if (status & 4) state.led = LED_STATE_BLINKING;
   // else state.led = LED_STATE_INVALID;
}

// loop for LED control - blinking when EDA code is running
void led_loop(void)
{
    switch (state.led) // state.led
    {
    case LED_STATE_OFF:
        led_set(false);
        break;
    case LED_STATE_ON:
        led_set(true);
        break;
    case LED_STATE_BLINKING:
        led_blinking();
        break;
    case LED_STATE_INVALID:
        __builtin_unreachable();
    }
    value.led = led_get();
}