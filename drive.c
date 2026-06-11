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

void drive_init()
{
    // Step 1   Turn On Clock
    PMC_PCER0 = 1u << TC6_ID; // tc6 id is 33

    // Step 2
    PIOC_PER = PIOC_PER_P23 | PIOC_PER_P24; // enable register for PIOC23 and 24
    PIOC_OER = PIOC_PER_P23 | PIOC_PER_P24; // set as output register for PIOC23 and 24
    PIOC_CODR = PIOC_PER_P23 | PIOC_PER_P24; // clear output datas register for PIOC23 and 24

    //PWM Output
    PIOC_PDR = PIOC_PDR_P25; // disable register -> give the pin, peripheral takes it
    PIOC_ABSR |= PIOC_ABSR_P25; // choose peripheral B. TIOA6 is at B

    TC2_CCR0 = TC_CCR_CLKDIS;  // Erstmal Clock stoppen — sicher ist sicher
    TC2_CCR0 = TC_CCR_SWTRG;   // Software Reset des Counters

    // Step 3   Configure TC2
    TC2_CMR0 =
        TC_CMR_WAVE           |  // Wave Mode (nicht Capture)
        TC_CMR_WAVSEL_UP_RC   |  // Zähle hoch, Reset bei RC
        TC_CMR_ACPA_CLEAR     |  // Bei RA: TIOA → LOW
        TC_CMR_ACPC_SET       |  // Bei RC: TIOA → HIGH
        TC_CMR_EEVT_XC0       |  // Externes Event = XC0 (damit TIOB als Output frei ist)
        TC_CMR_TCCLKS_TIMER_CLOCK2; // Prescaler MCK/8


    // Step 4   Set RA, RC
    TC2_RC0 = 1050;   // Periode → 10,5 MHz / 1050 = 10 kHz PWM
    TC2_RA0 = 0;      // Duty Cycle = 0% beim Start (Motor steht)

    // Step 5   Start Timer
    TC2_CCR0 = TC_CCR_CLKEN | TC_CCR_SWTRG;


    // TODO
}

void drive_loop()
{
    if (state.motion == MOTION_STATE_ON)
    {
        PIOC_PDR = PIOC_PDR_P25;
    }
    else
    {
        PIOC_PER = PIOC_PER_P25;
    }

    // TODO

    if (!(PIOC_ODSR & PIOC_PDSR_P23) && (PIOC_ODSR & PIOC_PDSR_P24))
    {
        value.motion =  100. * TC2_RA0 / TC2_RC0;
    }
    else if ((PIOC_ODSR & PIOC_ODSR_P23) && !(PIOC_ODSR & PIOC_PDSR_P24))
    {
        value.motion = -100. * TC2_RA0 / TC2_RC0;
    }
    else
    {
        value.motion = 0;
    }
}
