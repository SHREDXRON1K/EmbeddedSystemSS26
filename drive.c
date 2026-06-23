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

static unsigned int distance_average(unsigned int new_value)    // take 5 distance values to smoothen the calculation
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
    PMC_PCER1 = 1u << (TC6_ID % 32); // tc6 id is 33, should be placed in register PCER1

    // Step 2
    PIOC_PER = PIOC_PER_P25; // enable register for PIOC23 and 24
    PIOC_OER = PIOC_PER_P25; // set as output register for PIOC23 and 24
    PIOC_CODR = PIOC_PER_P25; // clear output data register for PIOC23 and 24

    PIOC_PER = PIOC_PER_P23 | PIOC_PER_P24; // enable register for PIOC23 and 24
    PIOC_OER = PIOC_PER_P23 | PIOC_PER_P24; // set as output register for PIOC23 and 24
    PIOC_CODR = PIOC_PER_P23 | PIOC_PER_P24; // clear output datas register for PIOC23 and 24

    //PWM Output
    PIOC_ABSR |= PIOC_ABSR_P25; // choose peripheral B. TIOA6 is at B
    TC2_CCR0 = TC2_CCR0_CLKDIS;  // Disable CLK, security!
    TC2_CCR0 = TC2_CCR0_SWTRG;   // Software Reset the Counter

    // Step 3   Configure TC2
    TC2_CMR0 =
        TC2_CMR0_WAVE           |  // Wave Mode
        TC2_CMR0_WAVSEL_UP_RC   |  // count++, Reset when ==RC
        TC2_CMR0_ACPA_CLEAR     |  // when RA: TIOA LOW
        TC2_CMR0_ACPC_SET       |  // when RC: TIOA HIGH
        TC2_CMR0_EEVT_XC0       |  // External Event = XC0 (so TIOB as Output is free)
        TC2_CMR0_TCCLKS_TIMER_CLOCK2; // Prescaler MCK/8

    // // Step 4   Set RA, RC
    //PWM Frequency (schnelles Ein- und Ausschalten, um die Motorleistung zu steuern.)
    uint32_t pwm = 100;   // f in hz    // why 100? In Documentation!
    TC2_RC0 = (MCK/8) / pwm;  // Value for register RC
    TC2_RA0 = 0; // Start value Register RA


    // Step 5   Start Timer
    TC2_CCR0 = TC2_CCR0_CLKEN | TC2_CCR0_SWTRG;     // Software enable | Software Trigger
    PIOC_PDR = PIOC_PDR_P25;    // disable P25?
}

void drive_loop()
{

    int32_t smoothed_distance = (int32_t)distance_average(value.sonic.distance);    // take the average from 5 distance values
    int32_t goal_distance = (int32_t)value.goal;

    int32_t dist_mm = (smoothed_distance - goal_distance);  // distance shown in matrix : x left to meet the target 
    int32_t dist_cm = dist_mm / 10;

    int32_t motionValue = 0; // this variable will be shown on minicom (m motion) 

    if (dist_cm > 64) 
    {   // overflow in led matrix (? will be shown)
        motionValue = MOTION_MAX;
        value.motion = motionValue;
    }
    else 
    {
        motionValue = dist_cm;  // the easiest way to make it slower and slower...
        // these ifs are to hinder the deadzone motor problem
        if (motionValue <= MOTION_MIN && motionValue > 0) motionValue = MOTION_MIN;  // when value > 0
        if (motionValue >= -MOTION_MIN && motionValue < 0) motionValue = -MOTION_MIN;  // when value < 0
        if (motionValue == 0) motionValue = MOTION_OFF;  // when value 0
        value.motion = motionValue;
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


    // Richtung setzen
    if(state.motion == MOTION_STATE_ON && motionValue > 0){  // always check if state.motion is ON 
        //IN1 0 IN2 1   01-> vorwärts
        PIOC_CODR = PIOC_CODR_P23;
        PIOC_SODR = PIOC_SODR_P24;
    }
    else if(state.motion == MOTION_STATE_ON && motionValue < 0){ // always check if state.motion is ON 
        //IN1 1 IN2 0   10-> rückwärts
        PIOC_SODR = PIOC_SODR_P23;
        PIOC_CODR = PIOC_CODR_P24;
        motionValue = -motionValue; // set with minimum torque to hinder deadzone problem // to control the backwards speed is not implemented here actually 
    }
    else{
        //IN1 0 IN2 0   00-> stop
        PIOC_CODR = PIOC_CODR_P23 | PIOC_CODR_P24;
        TC2_RA0 = 0;
        return;
    }

    int duty_int = 0;
    duty_int = TC2_RC0 * motionValue / 100;  // value motion in percentage (that's why divided by 100)
    TC2_RA0 = duty_int;

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

}
