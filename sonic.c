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
 * FILE: sonic.h
 * CONTENTS: declaration of sonic driver using ultrasonic sensor HC-SR04
 */

#include <sonic.h>

#include <constants.h>
#include <sam.h>
#include <state.h>

#include <matrix.h>

#define USE_INTERRUPT 1

// Measurements restarted with this frequency of trigger pulse
#define TRIGGER_FREQUENCY 10

// Sonic speed is about 343 m/s at 20 degrees Celsius
#define SONIC_SPEED 343


// volatile needed since variable outside the handler will be read
// without volatile : compiler could not optimize the value
volatile uint32_t distance_cm = 0;
volatile uint8_t  new_measurement = 0; // Flag: new Measurement


static inline void trigger_enable(void)
{
    PIOC_PDR = PIOC_PDR_P29;
}

static inline void trigger_disable(void)
{
    PIOC_PER = PIOC_PER_P29;
}

static void trigger_init(void)
{
    // Aufgabe 1
    // --- TC7: Wave-Mode, Trigger-Signal TIOB7 / PIOC29 ---

    // 1) make Pin PIOC29 as Peripheral B available (TIOB7)
    PIOC_PDR  =  1u << 29;   // PIO release Pin Control
    PIOC_ABSR |= 1u << 29;   // choose Peripheral B (= TIOB7)

    // 2) activate Clock for TC7 (ID=34 -> PCER1)
    PMC_PCER1 = 1u << (TC7_ID % 32);

    // 3) stop Timer before Konfiguration
    TC2_CCR1 = TC2_CCR1_CLKDIS;

    // 4) configure Wave-Modus
    TC2_CMR1 =
        TC2_CMR1_TCCLKS_TIMER_CLOCK1 |  // Prescaler MCK/2
        TC2_CMR1_WAVE                |  // Wave-Mode
        TC2_CMR1_WAVSEL_UP_RC        |  // Reset when ==RC
        TC2_CMR1_EEVT_XC0            |  // TIOB as Ausgang
        TC2_CMR1_BSWTRG_SET          |  // HIGH when Software-Trigger
        TC2_CMR1_BCPB_CLEAR          |  // LOW when ==RB (~10 µs)
        TC2_CMR1_BCPC_SET;              // HIGH when ==RC (neuer Zyklus)

    // 5) set time values (RB, RC)
    TC2_RC1 = MCK2 / 10;        // (1 / 10)s = 100ms
    TC2_RB1 = MCK2 / 100000;    // (1 / 100000)s = 10us

    // 6) Timer starten
    TC2_CCR1 = TC2_CCR1_CLKEN | TC2_CCR1_SWTRG;
}

static void echo_init(void)
{
    PIOD_PDR = 1u << 7;  //pio control on
    PIOD_ABSR |= 1u << 7;   

    PMC_PCER1 = PMC_PCER1_PID35;

    //stop timer to configure
    TC2_CCR2 = TC2_CCR2_CLKDIS;

    //capture mode (wave = 0)
    TC2_CMR2 = TC2_CMR2_TCCLKS_TIMER_CLOCK3
             | TC2_CMR2_LDRA_RISING //RA at rising
             | TC2_CMR2_LDRB_FALLING //RB at sinking
             | TC2_CMR2_LDBSTOP //stop the counter when rb is loaded 
             //| TC2_CMR2_CPCTRG; //reset through RC , in case not stopped by ldbstop
             | TC2_CMR2_ABETRG; //TIOA as external trigger

    TC2_RC2 = 0xFFFFFFFF;   // deactivate RC2

    #if USE_INTERRUPT
        TC2_IDR2 = 0xFFFFFFFF;  // deactive Interupts 
        TC2_IER2 = TC2_IER2_LDRBS; //activate the Interupt for the load event
        NVIC_EnableIRQ(TC8_ID); //NVIC->ISER[1] |= (1<<(TC8_ID % 32)); // TC8_ID is actually > 32, but the function itself already handles it
    #endif

    TC2_CCR2 = TC2_CCR2_CLKEN | TC2_CCR2_SWTRG; // activate
}


void TC8_Handler(void) {

    // 1) read SR!
    //    remove all Flags, to hinder sudden updated Auslösung
    uint32_t status = TC2_SR2;

    // 2) check LDRBS really ausgelöst
    if (status & TC2_SR2_LDRBS) {

        // 3) read measurement
        uint32_t ra = TC2_RA2;   // Echo-Start
        uint32_t rb = TC2_RB2;   // Echo-Ende
   
        // 4) count the time
        uint32_t ticks = rb - ra;

        // 5) save result in global for Aufgabe 4
        double time = (double)ticks / (double)MCK32;    // time in second // using double at first only for precision
        double distance_meter = time * (double)SONIC_SPEED/2;
        double distance_millimeter = distance_meter*1000;
        int32_t distance_millimeter_INT32 = (int32_t)distance_millimeter;
        value.sonic.distance = distance_millimeter_INT32; // save in global state in millimeter
        value.sonic.new = true; // update state
    }

    TC2_CCR2 = TC2_CCR2_CLKEN | TC2_CCR2_SWTRG;    // 6) start new Measurement for next Cycle
}


void sonic_init(void)
{
    trigger_init();
    echo_init();
}

static void trigger_loop(void)
{
    if (state.sonic == SONIC_STATE_OFF)
        trigger_disable();
    if (state.sonic == SONIC_STATE_ON)
        trigger_enable();
}


// 1) static define addtion function
static void display_loop(void)
{
    if (state.sonic != SONIC_STATE_ON)
        return;

    if (value.sonic.new == false)
        return;

    value.sonic.new = false;
}


void sonic_loop(void)
{
#if USE_INTERRUPT
    // Interrupt mode
    trigger_loop();
    display_loop();

// #else
//     // Polling mode
//     uint32_t timeout;

//     TC2_CCR2 = TC2_CCR2_CLKEN | TC2_CCR2_SWTRG;

//     timeout = 200000;
//     while (!(TC2_SR2 & TC2_SR2_LDRAS)) {
//         if (--timeout == 0) {
//             value.sonic.new = false;
//             return;
//         }
//     }

//     timeout = 200000;
//     while (!(TC2_SR2 & TC2_SR2_LDRBS)) {
//         if (--timeout == 0) {
//             value.sonic.new = false;
//             return;
//         }
//     }

//     uint32_t ra    = TC2_RA2;
//     uint32_t rb    = TC2_RB2;
//     uint32_t ticks = rb - ra;

//     double zeit          = (double)ticks / (double)MCK32;
//     double dist_meter    = zeit * (double)SONIC_SPEED / 2.0;
//     value.sonic.distance = dist_meter * 1000.0;
//     value.sonic.new      = true;

//     trigger_loop();
//     echo_loop();
//     display_loop();

#endif
}
