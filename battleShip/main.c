//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "LCD.h"

#define HXT_STATUS 1<<0
#define PLL_STATUS 1<<2
#define LIRC_STATUS 1<<3  // 10 kHz
//Macro define when there are keys shoted in each column
#define C3_pressed (!(PA->PIN & (1<<0)))
#define C2_pressed (!(PA->PIN & (1<<1)))
#define C1_pressed (!(PA->PIN & (1<<2)))
#define TMR0_COUNT 100 - 1

void System_Config(void);
void Clock_Config(void);
void GPIO_Config(void);
void SPI3_Config(void);

void UART0_Config(void);
char UART0_GetChar(void);
void UART02_IRQHandler(void);
void TMR0_IRQHandler(void);



void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);

//Gloabl Array to display on 7segment for NUC140 MCU
int displayNum[] = {
  0b10000010,  //Number 0          // ---a----
  0b11101110,  //Number 1          // |      |
  0b00000111,  //Number 2          // f      b
  0b01000110,  //Number 3          // |      |
  0b01101010,  //Number 4          // ---g----
  0b01010010,  //Number 5          // |      |
  0b00010010,  //Number 6          // e      c
  0b11100110,  //Number 7          // |      |
  0b00000010,  //Number 8          // ---d----
  0b01000010,  //Number 9
  0b11111111   //Blank LED 
};

volatile int mapY = 0;
volatile int mapX = 0;

volatile int cleanLCD = 0;
volatile int state = 0;

volatile int digit = 0;
volatile bool isY = false;
volatile int x = 1;
volatile int y = 1;
volatile int shot = 0;
volatile int hit = 0;

volatile int isBuzzerOn = 0;

volatile int map[8][8];
volatile int view[8][8] = {
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0}	
};

void printMap (void) {
	for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
			if (view[row][col] == 0) {
					printC_5x7(row*16, col*8, '-');
			} else if (view[row][col] == 1) {
					printC_5x7(row*16, col*8, 'x');
			}
		}
	}
}

// reset map to the original setting
void reset() {
	for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
			view[row][col] = 0;
		}
	}
	isY = false;
	shot = 0;
	hit = 0;
}

// keypad pressed button value setting
void setInputValue(int value) {
  if (!isY) {
    if (value == 9) {
			// if K9 is pressed, set to Y-coordinate setting
      isY = true;
    } else {
			// Assign value to X-coordinate
      x = value;
    }
  } else {
    if (value == 9) {
			// if K9 is pressed, set to X-coordinate setting
      isY = false;
    } else {
			// Assign value to Y-coordinate
      y = value;
    }
  }
}

static void search_col1(void) {
    // Drive ROW1 output pin as LOW. Other ROW pins as HIGH
    PA->DOUT &= ~(1<<3);
    PA->DOUT |= (1<<4);
    PA->DOUT |= (1<<5);
    if (C1_pressed) {
        setInputValue(1);
        return;
    } else {
    // Drive ROW2 output pin as LOW. Other ROW pins as HIGH
        PA->DOUT |= (1<<3);
        PA->DOUT &= ~(1<<4);
        PA->DOUT |= (1<<5);
    if (C1_pressed) {
        // If column1 is LOW, detect key press as K4 (KEY 4)
        setInputValue(4);
        return;
    } else{   
    // Drive ROW3 output pin as LOW. Other ROW pins as HIGH
        PA->DOUT |= (1<<3);
        PA->DOUT |= (1<<4);
        PA->DOUT &= ~(1<<5);
				if (C1_pressed) {
					// If column1 is LOW, detect key press as K7 (KEY 7)
					setInputValue(7);
					return;
				} else
					return;
			}
    }
}

static void search_col2(void) {
    // Drive ROW1 output pin as LOW. Other ROW pins as HIGH
    PA->DOUT &= ~(1<<3);
    PA->DOUT |= (1<<4);
    PA->DOUT |= (1<<5);
    if (C2_pressed) {
			// If column2 is LOW, detect key press as K2 (KEY 2)
			setInputValue(2);
			return;
    } else {
    // Drive ROW2 output pin as LOW. Other ROW pins as HIGH
        PA->DOUT |= (1<<3);
        PA->DOUT &= ~(1<<4);
        PA->DOUT |= (1<<5);
    if (C2_pressed) {
			// If column2 is LOW, detect key press as K5 (KEY 5)
			setInputValue(5);
			return;
    } else {
    // Drive ROW3 output pin as LOW. Other ROW pins as HIGH
			PA->DOUT |= (1<<3);
			PA->DOUT |= (1<<4);
			PA->DOUT &= ~(1<<5);
    if (C2_pressed) {
			// If column3 is LOW, detect key press as K8 (KEY 8)
			setInputValue(8);
			return;
    } else
			return;
    }
	}
}

