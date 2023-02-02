/************************************************************/
/* Bibliotheksfunktionen zur Schrittmotorsteuerung			*/
/*		mit TMC2209 Schrittmotor-Controller					*/
/*															*/
/************************************************************/

#define HSCALER 99 // OCR0=99 mit 64 Vorteiler --> 1876Hz Interruptfrequenz (--> Signalfrequenz = 1876/2 = 938Hz)

#define OUTPORTDDR DDRB // Datenrichtungsregister des Clock und Dir Pin
#define OUTPORT PORTB	// Portregister des Clock und Dir Pin
#define CLOCKPIN PB3	// Clock Pin
#define DIRPIN PB2		// Direction Pin

// Motorbewegung stoppen												
//																					
// R�ckgabe: Schritte der letzten Aktion bis zur Ausf�hrung dieser Routine (R�ckw�rts < 0 < Vorw�rts)								    					
int16_t moveStop();


// Motorbewegung vorw�rts bis zu neuer Anweisung						
//																					 
// R�ckgabe: Schritte der letzten Aktion bis zur Ausf�hrung dieser Routine (R�ckw�rts < 0 < Vorw�rts)							    					  */
int16_t moveForward();

// Motorbewegung r�ckw�rts bis zu neuer Anweisung				
//
//R�ckgabe: Schritte der letzten Aktion bis zur Ausf�hrung dieser Routine (R�ckw�rts < 0 < Vorw�rts)
int16_t moveBackward();

// Motorbewegung vorw�rts f�r n Schritte (bzw. Mikroschritte je nach Treiber config) 
//																	
// Eingabeparameter: Auszuf�hrende Schrittanzahl								
void moveNStepsForward(uint16_t n);

// Motorbewegung r�ckw�rts f�r n Schritte (bzw. Mikroschritte je nach Treiber config) 
//																			
// Eingabeparameter: Auszuf�hrende Schrittanzahl								
void moveNStepsBackward(uint16_t n);