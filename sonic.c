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


// volatile nötig weil Variable außerhalb des Handlers gelesen wird
// ohne volatile könnte der Compiler den Wert wegoptimieren
volatile uint32_t distance_cm = 0;
volatile uint8_t  new_measurement = 0; // Flag: neue Messung liegt vor


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
    // TO DO
    // Aufgabe 1
    // --- TC7: Wave-Modus, Trigger-Signal an TIOB7 / PIOC29 ---

    // 1) Pin PIOC29 als Peripherie B freischalten (TIOB7)
    PIOC_PDR  =  1u << 29;   // PIO gibt Pin-Kontrolle ab
    PIOC_ABSR |= 1u << 29;   // Peripheral B auswählen (= TIOB7)

    // 2) Clock für TC7 aktivieren (ID=34 → PCER1)
    PMC_PCER1 = 1u << TC7_ID % 32;

    // 3) Timer stoppen vor Konfiguration
    TC2_CCR1 = TC2_CCR1_CLKDIS;

    // 4) Wave-Modus konfigurieren
    TC2_CMR1 =
        TC2_CMR1_TCCLKS_TIMER_CLOCK1 |  // Prescaler MCK/2
        TC2_CMR1_WAVE                |  // Wave-Modus
        TC2_CMR1_WAVSEL_UP_RC        |  // Reset bei RC
        TC2_CMR1_EEVT_XC0            |  // TIOB als Ausgang
        TC2_CMR1_BSWTRG_SET          |  // HIGH beim Software-Trigger
        TC2_CMR1_BCPB_CLEAR          |  // LOW bei RB (~10 µs)
        TC2_CMR1_BCPC_SET;              // HIGH bei RC (neuer Zyklus)

    // 5) Zeitwerte setzen
    //TC2_RB1 = 27;       // ~10 µs High-Impuls
    //TC2_RC1 = 157500;   // ~60 ms Periode (oder 52500 für 20 ms)

    TC2_RC1 = MCK2 / 10;        // (1 / 10)s = 100ms
    TC2_RB1 = MCK2 / 100000;    // (1 / 100000)s = 10us

    // 6) Timer starten
    TC2_CCR1 = TC2_CCR1_CLKEN | TC2_CCR1_SWTRG;


}

static void echo_init(void)
{
    // TODO
    PIOD_PDR = 1u << 7;  //pio control ein
    PIOD_ABSR |= 1u << 7;   

    PMC_PCER1 = PMC_PCER1_PID35;


    //timer stppen zum konfigurieren
    TC2_CCR2 = TC2_CCR2_CLKDIS;

    //capture mode (wave = 0)
    TC2_CMR2 = TC2_CMR2_TCCLKS_TIMER_CLOCK3
             | TC2_CMR2_LDRA_RISING //RA bei dteigender Flanke
             | TC2_CMR2_LDRB_FALLING //RB bei sinkender flanke
             | TC2_CMR2_LDBSTOP //Stoppe der counter wenn rb geladen wird 
             //| TC2_CMR2_CPCTRG; //Zurücksetzen durch RC , falls nicht gestoppt durch ldbstop
             | TC2_CMR2_ABETRG; //TIOA als externer trigger


    TC2_RC2 = 0xFFFFFFFF;

    #if USE_INTERRUPT
        TC2_IDR2 = 0xFFFFFFFF;  // Interupts deaktivieren 
        TC2_IER2 = TC2_IER2_LDRBS; //aktiviert den Interupt für den load event
        NVIC_EnableIRQ(TC8_ID); //NVIC->ISER[1] |= (1<<(TC8_ID % 32));
    #endif
    TC2_CCR2 = TC2_CCR2_CLKEN | TC2_CCR2_SWTRG;
}


void TC8_Handler(void) {

    // 1) SR lesen – PFLICHT!
    //    Löscht alle Flags, verhindert sofortige erneute Auslösung
    uint32_t status = TC2_SR2;

    // 2) Prüfen ob wirklich LDRBS ausgelöst hat
    if (status & TC2_SR2_LDRBS) {

        // 3) Messwerte auslesen
        uint32_t ra = TC2_RA2;   // Echo-Start
        uint32_t rb = TC2_RB2;   // Echo-Ende

   
        // 4) Distanz berechnen
        uint32_t ticks = rb - ra;
        // Distanz [cm] = ticks * 32 * 340 * 100 / (84.000.000 * 2)

        // 5) Ergebnis global speichern für Aufgabe 4
        //zeitdauer in sekunden 
        double zeit = (double)ticks/ (double)MCK32;

        //entfernung in meter
        double dist_meter = zeit * (double)SONIC_SPEED/2;

        //Entfernung in mm
        double dist_Millimeter = dist_meter*1000;

        int32_t dist_mm_INT32 = (int32_t)dist_Millimeter;

        value.sonic.distance = dist_mm_INT32; // save in global state
        value.sonic.new = true;
    }

    // 6) Neue Messung starten für nächsten Zyklus
    TC2_CCR2 = TC2_CCR2_CLKEN | TC2_CCR2_SWTRG;
}


void sonic_init(void)
{
    // TODO

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


// 1) Statische Hilfsfunktion definieren – vor sonic_loop()
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
