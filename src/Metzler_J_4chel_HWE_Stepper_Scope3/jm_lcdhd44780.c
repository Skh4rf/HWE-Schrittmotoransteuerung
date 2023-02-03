/*
 * lcdhd44780lib.c
 *
 * Created: 09.01.2023 14:36:05
 * Author : Skh4rf (I2C-Function refer to zkslibi2c)
 *
 * Issues: 1. Display reservieren solange Schreiboperationen stattfinden
 *				vorallem bei ISR-Anwendungen relevant
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

static uint8_t iicAddress = 0x27;

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

/****************************************************************************************/
/* TW-Interface Declarations															*/
/* 														                         		*/

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

// I2C-Defines
#define TW_CTRL_ENABLE ((1<<TWINT) | (1<<TWEN))
#define TW_CTRL_START ((1<<TWINT) | (1<<TWSTA) | (1<<TWEN))
#define TW_CTRL_STOP ((1<<TWINT) | (1<<TWSTO) | (1<<TWEN))
#define TW_CTRL_EXEC ((1<<TWINT) | (1<<TWEN))
#define TW_CTRL_EXEC_ACK ((1<<TWINT) | (1<<TWEN) | (1<<TWEA))
#define TW_CTRL_EXEC_RESTART ((1<<TWINT) | (1<<TWEN) | (1<<TWSTO))

#define TW_START 0x08
#define TW_SLA_ACK_W 0x18
#define TW_SLA_ACK_R 0x40
#define TW_DATA_ACK_W 0x28
#define TW_DATA_ACK_R 0x58

#define TW_DATA_READ 1
#define TW_DATA_WRITE 0
#define TW_SEND_DEVICE_ADDR 1
#define TW_SKIP_DEVICE_ADDR 0



#define TW_STAT_CLOSED 0
#define TW_STAT_READY 1
#define TW_STAT_BUSY 2

#define ERR_START TW_ERR_START
#define ERR_NACK TW_ERR_NACK

#define ERR_CNT_MAX 20

#define TW_FRAME_DELAY 1

#define TW_COM_DELAY_SHORT_US 10
#define TW_COM_DELAY_LONG_US 100

static uint8_t tw_Status = TW_STAT_CLOSED;

#define TW_DDR_SCL DDRC
#define TW_DDR_SDA DDRC
#define TW_PORT_SCL PORTC
#define TW_PORT_SDA PORTC
#define TW_BIT_SCL 0
#define TW_BIT_SDA 1

/* ATMEGA16 Clock Generation */
/* fscl=F_CPU/(16+2*TWBR*4^TWPS) */
/* Pullups shall be not lower than 3mA charge current */

#define TW_TWBR_N 7
#define TW_TWBS_N 2

/****************************************************************************************/
/* Routine zum Setzen der IIC-Adresse													*/
/*																						*/
/* Eingabeparameter: IIC-Salveaddress													*/
void iic_setAddress(uint8_t address){
	iicAddress = address;
}

/****************************************************************************************/
/* Init the I2C Module																	*/
/* Set Pin configuration																*/
/* reset I2C Module,																	*/
unsigned char iic_Init (void)
{
	/* Setup I2C signal lines as inputs*/
	TW_DDR_SCL&=~(1<<TW_BIT_SCL);
	TW_DDR_SDA&=~(1<<TW_BIT_SDA);
	
	/* activate internal pullups */
	TW_PORT_SCL|=1<<TW_BIT_SCL;
	TW_PORT_SDA|=1<<TW_BIT_SDA;
	
	/* Setup Clock divider */
	TWBR = TW_TWBR_N;
	TWSR = TW_TWBS_N;
	
	/* reset i2c control reg */
	TWCR=0x00;
	
	tw_Status=TW_STAT_READY;
	
	return TW_ERR_OK;
	
}

/**************************************************************************************/
/* Write one Byte to Bus														     	*/
/* Result: OK if salve has acknowledged												  */
uint8_t _loc_WriteByte(uint8_t Data)
{
	/* Define variables */
	uint8_t Err=TW_ERR_OK;
	uint8_t tmoCnt=0;
	uint8_t twStatus=0;


	/* Data to send */
	TWDR=Data;

	/* Execute  */
	TWCR = TW_CTRL_EXEC;
	
	/* wait until data byte has been completed */
	tmoCnt=0;
	while (!(TWCR & (1<<TWINT)) && (tmoCnt<ERR_CNT_MAX))
	{
		tmoCnt++;
		_delay_us(TW_COM_DELAY_SHORT_US);
	}
	
	// Check if loop has timed out and set error status
	if (tmoCnt>=ERR_CNT_MAX)
	{
		Err=TW_ERR_TMO;
	}
	else
	{
		/* check status after completion */
		twStatus=(TWSR & 0xF8);
		switch(twStatus)
		{
			// Slave has confirmed
			case 0x28:
			Err=TW_ERR_OK;
			break;
			
			// Slave has NOT confirmed
			case 0x30:
			Err=TW_ERR_NACK;
			break;
			
			// unrecognised status
			default:
			Err=TW_ERR_ILLEGAL;
			break;
		}
	}

	// Return the error status of the operation
	return Err;
}