static void search_col3(void) {
    // Drive ROW1 output pin as LOW. Other ROW pins as HIGH
		PA->DOUT &= ~(1<<3);
		PA->DOUT |= (1<<4);
		PA->DOUT |= (1<<5);
 
    if (C3_pressed) {
			// If column3 is LOW, detect key press as K3 (KEY 3)
			setInputValue(3);
			return;
    } else {
    // Drive ROW2 output pin as LOW. Other ROW pins as HIGH
			PA->DOUT |= (1<<3);
			PA->DOUT &= ~(1<<4);
			PA->DOUT |= (1<<5);
    if (C3_pressed) {
			// If column3 is LOW, detect key press as K6 (KEY 6)
			setInputValue(6);
			return;
    } else {
    // Drive ROW3 output pin as LOW. Other ROW pins as HIGH
			PA->DOUT |= (1<<3);
			PA->DOUT |= (1<<4);
			PA->DOUT &= ~(1<<5);
    if (C3_pressed) {
			// If column3 is LOW, detect key press as K9 (KEY 9)
			setInputValue(9);
			return;
    } else
			return;
    }
	}
}

void keyPad_pressed(void) {
  //Turn all Rows to LOW 
  PA->DOUT &= ~(1<<3);
  PA->DOUT &= ~(1<<4);
  PA->DOUT &= ~(1<<5);
  // Check for key shot in key matrix
	if(C1_pressed) {
		search_col1();
	} else if(C2_pressed) {
		search_col2();
	} else if(C3_pressed) {
		search_col3();
	}
}

void Keypad_Setup(void) {
  PA->PMD &= (~(0b11<< 6));
  PA->PMD |= (0b01 << 6);    
  PA->PMD &= (~(0b11<< 8));
  PA->PMD |= (0b01 << 8);  		
  PA->PMD &= (~(0b11<< 10));
  PA->PMD |= (0b01 << 10);
}

void LED_Setup(void) {
  //Configure GPIO for 7segment, Set mode for PC4 to PC7
  PC->PMD &= (~(0xFF<< 8));		    //Clear PMD[15:8] 
  PC->PMD |= (0b01010101 << 8);   //Set output push-pull for PC4 to PC7
}

// Display a digit function
void digitNumber(int digit, int num){
  // Close all 4 digits 
  PC->DOUT &= ~(1<<4);		//SC1
  PC->DOUT &= ~(1<<5);		//SC2
  PC->DOUT &= ~(1<<6);		//SC3
  PC->DOUT &= ~(1<<7);    //SC4

  //Logic 1 to turn on the requested digit
  if (digit == 3) {PC->DOUT |= (1<<4);}
  else if (digit == 2) {PC->DOUT |= (1<<5);}
  else if (digit == 1) {PC->DOUT |= (1<<6);}
  else if (digit == 0) {PC->DOUT |= (1<<7);}
  
  PE->DOUT = displayNum[num];
  CLK_SysTickDelay(200); // Software debouncing
  PE->DOUT = displayNum[10];
}

// Set value to 7-segment digits
void displayDigit() {
  if (digit == 0 && !isY) {digitNumber(digit, x);}
  else if (digit == 1 && isY) {digitNumber(digit, y);}
  else if (digit == 2) {digitNumber(digit, shot/10);}
  else if (digit == 3) {digitNumber(digit, shot%10);}
}

