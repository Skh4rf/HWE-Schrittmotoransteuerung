/*
 * Metzler_J_4chel_HWE_Stepper_Scope3.c
 *
 * Created: 17.01.2023 10:44:18
 * Author : Jakob
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "jm_lcdhd44780.h"

#define OUTPORTDDR DDRB // Datenrichtungsregister des Clock und Dir Pin
#define OUTPORT PORTB	// Portregister des Clock und Dir Pin
#define CLOCKPIN PB3	// Clock Pin
#define DIRPIN PB2		// Dir Pin

uint16_t steps;				// Globale Schrittzähl-Variable definieren
signed char inkrementor;	// Globaler Schrittzähl-Inkrementor definieren
uint16_t counter;			// Globaler Softwarecounter zur Signalerzeugung definieren
uint16_t period;			// Globale Periodendauer-Variable definieren
double frequency;			// Globale Frequenz-Variable definieren
char reserved;				// Globale Variable zur Display-Update verhinderung definieren

/*********************************************************************************************************************************************/
/* Interrupt-Service-Routine der Signalerzeugung (f=10kHz -> T=0.1ms)																		 */
/*																					                                                         */
/* period-Variable kann zur Frequenzänderung verwendet werden (!Achtung! zur Frequenzerzeugung muss die doppelte Frequenz gewählt werden)    */
ISR (TIMER0_COMP_vect){
	counter++;											// Softwarecounter inkrementieren
	if (counter >= period){								// halbe Periodendauer der aktuellen Frequenz abwarten
		OUTPORT ^= (1<<CLOCKPIN); 						// Clock-Pin toggeln
		counter = 0;									// Softwarecounter zurücksetzen
		if (OUTPORT & (1<<CLOCKPIN)){					// Clock-Pin logisch 1? (jeder zweite ISR Aufruf -> 1 Step)
			steps = steps + inkrementor;				// Schrittzähl-Variable inkrementieren			
			if (steps == 0){							// Schrittzähl-Variable 0?
				inkrementor = 1;						// Schrittzähl-Inkrementor auf 1 setzen (-> Zählrichtungsumkehr)
				steps = 1;								// Schrittzähl-Variable auf 1 setzen (Inkrement für diesen Zyklus manuell durchführen)
			}
			if (steps % 100 == 0 && reserved != 1){		// Schrittzahl gerade durch 100 teilbar und Display Zugriff nicht reserviert?
				char str[14];							// String-Buffer definieren
				sprintf(str, "Step: %d   ", steps);		// Schrittzählvariable als String auf den String-Buffer speichern
				lcd_setCursor(LCD_LINE1ADDR);			// LCD-Cursor auf Anfangsadresse setzen
				lcd_printString(str);					// String-Buffer auf Display ausgeben
			}
			steps = steps % 1600;						// Falls Schrittzählvariable 1600 -> zurücksetzen
		}
	}
}

/*****************************************************************************************************/
/* Initialisiert Timer0 zur Clockausgabe an Pin PB3 (ISR-Aufruffrequenz = 1kHz -> T= 0.1ms)			 */
/* --> Motorbewegung starten																		 */
void init_Timer0(){
	OCR0 = 149;									// OCR0 Compare-Wert setzen																
	TCCR0 = (1<<WGM01) | (1<<CS01);				// Timer konfiguration -> Mode: CTC, HW-Teiler: 8
	TIMSK = (1<<OCIE0);							// OCIE0-Match Interrupt spezifisch freigeben
	sei();										// Interrupts global freigeben
}

/*****************************************************************************************************/
/* Timer0 stoppen und Clock-Pin auf logisch 0 setzen												 */
/* --> Motorbewegung stoppen																		 */
void stop_Timer0(){
	TCCR0=0x00;									// Timer-Controll Register löschen
	OUTPORT &= ~(1<<CLOCKPIN);					// Clock-Pin auf logisch 0 bringen	
}

/***********************************************************************************************************************************/
/* Routine zur Verringerung der Frequenz (falls f < 0 --> Richtungsumkehr)							                               */
void decreaseFrequency(){
	char frequencyArray[12];												// String-Buffer definieren
	if (frequency > 5){														// Frequenz größer 5?
		frequency -= 5;														// Frequenz dekrementieren
		period = (uint16_t)1/(frequency*2)/0.0001;							// Neue Periodendauer aus Frequenz berechnen	
		sprintf(frequencyArray, "Freq: %dHz    ", (int16_t)frequency);		// Frequenz als String in String-Buffer speichern
		lcd_setCursor(LCD_LINE2ADDR);										// LCD-Cursor auf Startadresse der 2. Zeile setzen
		lcd_printString(frequencyArray);									// String-Buffer auf Display ausgeben
	}else{									// Frequenz kleiner 5?
		inkrementor *= -1;					// Inkrementor invertieren (Steps Anzeige zählt nun bis zum erreichen von 0 rückwärts)
		stop_Timer0();						// Motor stoppen
		frequency = 0;
		OUTPORT ^= (1<<DIRPIN);				// Richtungspin invertieren
		lcd_setCursor(LCD_LINE2ADDR);		// LCD-Cursor auf Startadresse der 2. Zeile setzen
		lcd_printString("Freq: 0Hz    ");	// Frequenz auf Display ausgeben
	}
}