/**************************************************************************************/
/* Write SLA+W and wait for completion												  */
/* Result: OK if slave has acknowledged												  */
uint8_t _loc_SendSlaW (uint8_t DevAddr)
{
	/* Define variables */
	uint8_t Err=TW_ERR_OK;
	uint8_t tmoCnt=0;
	uint8_t twStatus=0;


	/* Set Device Adresse and R/W = 0 */
	TWDR=(DevAddr<<1);

	/* Send Device Adresse   */
	TWCR = TW_CTRL_EXEC;
	
	/* wait until start condition has been completed */
	tmoCnt=0;
	while (!(TWCR & (1<<TWINT)) && (tmoCnt<ERR_CNT_MAX))
	{
		tmoCnt++;
		_delay_us(TW_COM_DELAY_SHORT_US);
	}
	
	// Check if loop has timed out and set error status
	if (tmoCnt>=ERR_CNT_MAX)
	{
		Err=TW_ERR_TMO;
	}
	else
	{
		/* check if start condition has completed */
		twStatus=(TWSR & 0xF8);
		switch(twStatus)
		{
			// Slave has confirmed
			case 0x18:
			Err=TW_ERR_OK;
			break;
			
			// Slave has NOT confirmed
			case 0x20:
			Err=TW_ERR_NOSLAVE;
			break;
			
			// unrecognised status
			default:
			Err=TW_ERR_ILLEGAL;
			break;
		}
	}

	// Return the error status of the operation
	return Err;
}

/**************************************************************************************/
/* Close the current communication	unconditionally									  */
uint8_t _loc_BusStop (void)
{
	
	/* Send StopCondition */
	TWCR = TW_CTRL_STOP;
	_delay_us(20);
	TWCR=0x00;
	
	tw_Status=TW_STAT_READY;
	
	return TW_ERR_OK;
	
}

/**************************************************************************************/
/* Open the TW channel for later bus access										      */
/*  initiate start condition and wait for completion								  */
unsigned char _loc_BusStart (void)
{
	/* Define variables */
	uint8_t Err=TW_ERR_OK;
	uint8_t tmoCnt=0;
	uint8_t twStatus=0;

	/* Send Start Condition */
	TWCR = TW_CTRL_START;
	
	/* wait until start condition has been completed */
	tmoCnt=0;
	while (!(TWCR & (1<<TWINT)) && (tmoCnt<ERR_CNT_MAX))
	{
		tmoCnt++;
		_delay_us(TW_COM_DELAY_SHORT_US);
	}
	
	// Check if loop has timed out and set error status
	if (tmoCnt>=ERR_CNT_MAX)
	{
		Err=TW_ERR_TMO;
	}
	else
	{
		/* check if start condition has completed */
		twStatus=(TWSR & 0xF8);
		switch(twStatus)
		{
			// Start Condition successful
			case 0x08:
			tw_Status=TW_STAT_BUSY;
			Err=TW_ERR_OK;
			break;
			
			// unrecognised status
			default:
			Err=TW_ERR_ILLEGAL;
			break;
		}
	}

	// Return the error status of the operation
	return Err;
}


/**************************************************************************************/
/* Schreiben einer beliebigen Anzahl Bytes an einen angeschlossenen Slave             */
/* Parameter: Geräteadresse, Pointer auf den Datenbuffer, Anzahl zu übertragenden     */
/*				Bytes							                                      */
/*																					  */
/* Wird vom Slave kein ACK empfangen dann bricht die Funktion ab und meldet einen     */
/*	Fehler zurück																      */
/*																					  */
/* Rückgabe: ERR-Code laut #defines													  */
uint8_t iic_WriteNBytes (uint8_t DeviceAddr, uint8_t  * Data, uint8_t NBytes)
{
	uint8_t twErr=TW_ERR_OK;
	uint8_t Cnt=0;
	
	if((NBytes<1)||(NBytes>99))
	{
		return TW_ERR_NBYTES;
	}
	else
	{
		// Send Start Condition
		if(_loc_BusStart()!=TW_ERR_OK)
		{
			_loc_BusStop();
			return TW_ERR_START;
		}
		
		// Send SLave Adresse for Write access
		switch(_loc_SendSlaW(DeviceAddr))
		{
			case TW_ERR_OK:
			// Slave has confirmed, send data out
			//Transmit the data as long a as slave sends acknowledge
			Cnt=0;
			while ((Cnt<NBytes)&&(twErr==TW_ERR_OK))
			{
				twErr=_loc_WriteByte(Data[Cnt]);
				Cnt++;
			}
			
			// Release Bus unconditionally and return with most recent Err Code
			_loc_BusStop();
			return twErr;
			break;
			
			case TW_ERR_NOSLAVE:
			_loc_BusStop();
			return TW_ERR_NOSLAVE;
			
			default:
			_loc_BusStop();
			return TW_ERR_ILLEGAL;
			break;
		}
	}
}