int main(void) {

	//System initialization
	SYS_UnlockReg(); // Unlock protected registers

	System_Config();
	Clock_Config();

	GPIO_Config();
	Keypad_Setup();
	UART0_Config();

	SYS_LockReg();  // Lock protected registers
	
	//SPI3 initialization
	SPI3_Config();

	//LCD initialization
	LCD_start();
	LCD_clear();

	while (1) {
		if (cleanLCD) {
				LCD_clear();
				cleanLCD = 0;
		}
		
		switch (state) {
		case 0: // Welcome State
		printS_5x7(10, 10, "Welcome to Battleship");
		printS_5x7(25, 35, "Please load map");
		printS_5x7(25, 60, "Team 8");

			break;
		case 1: // State Uploading Map Finish
		printS_5x7(35, 10, "Battleship");
		printS_5x7(5, 30, "Map Loaded Successfully");
		printS_5x7(5, 40, "Push button SW_INT1");
		printS_5x7(5, 50, "to start the game!");

			break;
		case 2: // Game State
			// Reset buzzer
			isBuzzerOn = 0;
		
			// Start displaying 7seg
			TIMER0->TCSR |= (0x01 << 30);
		
			printMap();
			break;
		case 3: // Game Over State
			// Turn off Timer0
			TIMER0->TCSR &= ~(0x01 << 30);
			TIMER0->TCSR |= (1 << 26);
		
			PC->DOUT ^= (1<<15);		
			
			printS_5x7(35, 10, "Game over");
					
			printS_5x7(5, 35, "Push button SW_INT1");
			printS_5x7(5, 45, "to start the game!");

			//buzzer
			if (!isBuzzerOn) {
				for (int i = 0; i < (5 * 2); i++) {
						PB->DOUT ^= (1 << 11);
						CLK_SysTickDelay(200000);
				}
				isBuzzerOn = 1;
			}
			break;
		default:
			break;
		}
	}
}


//-----------------------------------------------------------------------------------
// Functions definition
//-----------------------------------------------------------------------------------
void System_Config(void) {
	// Enable 12MHz HXT, wait still stable
  CLK->PWRCON |= (1 << 0);
  while(!(CLK->CLKSTATUS & HXT_STATUS));

	CLK->PWRCON |= 1<<3;
  while(!(CLK->CLKSTATUS & LIRC_STATUS)); // Wait until 10 kHz clock is stable

  // CPU Clock Config Start---------------
  // Only use 12 MHz HXT
  // Select CPU clock
  CLK->CLKSEL0 &= ~(0b111 << 0);  // 12 MHz HXT
  CLK->PWRCON &= ~(1<<7);         // Normal mode
  // Clock frequency divider
  CLK->CLKDIV &= ~(0xF<<0);
  // CPU Clock Config End----------------

	//Clock source selection
	CLK->CLKSEL0 &= (~(0x07 << 0));
	CLK->CLKSEL0 |= (0x02 << 0);
	//Clock frequency division
	CLK->CLKDIV &= (~0x0F << 0);

	//Enable clock of SPI3
	CLK->APBCLK |= 1 << 15;
	
	//UART0 Clock selection and configuration
	CLK->CLKSEL1 |= (0x03 << 24);   // UART0 clock source is 22.1184 MHz
	CLK->CLKDIV &= ~(0x0F << 8); 	// Clock Divider is 1
	CLK->APBCLK |= (0x01 << 16); 	// Enable UART0 clock
}

void Clock_Config(void) {
	// Timer0 Config with Interrupt--------
	CLK->CLKSEL1 &= ~(0x07 << 8);
	CLK->CLKSEL1 |= (0x02 << 8);        // Select 12 MHz HLCK (DataSheet p128)
	CLK->APBCLK |= (0x01 << 2);         // Enable TM0 

	// Pre-scale (value + 1)
	TIMER0->TCSR &= ~(0xFF << 0);
	TIMER0->TCSR |= 11 << 0; 			// 12 Mhz above / (11+1) = 1 MHz

	// Reset and config operating mode
	TIMER0->TCSR |= (0x01 << 26);       // Reset Timer 0
	
	// Periodic mode
	TIMER0->TCSR &= ~(0x03 << 27);		// Reset
	TIMER0->TCSR |= (0x01 << 27);       // 0x01 Periodic mode 

	TIMER0->TCSR &= ~(0x01 << 24);      // Disable CTB

	// Enable TE bit (bit 29) of TCSR
	// The bit will enable the timer interrupt flag TIF (for Polling mode and Interrupt)
	TIMER0->TCSR |= (1 << 29);

	// TDR to be updated continuously while timer counter is counting
	TIMER0->TCSR |= (0x01 << 16);
	TIMER0->TCMPR = TMR0_COUNT;

	// TMR0 Interrupt
	NVIC->ISER[0] |= (1 << 8); 	// TMR0 Interrupt System
	NVIC->IP[2] &= ~(3 << 6); 	// Reset value - Highest priority
	NVIC->IP[2] |= (0 << 6); 	// Lowest priority
	//-----End of Timer0 Config---------
}

