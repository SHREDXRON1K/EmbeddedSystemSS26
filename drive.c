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
 * FILE: drive.h
 * CONTENTS: implementation for all functions for PWM motor drive
 */

#include <drive.h>

#include <constants.h>
#include <state.h>
#include <sam.h>

// tuning the motor to hinder the deadzone motion problem
#define MOTION_MAX 100 
#define MOTION_MIN 20
#define MOTION_OFF 0

#define AVG_WINDOW 5

static unsigned int distance_history[AVG_WINDOW] = {0};
static unsigned int history_index = 0;
static unsigned int history_count = 0;

static unsigned int distance_average(unsigned int new_value)
{
    distance_history[history_index] = new_value;
    history_index = (history_index + 1) % AVG_WINDOW;

    if (history_count < AVG_WINDOW)
        history_count++;

    unsigned int sum = 0;
    for (unsigned int i = 0; i < history_count; i++)
        sum += distance_history[i];

    return sum / history_count;
}


void drive_init()
{
    // Step 1   Turn On Clock
    PMC_PCER0 = 1u << PIOC_ID; // =======
    PMC_PCER1 = 1u << (TC6_ID % 32); // tc6 id is 33 // ID is 33, shoulnt it be PCER1 ???

    // Step 2
    PIOC_PER = PIOC_PER_P25; // enable register for PIOC23 and 24
    PIOC_OER = PIOC_PER_P25; // set as output register for PIOC23 and 24
    PIOC_CODR = PIOC_PER_P25; // clear output datas register for PIOC23 and 24


    PIOC_PER = PIOC_PER_P23 | PIOC_PER_P24; // enable register for PIOC23 and 24
    PIOC_OER = PIOC_PER_P23 | PIOC_PER_P24; // set as output register for PIOC23 and 24
    PIOC_CODR = PIOC_PER_P23 | PIOC_PER_P24; // clear output datas register for PIOC23 and 24

    //PWM Output
    //PIOC_PDR = PIOC_PDR_P25; // disable register -> give the pin, peripheral takes it
    PIOC_ABSR |= PIOC_ABSR_P25; // choose peripheral B. TIOA6 is at B

    TC2_CCR0 = TC2_CCR0_CLKDIS;  // Erstmal Clock stoppen — sicher ist sicher
    TC2_CCR0 = TC2_CCR0_SWTRG;   // Software Reset des Counters

    // Step 3   Configure TC2
    TC2_CMR0 =
        TC2_CMR0_WAVE           |  // Wave Mode (nicht Capture)
        TC2_CMR0_WAVSEL_UP_RC   |  // Zähle hoch, Reset bei RC
        TC2_CMR0_ACPA_CLEAR     |  // Bei RA: TIOA → LOW
        TC2_CMR0_ACPC_SET       |  // Bei RC: TIOA → HIGH
        TC2_CMR0_EEVT_XC0       |  // Externes Event = XC0 (damit TIOB als Output frei ist)
        TC2_CMR0_TCCLKS_TIMER_CLOCK2; // Prescaler MCK/2


    // // Step 4   Set RA, RC
    // TC2_RC0 = 1050;   // Periode → 10,5 MHz / 1050 = 10 kHz PWM
    // TC2_RA0 = 0;      // Duty Cycle = 0% beim Start (Motor steht)


    // von Karesse

    //PWM Frequenz festlegen (schnelles Ein- und Ausschalten, um die Motorleistung zu steuern.)
    uint32_t pwm = 100;   // f in hz    // gotta be careful, dont set to slow otherwise the motor will burn

    //startwerte festlegen und timer starten
    TC2_RC0 = (MCK/8) / pwm;  //Rc bestimmt die PMW frequenz 

    TC2_RA0 = 0; //TC2_RC0 * (value.motion)/ 100 ; //RA bestimmt den duty cycle (duty cycle 30 %), value motion in percentage (that's why divided by 100)




    // Step 5   Start Timer
    TC2_CCR0 = TC2_CCR0_CLKEN | TC2_CCR0_SWTRG;
    PIOC_PDR = PIOC_PER_P25; // enable register for PIOC23 and 24

    // TODO
}

void drive_loop()
{

    // von Karesse
    int32_t smoothed_distance = (int32_t)distance_average(value.sonic.distance);
    int32_t goal_distance = (int32_t)value.goal;

    int32_t dist_mm = (smoothed_distance - goal_distance);
    int32_t dist_cm = dist_mm / 10;

    int32_t motionWert = 0; // this variable will be shown on minicom (m motion) 

    if(dist_cm > 64 ){
        motionWert = MOTION_MAX;
        value.motion = motionWert;
    }
    else{
        motionWert = dist_cm;

        if (motionWert <= MOTION_MIN && motionWert > 0) motionWert = MOTION_MIN;  // to hinder the deadzone motor problem
        if (motionWert == 0) motionWert = MOTION_OFF;  // to hinder the deadzone motor problem

        value.motion = motionWert;
    }


    // ==== BAUSTEIN =====
    if (state.motion == MOTION_STATE_ON)
    {
        PIOC_PDR = PIOC_PDR_P25;
    }
    else
    {
        PIOC_PER = PIOC_PER_P25; // pwm signal
        PIOC_CODR = PIOC_CODR_P23 | PIOC_CODR_P24;  // activate IN1 & IN2
        TC2_RA0 = 0; // set RA as 0
    }
    // ==== BAUSTEIN =====


    // von Karesse
    // Richtung setzen
    //float motionWert = value.motion;

    if(state.motion == MOTION_STATE_ON && motionWert > 0){  // always check if state.motion is ON 
        //IN1 0 IN2 1   01-> vorwärts
        PIOC_CODR = PIOC_CODR_P23;
        PIOC_SODR = PIOC_SODR_P24;
    }
    else if(state.motion == MOTION_STATE_ON && motionWert < 0){ // always check if state.motion is ON 
        //IN1 1 IN2 0   10-> rückwärts
        PIOC_SODR = PIOC_SODR_P23;
        PIOC_CODR = PIOC_CODR_P24;
        motionWert = -1 * 10; // set with minimum torque to hinder deadzone problem
    }
    else{
        //IN1 0 IN2 0   00-> stop
        PIOC_CODR = PIOC_CODR_P23 | PIOC_CODR_P24;
        TC2_RA0 = 0;
        return;
    }

    // TODO

    // ==== BAUSTEIN =====
    // if (!(PIOC_ODSR & PIOC_PDSR_P23) && (PIOC_ODSR & PIOC_PDSR_P24))
    // {
    //     value.motion =  100. * TC2_RA0 / TC2_RC0;
    // }
    // else if ((PIOC_ODSR & PIOC_ODSR_P23) && !(PIOC_ODSR & PIOC_PDSR_P24))
    // {
    //     value.motion = -100. * TC2_RA0 / TC2_RC0;
    // }
    // else
    // {
    //     value.motion = 0;
    // }
    // ==== BAUSTEIN =====


    //float duty = (motionWert / 100.0f) * (float)TC2_RC0;    // value motion in percentage (that's why divided by 100)
    
    //TC2_RA0 = (uint32_t)duty;
    int duty_int = 0;
    duty_int = TC2_RC0 * motionWert / 100;  // value motion in percentage (that's why divided by 100)
    TC2_RA0 = duty_int;

}
