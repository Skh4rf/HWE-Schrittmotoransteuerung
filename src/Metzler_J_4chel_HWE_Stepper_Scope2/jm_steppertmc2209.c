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

int16_t steps = 0;				// Schrittz�hl-Variable
int16_t nsteps = 0;				// Schritt-Variable f�r NStep Routinen
static int8_t stepsInkrementor; // Schrittinkrementor Variable (Vorw�rts = 1, R�ckw�rts = -1)
static int8_t swScaler = 0;		// Software-Scaler f�r Frequenzanpassung
static uint8_t counter = 0;		// Timer-Counter f�r Softwarefrequenzanpassung

/**************************************************************************************/
/* Interrupt-Service-Routine der Signalerzeugung und Schrittz�hlung					  */
/* Falls nsteps nicht 0 --> automatisch Schrittmodus								  */
/*																					  */
/* swScaler - Variable kann zur Frequenz�nderung verwendet werden (nicht linear)	  */
ISR (TIMER0_COMP_vect){	// Timer Compare Match Interrupt-Routine
	if (counter == swScaler){					// Softwareteiler erreicht?
		if(nsteps != 0 && nsteps <= steps){		// Falls Betrieb mit vorgeschriebener Schrittzahl und Schrittzahl erreicht
			nsteps = 0;							// vorgeschriebene Schrittzahl zur�cksetzen (Warteschleife in NSteps-Routinen wird abgebrochen)
			TCCR0 = 0x00;						// Timer0 stoppen
		}else{
			OUTPORT ^= (1<<CLOCKPIN);			// Clock-Pin Toggeln zur Frequenzerzeugung
			counter = 0;						// Softwareteiler-Z�hlvariable zur�cksetzen
			steps += stepsInkrementor;			// Schrittz�hler mit stepsInkrementor in- bzw. dekrementieren
		}
	}else{counter++;}							// Softwareteiler-Z�hlvariable inkrementieren
}

/**************************************************************************************/
/* Initialisieren der Clock zur Signalerzeugung am Clk-Pin							  */
/* Timer0 -> Mode: CTC, Vorteiler 64, Interruptsfreigabe (global & spezifisch)		  */
/*																					  */
/* HSCALER kann zur Ver�nderung der Signalfrequenz verwendet werden					  */
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
/* R�ckgabe: Schritte der letzten Aktion bis zur Ausf�hrung dieser Routine			  */
/*		(R�ckw�rts < 0 < Vorw�rts)							    					  */
int16_t moveStop(){								
	OUTPORT &= ~(1<<CLOCKPIN);	// Clock-Pin auf Low-Level setzen
	TCCR0 = 0x00;				// Timer0 stoppen
	int16_t s = steps;			// Schrittz�hlvariable zwischenspeichern
	steps = 0;					// Schrittz�hlvariable zur�cksetzen
	return s;					// gez�hlte Schritte der letzten Bewegung zur�ckgeben (R�ckw�rts < 0 < Vorw�rts)
}

/**************************************************************************************/
/* Motorbewegung vorw�rts bis zu neuer Anweisung									  */
/*																					  */
/* R�ckgabe: Schritte der letzten Aktion bis zur Ausf�hrung dieser Routine			  */
/*		(R�ckw�rts < 0 < Vorw�rts)							    					  */
int16_t moveForward(){
	int16_t s = moveStop();		// Motor stoppen und Schrittzahl der letzten Bewegung zwischenspeichern
	OUTPORTDDR |= (1<<DIRPIN);	// Richtungs-Pin im Datenrichtungsregister als Ausgang definieren
	OUTPORT |= (1<<DIRPIN);     // Richtungs-Pin auf High setzen
	stepsInkrementor = 1;		// Schrittz�hl-Inkrementor auf 1 setzen
	clockInit();				// Timer0 initialisieren (--> Bewegung starten)
	return s;					// Schrittzahl der letzten Bewegung zur�ckgeben (R�ckw�rts < 0 < Vorw�rts)
}

/**************************************************************************************/
/* Motorbewegung r�ckw�rts bis zu neuer Anweisung									  */
/*																					  */
/* R�ckgabe: Schritte der letzten Aktion bis zur Ausf�hrung dieser Routine			  */
/*		(R�ckw�rts < 0 < Vorw�rts)							    					  */
int16_t moveBackward(){			
	int16_t s = moveStop();		// Motor stoppen und Schrittzahl der letzen Bewegung zwischenspeichern
	OUTPORTDDR |= (1<<DIRPIN);  // Richtungs-Pin im Datenrichtungsregister als Ausgang definieren
	OUTPORT &= ~(1<<DIRPIN);    // Richtungs-Pin auf Low setzen
	stepsInkrementor = -1;		// Schrittz�hl-Inkrementor auf -1 setzen
	clockInit();				// Timer0 initialisieren (--> Bewegung starten)
	return s;					// Schrittzahl der letzen Bewegung zur�ckgeben (R�ckw�rts < 0 < Vorw�rts)
}

/**************************************************************************************/
/* Motorbewegung vorw�rts f�r n Schritte (bzw. Mikroschritte je nach Treiber config)  */
/*																					  */
/* Eingabeparameter: Auszuf�hrende Schrittanzahl									  */
void moveNStepsForward(uint16_t n){
	moveStop();							// Motor stoppen
	OUTPORTDDR |= (1<<DIRPIN);			// Richtungs-Pin im Datenrichtungsregister als Ausgang definieren
	OUTPORT |= (1<<DIRPIN);				// Richtungs-Pin auf High setzen
	stepsInkrementor = 1;				// Schrittz�hl Inkrementor auf 1 setzen
	nsteps = n;							// Soll-Schrittvariable auf �bergebenen Wert setzen
	clockInit();						// Timer0 initialiseren (--> Bewegung starten)
	while(nsteps != 0){_delay_us(1);}	// warten bis Schritte gefahren wurden
	moveStop();							// Bewegung stoppen
}

/**************************************************************************************/
/* Motorbewegung r�ckw�rts f�r n Schritte (bzw. Mikroschritte je nach Treiber config) */
/*																					  */
/* Eingabeparameter: Auszuf�hrende Schrittanzahl									  */
void moveNStepsBackward(uint16_t n){
	moveStop();							// Motor stoppen
	OUTPORTDDR |= (1<<DIRPIN);			// Richtungs-Pin im Datenrichtungsregister als Ausgang definieren
	OUTPORT &= ~(1<<DIRPIN);			// Richtungs-Pin auf Low setzen
	stepsInkrementor = 1;				// Schrittz�hl Inkrementor auf 1 setzen
	nsteps = n;							// Soll-Schrittvariable auf �bergebenen Wert setzen
	clockInit();						// Timer0 initialisieren (--> Bewegung starten) 
	while(nsteps != 0){_delay_us(1);}	// warten bis Schritte gefahren wurden
	moveStop();							// Bewegung stoppen
}

