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
// Rückgabe: Schritte der letzten Aktion bis zur Ausführung dieser Routine (Rückwärts < 0 < Vorwärts)								    					
int16_t moveStop();


// Motorbewegung vorwärts bis zu neuer Anweisung						
//																					 
// Rückgabe: Schritte der letzten Aktion bis zur Ausführung dieser Routine (Rückwärts < 0 < Vorwärts)							    					  */
int16_t moveForward();

// Motorbewegung rückwärts bis zu neuer Anweisung				
//
//Rückgabe: Schritte der letzten Aktion bis zur Ausführung dieser Routine (Rückwärts < 0 < Vorwärts)
int16_t moveBackward();

// Motorbewegung vorwärts für n Schritte (bzw. Mikroschritte je nach Treiber config) 
//																	
// Eingabeparameter: Auszuführende Schrittanzahl								
void moveNStepsForward(uint16_t n);

// Motorbewegung rückwärts für n Schritte (bzw. Mikroschritte je nach Treiber config) 
//																			
// Eingabeparameter: Auszuführende Schrittanzahl								
void moveNStepsBackward(uint16_t n);