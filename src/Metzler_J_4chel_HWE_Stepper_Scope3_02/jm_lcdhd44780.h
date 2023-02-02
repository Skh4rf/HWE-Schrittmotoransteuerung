/************************************************************/
/* Bibliotheksfunktionen für HD44780 LCD-Controller			*/
/*		mit PCF8574-IIC-I/O-Expander						*/															
/*															*/
/* IIC Init und WriteNByte stammen von zkslibi2c			*/
/*															*/
/************************************************************/

// LCD-Befehle
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80
#define LCD_LINE1ADDR 0x00
#define LCD_LINE2ADDR 0x40


// Andere Definitionen
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// Status data
#define TW_ERR_OK		0x00
#define TW_ERR_START	0x01
#define TW_ERR_NACK		0x02
#define TW_ERR_TMO		0x03
#define TW_ERR_STATUS	0x04
#define TW_ERR_ACK		0x05
#define TW_ERR_NBYTES   0x06
#define TW_ERR_NOSLAVE  0x07
#define TW_ERR_ILLEGAL  0x08;



//Init the I2C Module														
//Set Pin configuration													
//reset I2C Module,																
uint8_t iic_Init (void);



// Schreiben einer beliebigen Anzahl Bytes an einen angeschlossenen Slave          
// Parameter: Geräteadresse, Pointer auf den Datenbuffer, Anzahl zu übertragenden    
//				Bytes							                                  
//																					 
// Wird vom Slave kein ACK empfangen dann bricht die Funktion ab und meldet einen    
//	Fehler zurück																      
//																					
// Rückgabe: ERR-Code laut #defines													 
uint8_t iic_WriteNBytes (uint8_t DeviceAddr, uint8_t  * Data, uint8_t NBytes);

// Routine zum Setzen der IIC-Adresse											
//																					
// Eingabeparameter: IIC-Salveaddress (Default=0x27)							
void iic_setAddress(uint8_t address);

// 4-Bit LCD-Befehls-Senderoutine													
//																			
// Parameter: 4-Bit Befehl 						                             
void iic_cmd4(uint8_t n);


// 8-Bit LCD-Befehls-Senderoutine für 4-Bit Modus mit 4-Bit Eingabe					  
//																				
// Parameter: obere 4-Bit des Befehls, untere 4-Bit des Befehls	                  
void iic_cmd8(uint8_t hn, uint8_t ln);

// 8-Bit LCD-Befehls-Senderoutine für 4-Bit Modus mit 8-Bit Eingabe					
//																				
// Parameter: 8-Bit Befehl										                  
void iic_cmd8Hex(uint8_t n);

// LCD-Initialisierung für 4-Bit Modus			
void lcd_init();

// LCD-Daten-Senderoutine												
//																				
// Parameter: obere 4-Bit der Daten, unter 4-Bit der Daten					
void iic_data(uint8_t hn, uint8_t ln);

// LCD-Letter-Printroutine für aktuelle Cursorposition							
//																				
// Parameter: Buchstabe als String-Type											
void lcd_printLetter(char letter);

// LCD-String-Printroutine für aktuelle Cursorposition							
//																				
// Parameter: Zeichenkette														
// Hinweis: Line-Switching funktioniert nur bei Cursor-Positon 0				
void lcd_printString(char* str);

// LCD-Set-Cursor-Routine														
//																				
// Parameter: Cursor-Adresse													
void lcd_setCursor(char address);

// LCD-Print-Function für String ab Cursorposition 0								 
//																				
// Parameter: Zeichenkette	    												
void  lcd_overwriteWithString(char* str); 