void GPIO_Config(void) {
    // GPIO Configuration starts-------------
	// LED5-6 - GPC12-13
	PC->PMD &= (~(0x03 << 24));
	PC->PMD |= (0x01 << 24); // push-pull

	PC->PMD &= (~(0x03 << 26));
	PC->PMD |= (0x01 << 26); // push-pull

	// Buzzer - GPB11
	PB->PMD &= (~(0x03 << 22));
	PB->PMD |= (0x01 << 22); // push-pull

	// GPIO Interrupt configuration. B.15 is the interrupt source
	// Set Interrupt Debounce Cycle Control
	GPIO->DBNCECON |= (1 << 4); 		// Debounce Clock Source 10 kHz LIRC
	GPIO->DBNCECON &= ~(0xF << 0); 	// Clear first
	GPIO->DBNCECON |= (8 << 0); 		// Sampling Interrupt Input every 256 clocks

	// DataSheet p192 & p193
	PB->PMD &= (~(0x03 << 30)); 		// Set pin mode to input
	PB->DBEN |= (1 << 15); 					// Debounce
	PB->IMD &= (~(0x01 << 15)); 		// 0: Trigger on edge -> Can control debounce
	PB->IEN |= (0x01 << 15); 				// Falling edge trigger

	//NVIC interrupt configuration for B.15 interrupt source
	NVIC->ISER[0] |= (1 << 3); 			// Interrupt enable
	NVIC->IP[0] &= (~(3 << 30)); 		// ISR priority
	// GPIO Configuration ends-------------
}

void SPI3_Config(void) {
	SYS->GPD_MFP |= 1 << 11; 		//1: PD11 is configured for alternative func-tion
	SYS->GPD_MFP |= 1 << 9; 		//1: PD9 is configured for alternative function
	SYS->GPD_MFP |= 1 << 8; 		//1: PD8 is configured for alternative function

	SPI3->CNTRL &= ~(1 << 23); 	//0: disable variable clock feature
	SPI3->CNTRL &= ~(1 << 22); 	//0: disable two bits transfer mode
	SPI3->CNTRL &= ~(1 << 18); 	//0: select Master mode
	SPI3->CNTRL &= ~(1 << 17); 	//0: disable SPI interrupt
	SPI3->CNTRL |= 1 << 11; 		//1: SPI clock idle high
	SPI3->CNTRL &= ~(1 << 10); 	//0: MSB is sent first
	SPI3->CNTRL &= ~(3 << 8); 	//00: one transmit/receive word will be exe-cuted in one data transfer

	SPI3->CNTRL &= ~(31 << 3); 	//Transmit/Receive bit length
	SPI3->CNTRL |= 9 << 3;     	//9: 9 bits transmitted/received per data transfer

	SPI3->CNTRL |= (1 << 2);  	//1: Transmit at negative edge of SPI CLK
	SPI3->DIVIDER = 0; // SPI clock divider. SPI clock = HCLK / ((DIVID-ER+1)*2). HCLK = 50 MHz
}

void UART0_Config(void) {
	// UART0 pin configuration. PB.0 pin is for UART0 RX
	PB->PMD &= ~(0x03 << 0);			// PB.0 is input pin
	SYS->GPB_MFP |= (0x01 << 0);  // GPB_MFP[0] = 1 -> PB.0 is UART0 RX pin

	// UART0 operation configuration
	UART0->FCR |= (0x03 << 1);    // Clear both TX & RX FIFO
	UART0->FCR &= ~(0x0F << 16);  // FIFO Trigger Level is 1 byte

	UART0->FCR &= ~(0xF << 4);		// RX FIFO by 1 byte (8bits)

	UART0->LCR &= ~(0x01 << 3);   // No parity bit
	UART0->LCR &= ~(0x01 << 2);   // One stop bit
	UART0->LCR |= (0x03 << 0);    // 8 data bit

	// UART0 Interrupt
	UART0->IER |= (0x01 << 0);    // Enable UART0 Interrupt RDA_IEN

	// Vector Number (DataSheet p95) NVIC->ISER[0] (1 << x)
	// UART02_INT : 28 (12)
	NVIC->ISER[0] |= (1 << 12);    // UART02_INT
	// Interrupt Priority (DataSheet p105) NVIC->IP[3]  (value << x)
	// value 0: Highest priority | 3: Lowest Priority
	// UART02_INT : [7:6] 	(12)
	NVIC->IP[3] &= ~(3 << 6); // UART02_INT Highest priority

	// Baud Rate config: BRD/A = 1, DIV_X_EN=0
    // Baud Rate from Terminal software: 19200 bps -> A=70
	// --> Mode 0, Baud rate = UART_CLK/[16*(A+2)] = 22.1184 MHz/[16*(70+2)]= 19200 bps
	UART0->BAUD &= ~(0x0FFFF << 0);
	UART0->BAUD |= 70;
	UART0->BAUD &= ~(0x03 << 28); // Mode 0
}

