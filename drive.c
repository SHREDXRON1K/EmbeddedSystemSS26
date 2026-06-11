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
