/*
 * Metzler_J_4chel_HWE_Stepper_Scope1.c
 *
 * Created: 17.01.2023 10:05:32
 * Author : Jakob
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

static int count = 0; // statische Zählvariable zur Softwareteilung der Frequenz

/**************************************************************************************/
/* Interrupt-Service-Routine für 1kHz Signal an Pin PB3								  */
ISR (TIMER0_COMP_vect){		// Timer Compare Match Interrupt-Routine
	count++;				// Zählvariable inkrementieren
	if (count >= 100){		// Wurde Zählvariable 100-mal inkrementiert?
		PORTB ^= (1<<PB3);	// Pin PB3 Toggeln
		count = 0;			// Zählvariable zurücksetzen
	}
}

/**************************************************************************************/
/* Initialisiert Timer0 zur Clockausgabe an Pin PB3 (f=1kHz)						  */
void clockInit(){					// 200kHz Interrupt Frequenz 
	DDRB |= (1<<PB3);				// Clock-Pin-Datenrichtung auf Ausgang
	OCR0 = 59;						// CTC-Mode OCR0=59 mit 1 Vorteiler --> 200kHz Interruptfrequenz
	TCCR0 = (1<<CS00) | (1<<WGM01); // Mode: CTC, Vorteiler: 1
	TIMSK = (1<<OCIE0);				// OCIE0-Match interrupt spezifisch freigeben
	sei();							// Interrupts global freigeben
}

/**************************************************************************************/
/* Routine um Motor mit konstanter Frequenz zu betreiben (PIN_CLK=PB3; f=1kHz)		´ */
void moveForward(){
	clockInit(); // 1kHz auf Pin PB3
}

int main(void)
{
    moveForward(); // Motor mit konstanter Frequenz in eine Richtung betreiben
    while (1) 
    {
		;
    }
}