char UART0_GetChar(void) {
	while (1) {
		if(!(UART0->FSR & (0x01 << 14))){
			return(UART0->DATA);
		}
	}
}

void UART02_IRQHandler(void) {
	char input = UART0_GetChar();
	
	if (input == '0' || input == '1') {
		// Store value
		map[mapY][mapX] = input - '0';
		
		// Increase load index
		if (mapY == 7) {
			mapX++;
			mapY = 0;
		} else {
			mapY++;
		}
	}
	
	if (mapX == 7 && mapY == 7) {
		PC->DOUT ^= (1 << 13); // Notify that the map is loaded all - LED6 ON
		CLK_SysTickDelay(50000);
		cleanLCD = 1;
		state = 1;
	}
}

void EINT1_IRQHandler(void) {
	if (state == 1) {
		// shot to start game
		cleanLCD = 1;
		state = 2;
	} else if (state == 2) {
		// Check shot condition
		if (view[y-1][x-1] == 0) {
			// Process shot
			if (map[y-1][x-1] == 1) {
				view[y-1][x-1] = 1;
                
				// Generate systick delay for led
				for (int i = 0; i < (3 * 2); i++) {
								PC->DOUT ^= (1 << 12); // LED5 ON
								CLK_SysTickDelay(25000);
				}
				
				// If hit, hit increases by 1
				hit++;
			}
		}
        
		// Increase shot
		shot++;
		
		// Check if whether game is over
		if ((shot == 16) || (hit == 10)) {
			// Change state to Game Over
			cleanLCD = 1;
			state = 3;
		} 
		
		// Reset value of x and y coordinates for the next shot
		x = 1;
		y = 1;
		isY = false;
		
	} else if (state == 3) {
		// shot to reset game
		cleanLCD = 1;
		state = 1;
		// Reset value
		reset();
	}
	// Clear interrupt
	PB->ISRC |= (1 << 15);
}

// 7-segment sweeping
void TMR0_IRQHandler(void) {
	keyPad_pressed();
	if (digit == 3) {
			digit = 0;
	} else {
			digit++;
	}
	displayDigit();
      
	TIMER0->TISR |= (1 << 0);
}

void LCD_start(void) {
	LCD_command(0xE2); // Set system reset
	LCD_command(0xA1); // Set Frame rate 100 fps
	LCD_command(0xEB); // Set LCD bias ratio E8~EB for 6~9 (min~max)
	LCD_command(0x81); // Set V BIAS potentiometer
	LCD_command(0xA0); // Set V BIAS potentiometer: A0 ()
	LCD_command(0xC0);
	LCD_command(0xAF); // Set Display Enable
}

void LCD_command(unsigned char temp) {
	SPI3->SSR |= 1 << 0;
	SPI3->TX[0] = temp;
	SPI3->CNTRL |= 1 << 0;
	while (SPI3->CNTRL & (1 << 0));
	SPI3->SSR &= ~(1 << 0);
}

void LCD_data(unsigned char temp) {
	SPI3->SSR |= 1 << 0;
	SPI3->TX[0] = 0x0100 + temp;
	SPI3->CNTRL |= 1 << 0;
	while (SPI3->CNTRL & (1 << 0));
	SPI3->SSR &= ~(1 << 0);
}

void LCD_clear(void) {
	int16_t i;
	LCD_SetAddress(0x0, 0x0);
	for (i = 0; i < 132 * 8; i++) {
		LCD_data(0x00);
	}
}

void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr) {
	LCD_command(0xB0 | PageAddr);
	LCD_command(0x10 | (ColumnAddr >> 4) & 0xF);
	LCD_command(0x00 | (ColumnAddr & 0xF));
}
//------------------------ main.c CODE ENDS --------------------------------------------------