/***********************************************************************************************************************************/
/* Routine zum erhöhen der Frequenz																	                               */
void increaseFrequency(){		
	char frequencyArray[12];											// String-Buffer definieren
	if (frequency < 100){												// Frequenz kleiner 100?
		if (frequency < 5){												// Frequenz kleiner 5?
			init_Timer0();												// Motorbewegung starten (Timer initialisieren)
		}
		frequency += 5;													// Frequenz inkrementieren									
		period = (uint16_t)1/(frequency*2)/0.0001;						// Neue Periodendauer aus Frequenz berechnen
		sprintf(frequencyArray, "Freq: %dHz   ", (int16_t)frequency);	// Frequenz als String in String-Buffer speichern
		lcd_setCursor(LCD_LINE2ADDR);									// LCD-Cursor auf Startadresse der 2. Zeile setzen
		lcd_printString(frequencyArray);								// String-Buffer auf Display ausgeben
	}
}

int main(void)
{
	iic_Init();											// IIC-Schnittstelle initialisieren
	lcd_init();											// LCD-Schnittstelle initialisieren
	iic_cmd8Hex(LCD_CLEARDISPLAY);						// LCD-Inhalt löschen
	iic_cmd8Hex(LCD_DISPLAYCONTROL | LCD_DISPLAYON);	// Cursoranzeige ausschalten (LCD_DISPLAYON gesetzt da das Display trotzdem an bleiben sollte)
	
	OUTPORTDDR = (1<<CLOCKPIN) | (1<<DIRPIN);	// Clock-Pin und Direction-Pin als Ausgang definieren
	OUTPORT = (1<<DIRPIN);						// Direction-Pin auf logisch 1 setzen
	
	DDRA = 0x00;	// Taster-Port als Eingang definieren
	PORTA = 0x0F;	// PullUps für Taster aktivieren
	
	reserved = 0;		// Display-Reservierungs-Variable auf 0 setzen (nicht reserviert)
	inkrementor = 1;	// Inkrementor auf 1 setzen (Start)
	steps = 0;			// Schrittzählvariable auf 0 setzen (Start)
	counter = 0;		// Softwarecounter auf 0 setzen (Start)
	frequency = 100;	// Frequenz auf 100 setzen (Start)
	
	period = (uint16_t)1/(frequency*2)/0.0001;	// Periodendauer aus Frequenz berechnen
	lcd_setCursor(LCD_LINE2ADDR);				// LCD-Cursor auf Startadresse der 2. Zeile setzen
	lcd_printString("Freq: 100Hz");				// Frequenz auf Display ausgeben

	init_Timer0();	// Motor bewegen (Timer0 initialisieren)

	while (1)
	{
		/*********************************************************************************************/
		/* Taste 0 gedrückt? --> Frequenz erhöhen (DIR=0) bzw. verringern (DIR=1)					 */
		if(!(PINA & (1<<PA0))){
			reserved = 1;					// Display reservieren
			if (OUTPORT & (1<<DIRPIN)){		// Direction-Pin logisch 1?
				decreaseFrequency();		// Frequenz verringern
			}else{							// Direction-Pin logisch 0?
				increaseFrequency();		// Frequenz erhöhen
			}
			_delay_ms(100);					// 100ms warten (0%-100% linear sweep dauert ca. 2s)
			reserved = 0;					// Display freigeben
		}
		
		/*********************************************************************************************/
		/* Taste 1 gedrückt? --> Frequenz erhöhen (DIR=1) bzw. verringern (DIR=0)			         */
		if(!(PINA & (1<<PA1))){
			reserved = 1;					// Display reservieren
			if (OUTPORT & (1<<DIRPIN)){		// Direction-Pin logisch 1?
				increaseFrequency();		// Frequenz erhöhen
			}else{							// Direction-Pin logisch 0?
				decreaseFrequency();		// Frequenz verringern
			}
			_delay_ms(100);					// 100ms warten (0%-100% linear sweep dauert ca. 2s)
			reserved = 0;					// Display freigeben
		}
	}
}


