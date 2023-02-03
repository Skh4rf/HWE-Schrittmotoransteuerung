/*
 * jm_steppertmc2209.c
 *
 * Created: 09.01.2023 14:36:05
 * Author : Skh4rf 
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define HSCALER 99 // OCR0=99 mit 64 Vorteiler --> 1876Hz Interruptfrequenz (--> Signalfrequenz = 1876/2 = 938Hz)

#define OUTPORTDDR DDRB // Datenrichtungsregister des Clock und Dir Pin
#define OUTPORT PORTB	// Portregister des Clock und Dir Pin
#define CLOCKPIN PB3	// Clock Pin
#define DIRPIN PB2		// Dir Pin

int16_t steps = 0;				// Schrittzähl-Variable
int16_t nsteps = 0;				// Schritt-Variable für NStep Routinen
static int8_t stepsInkrementor; // Schrittinkrementor Variable (Vorwärts = 1, Rückwärts = -1)
static int8_t swScaler = 0;		// Software-Scaler für Frequenzanpassung
static uint8_t counter = 0;		// Timer-Counter für Softwarefrequenzanpassung

/**************************************************************************************/
/* Interrupt-Service-Routine der Signalerzeugung und Schrittzählung					  */
/* Falls nsteps nicht 0 --> automatisch Schrittmodus								  */
/*																					  */
/* swScaler - Variable kann zur Frequenzänderung verwendet werden (nicht linear)	  */
ISR (TIMER0_COMP_vect){	// Timer Compare Match Interrupt-Routine
	if (counter == swScaler){					// Softwareteiler erreicht?
		if(nsteps != 0 && nsteps <= steps){		// Falls Betrieb mit vorgeschriebener Schrittzahl und Schrittzahl erreicht
			nsteps = 0;							// vorgeschriebene Schrittzahl zurücksetzen (Warteschleife in NSteps-Routinen wird abgebrochen)
			TCCR0 = 0x00;						// Timer0 stoppen
		}else{
			OUTPORT ^= (1<<CLOCKPIN);			// Clock-Pin Toggeln zur Frequenzerzeugung
			counter = 0;						// Softwareteiler-Zählvariable zurücksetzen
			steps += stepsInkrementor;			// Schrittzähler mit stepsInkrementor in- bzw. dekrementieren
		}
	}else{counter++;}							// Softwareteiler-Zählvariable inkrementieren
}

/**************************************************************************************/
/* Initialisieren der Clock zur Signalerzeugung am Clk-Pin							  */
/* Timer0 -> Mode: CTC, Vorteiler 64, Interruptsfreigabe (global & spezifisch)		  */
/*																					  */
/* HSCALER kann zur Veränderung der Signalfrequenz verwendet werden					  */
void clockInit(){
	OUTPORTDDR |= (1<<CLOCKPIN);				// Clock-Pin-Datenrichtung als Ausgang definieren
	OCR0 = HSCALER;								// OCR0 auf Hardwarescaler-Define setzen
	TCCR0 = (1<<CS00) | (1<<CS01) | (1<<WGM01); // Timer0 konfigurieren --> Mode: CTC, Vorteiler: 64
	TIMSK = (1<<OCIE0);							// OCIE0-Match Interrupt spezifisch freigeben
	sei();										// Interrupts global freigeben
}

/**************************************************************************************/
/* Motorbewegung stoppen															  */
/*																					  */
/* Rückgabe: Schritte der letzten Aktion bis zur Ausführung dieser Routine			  */
/*		(Rückwärts < 0 < Vorwärts)							    					  */
int16_t moveStop(){								
	OUTPORT &= ~(1<<CLOCKPIN);	// Clock-Pin auf Low-Level setzen
	TCCR0 = 0x00;				// Timer0 stoppen
	int16_t s = steps;			// Schrittzählvariable zwischenspeichern
	steps = 0;					// Schrittzählvariable zurücksetzen
	return s;					// gezählte Schritte der letzten Bewegung zurückgeben (Rückwärts < 0 < Vorwärts)
}