/**************************************************************************************/
/* 4-Bit LCD-Befehls-Senderoutine													  */    
/*																					  */
/* Parameter: 4-Bit Befehl 						                                      */
void iic_cmd4(uint8_t n){ 
	uint8_t data = (n<<4) | 8; // 
	iic_WriteNBytes(iicAddress, &data, 1);
	data = (n<<4) | 12;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = (n<<4) | 8;
	iic_WriteNBytes(iicAddress, &data, 1);
}

/**************************************************************************************/
/* 8-Bit LCD-Befehls-Senderoutine für 4-Bit Modus mit 4-Bit Eingabe					  */
/*																					  */
/* Parameter: obere 4-Bit des Befehls, untere 4-Bit des Befehls	                      */
void iic_cmd8(uint8_t hn, uint8_t ln){
	uint8_t data = (hn<<4) | 8;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = (hn<<4) | 12;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = (hn<<4) | 8;
	iic_WriteNBytes(iicAddress, &data, 1);
	
	data = (ln<<4) | 8;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = (ln<<4) | 12;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = (ln<<4) | 8;
	iic_WriteNBytes(iicAddress, &data, 1);
	
	_delay_ms(1);
}

/**************************************************************************************/
/* 8-Bit LCD-Befehls-Senderoutine für 4-Bit Modus mit 8-Bit Eingabe					  */
/*																					  */
/* Parameter: 8-Bit Befehl										                      */
void iic_cmd8Hex(uint8_t n){
	uint8_t hn = (n>>4) & 0x0F;
	uint8_t ln = n & 0x0F;
	iic_cmd8(hn, ln);
}

/**************************************************************************************/
/* LCD-Initialisierung für 4-Bit Modus												  */
void lcd_init(){
	// Initialisierungssequenz laut Datenblatt:
	iic_cmd4(3); _delay_ms(500);
	iic_cmd4(3); _delay_ms(20);
	iic_cmd4(3); _delay_ms(10);
	iic_cmd4(2); _delay_ms(10);
	
	iic_cmd8(2,8); // Function Set // 0100 1000
	iic_cmd8(0,8); // Display Off
	iic_cmd8(0,1); // Clear Display
	iic_cmd8(0,6); // Entry Mode Set
	iic_cmd8(0,14); // Display On
	iic_cmd8(0,2); // Return Home
}

/**************************************************************************************/
/* LCD-Daten-Senderoutine															  */
/*																					  */
/* Parameter: obere 4-Bit der Daten, unter 4-Bit der Daten							  */
void iic_data(uint8_t hn, uint8_t ln){
	uint8_t data = hn | 9;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = hn | 13;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = hn | 9;
	iic_WriteNBytes(iicAddress, &data, 1);
	
	data = ln | 9;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = ln | 13;
	iic_WriteNBytes(iicAddress, &data, 1);
	data = ln | 9;
	iic_WriteNBytes(iicAddress, &data, 1);
}

/**************************************************************************************/
/* LCD-Letter-Printroutine für aktuelle Cursorposition								  */
/*																					  */
/* Parameter: Buchstabe als String-Type												  */
void lcd_printLetter(char letter){
	int asciiLetter = (int)letter;
	iic_data((asciiLetter & 0xF0), (asciiLetter & 0x0F)<<4); 
	//_delay_ms(1);
}

/**************************************************************************************/
/* LCD-String-Printroutine für aktuelle Cursorposition								  */
/*																					  */
/* Parameter: Zeichenkette															  */
/* Hinweis: Line-Switching funktioniert nur bei Cursor-Positon 0					  */
void lcd_printString(char* str){
	for (int i = 0; i < strlen(str); i++) {
		if(i == 16){
			iic_cmd8Hex(LCD_LINE2ADDR | LCD_SETDDRAMADDR);
		}
		lcd_printLetter(str[i]);
	}
}

/**************************************************************************************/
/* LCD-Set-Cursor-Routine															  */
/*																					  */
/* Parameter: Cursor-Adresse														  */
void lcd_setCursor(char address){
	iic_cmd8Hex(address | LCD_SETDDRAMADDR);
}

/**************************************************************************************/
/* LCD-Print-Function für String ab Cursorposition 0								  */
/*																					  */
/* Parameter: Zeichenkette	    													  */
void  lcd_overwriteWithString(char* str){
	lcd_setCursor(LCD_LINE1ADDR);
	lcd_printString(str);
}


/**************************************************************************************/
/* Example-Code:																	  */
/*																					  */
/* int main(void)																	  */
/* {																			      */
/*		iic_Init(); // IIC-Schnittstelle initialisieren								  */
/*		_delay_ms(500); // 500ms warten												  */
/*		lcd_init(); // LCD-Initialisieren										      */
/*		iic_cmd8(0, 2); // Return Home											      */
/*		lcd_printString("Hallo Welt!"); // "Hallo Welt!" auf LCD schreiben		      */
/*		_delay_ms(1000);														      */
/*		lcd_overwriteWithString("Tschuess Welt!");									  */
/*		while (1)																	  */
/*		{																			  */
/*			;																		  */
/*		}																			  */
/* }																				  */






