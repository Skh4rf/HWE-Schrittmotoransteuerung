/*
 * Metzler_J_4chel_HWE_Stepper_Scope2.c
 *
 * Created: 17.01.2023 10:44:18
 * Author : Jakob
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "jm_steppertmc2209.h"

int main(void)
{
	/**************************************************************************************/
	/* Ports konfigurieren																  */
	DDRA &= ~(0x0F);	// Taster-Pins als Eingang definieren
	PORTA |= 0x0F;		// Taster PullUp aktivieren
	DDRC |= 0xF0;		// PC7:4 als Ausgang definieren
	PORTC &= ~(0xF0);	// LED8:5 ausschalten
	
	/**************************************************************************************/
	/* Variablen zur Sequenzspeicherung definieren										  */
	static int16_t sequence[20];						// Seqence-Array definieren 
	for (int i = 0; i < 20; i++){ sequence[i] = 0;}		// Better-Save-Then-Sorry: um undefinierte Typen zu vermeiden
	uint16_t index = 0;									// Sequence-Index definieren

	while(1){
		/**************************************************************************************/
		/* Taste 0 gedr�ckt? --> Bewegung: Motor stoppen									  */
		if (!(PINA & (1<<PA0))){				// Taster PA0 gedr�ckt?
			/****Tasterentprellen****/
			_delay_ms(10);						// 10ms Delay zur Entprellung beim Dd�cken	
			while(!(PINA & (1<<PA0))) {;}		// Warten solange Taste gedr�ckt
			_delay_ms(10);						// 10ms Delay zur Entprellung beim loslassen
			/****Tasterentprellen****/
			
			if (index < 20){					// Sequenz-Array nicht voll?
				int16_t steps = moveStop();		// Motor stoppen und Schrittanzahl der vorherigen Bewegung zwischenspeichern
				if (steps != 0){				// Schrittanzahl nicht 0?
					sequence[index] = steps;	// Schrittanzahl bei aktuellem Index ins Sequence-Array speichern
					index++;					// Index inkrementieren
				}
			} 
			if (index > 19){					// Sequenz-Array voll?
				moveStop();						// Motor stoppen (sollte bereits der Fall sein)
				PORTC = 0xF0;					// LED8:5 einschalten (volle Sequenzspeicher signalisieren)
			}
			
		}
		
		/**************************************************************************************/
		/* Taste 1 gedr�ckt? --> Bewegung: Motor vorw�rts									  */
		if (!(PINA & (1<<PA1))){
			/****Tasterentprellen****/
			_delay_ms(10);						// 10ms Delay zur Entprellung beim Dd�cken
			while(!(PINA & (1<<PA1))) {;}		// Warten solange Taste gedr�ckt
			_delay_ms(10);						// 10ms Delay zur Entprellung beim loslassen
			/****Tasterentprellen****/
			
			if (index < 20){					// Sequenz-Array nicht voll?
				int16_t steps;					// Variable zur Zwischenspeicherung der Schrittanzahl definieren
				if(index != 19){				// Letzter Index nicht erreicht?
					steps = moveForward();		// Motor vorw�rts und Schrittanzahl der vorherigen Bewegung zwischenspeichern
				}
				else{							// Letzer Index erreicht?
					steps = moveStop();			// Motor stoppen und Schrittanzahl der vorherigen Bewegung zwischenspeichern
				}
				if (steps != 0){				// Schrittanzahl nicht 0?
					sequence[index] = steps;	// Schrittanzahl bei aktuellem Index ins Sequence-Array speichern
					index++;					// Index inkrementieren
				}
			}
			if (index > 19) {					// Sequenz-Array voll?
				moveStop();						// Motor stoppen (sollte ohnehin schon stehen)
				PORTC = 0xF0;					// LED8:5 einschalten (voller Sequenzspeicher signalisieren)
			}
		}
		
		/**************************************************************************************/
		/* Taste 2 gedr�ckt? --> Bewegung: Motor r�ckw�rts									  */
		if (!(PINA & (1<<PA2))){
			/****Tasterentprellen****/
			_delay_ms(10);						// 10ms Delay zur Entprellung beim Dr�cken
			while(!(PINA & (1<<PA2))) {;}		// Warten solange Taste gedr�ckt
			_delay_ms(10);						// 10ms Delay zur Entprellung beim loslassen
			/****Tasterentprellen****/
			
			if (index < 20){					// Sequenz-Array nicht voll?
				int16_t steps;					// Variable zur Zwischenspeicherung der Schrittanzahl definieren
				if(index != 19){				// Letzter Index nicht erreicht?
					steps = moveBackward();		// Motor r�ckw�rts und Schrittanzahl der vorherigen Bewegung zwischenspeichern
				}
				else{							// Letzer Index erreicht?
					steps = moveStop();			// Motor stoppen und Schrittanzahl der vorherigen Bewegung zwischenspeichern
				}
				if (steps != 0){				// Schrittanzahl nicht 0?
					sequence[index] = steps;	// Schrittanzahl bei aktuellem Index ins Sequence-Array speichern
					index++;					// Index inkrementieren
				}
			}
			if (index > 19) {					// Sequenz-Array voll?
				moveStop();						// Motor stoppen (sollte ohnehin schon stehen)
				PORTC = 0xF0;					// LED8:5 einschalten (voller Sequenzspeicher signalisieren)
			}
		}
		
		/**************************************************************************************/
		/* Taste 3 gedr�ckt? --> gespeicherte Sequenz abspielen								  */
		if (!(PINA & (1<<PA3))){
			/****Tasterentprellen****/
			_delay_ms(10);						// 10ms Delay zur Entprellung beim Dd�cken
			while(!(PINA & (1<<PA3))) {;}		// Warten solange Taste gedr�ckt
			_delay_ms(10);						// 10ms Delay zur Entprellung beim loslassen
			/****Tasterentprellen****/
			
			int32_t origin = 0;								// Variable f�r den Abstand zum Ursprung definieren
			for (int i = 0; i < 20; i++){					// Durch die Elemente in Sequence iterieren 
				origin = origin + sequence[i];				// Abstand zum Ursprung durch die Summe der Elemente in Sequence ermitteln
			}
			
			if (origin > 0){								// Abstand zum Urpsrung gr��er 0?
				moveNStepsBackward((int16_t)origin);		// Um Schrittanzahl zum Ursprung zur�ckfahren
			} else{											// Abstand zum Ursprung nicht gr��er 0?
				moveNStepsForward((int16_t)origin * -1);	// Um Betrag der Schrittanzahl zum Ursprung vorw�rts fahren
			}
			_delay_ms(500);									// 500ms warten um Ankunft am Ursprung zu signalisieren
			
			for(int i = 0; i < 20; i++){						// Durch die Elemente in Sequence iterieren
				if (sequence[i] != 0){							// Schrittanzahl im aktuellen Sequence-Element nicht 0?
					if (sequence[i] > 0){						// Schrittanzahl gr��er 0?
						moveNStepsForward(sequence[i]);			// Um Schrittanzahl des Elements vorw�rts fahren
						}else{									// Schrittanzahl nicht kleiner 0?
						moveNStepsBackward(sequence[i] * -1);	// Um Betrag der Schrittanzahl des Elements r�ckw�rts fahren 
					}
				}
			}
		}
	}
}