/**************************************************************************************/
/* Motorbewegung vorwärts bis zu neuer Anweisung									  */
/*																					  */
/* Rückgabe: Schritte der letzten Aktion bis zur Ausführung dieser Routine			  */
/*		(Rückwärts < 0 < Vorwärts)							    					  */
int16_t moveForward(){
	int16_t s = moveStop();		// Motor stoppen und Schrittzahl der letzten Bewegung zwischenspeichern
	OUTPORTDDR |= (1<<DIRPIN);	// Richtungs-Pin im Datenrichtungsregister als Ausgang definieren
	OUTPORT |= (1<<DIRPIN);     // Richtungs-Pin auf High setzen
	stepsInkrementor = 1;		// Schrittzähl-Inkrementor auf 1 setzen
	clockInit();				// Timer0 initialisieren (--> Bewegung starten)
	return s;					// Schrittzahl der letzten Bewegung zurückgeben (Rückwärts < 0 < Vorwärts)
}

/**************************************************************************************/
/* Motorbewegung rückwärts bis zu neuer Anweisung									  */
/*																					  */
/* Rückgabe: Schritte der letzten Aktion bis zur Ausführung dieser Routine			  */
/*		(Rückwärts < 0 < Vorwärts)							    					  */
int16_t moveBackward(){			
	int16_t s = moveStop();		// Motor stoppen und Schrittzahl der letzen Bewegung zwischenspeichern
	OUTPORTDDR |= (1<<DIRPIN);  // Richtungs-Pin im Datenrichtungsregister als Ausgang definieren
	OUTPORT &= ~(1<<DIRPIN);    // Richtungs-Pin auf Low setzen
	stepsInkrementor = -1;		// Schrittzähl-Inkrementor auf -1 setzen
	clockInit();				// Timer0 initialisieren (--> Bewegung starten)
	return s;					// Schrittzahl der letzen Bewegung zurückgeben (Rückwärts < 0 < Vorwärts)
}

/**************************************************************************************/
/* Motorbewegung vorwärts für n Schritte (bzw. Mikroschritte je nach Treiber config)  */
/*																					  */
/* Eingabeparameter: Auszuführende Schrittanzahl									  */
void moveNStepsForward(uint16_t n){
	moveStop();							// Motor stoppen
	OUTPORTDDR |= (1<<DIRPIN);			// Richtungs-Pin im Datenrichtungsregister als Ausgang definieren
	OUTPORT |= (1<<DIRPIN);				// Richtungs-Pin auf High setzen
	stepsInkrementor = 1;				// Schrittzähl Inkrementor auf 1 setzen
	nsteps = n;							// Soll-Schrittvariable auf übergebenen Wert setzen
	clockInit();						// Timer0 initialiseren (--> Bewegung starten)
	while(nsteps != 0){_delay_us(1);}	// warten bis Schritte gefahren wurden
	moveStop();							// Bewegung stoppen
}

/**************************************************************************************/
/* Motorbewegung rückwärts für n Schritte (bzw. Mikroschritte je nach Treiber config) */
/*																					  */
/* Eingabeparameter: Auszuführende Schrittanzahl									  */
void moveNStepsBackward(uint16_t n){
	moveStop();							// Motor stoppen
	OUTPORTDDR |= (1<<DIRPIN);			// Richtungs-Pin im Datenrichtungsregister als Ausgang definieren
	OUTPORT &= ~(1<<DIRPIN);			// Richtungs-Pin auf Low setzen
	stepsInkrementor = 1;				// Schrittzähl Inkrementor auf 1 setzen
	nsteps = n;							// Soll-Schrittvariable auf übergebenen Wert setzen
	clockInit();						// Timer0 initialisieren (--> Bewegung starten) 
	while(nsteps != 0){_delay_us(1);}	// warten bis Schritte gefahren wurden
	moveStop();							// Bewegung stoppen
}

