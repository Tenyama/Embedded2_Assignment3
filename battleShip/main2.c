//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "LCD.h"

#define HXT_STATUS 1 << 0
#define PLL_STATUS 1 << 2
#define LIRC_STATUS 1 << 3  // 10 kHz
// Macro define when there are keys shot in each column
#define COL3_PRESSED (!(PA->PIN & (1 << 0)))
#define COL2_PRESSED (!(PA->PIN & (1 << 1)))
#define COL1_PRESSED (!(PA->PIN & (1 << 2)))
#define TIMER0_COUNT 100 - 1

void ConfigureSystem(void);
void ConfigureClock(void);
void ConfigureGPIO(void);
void ConfigureSPI3(void);
void ConfigureUART0(void);
char UART0_ReadChar(void);
void UART02_InterruptHandler(void);
void TIMER0_InterruptHandler(void);
void TIMER1_InterruptHandler(void);

void LCD_Initialize(void);
void LCD_SendCommand(unsigned char command);
void LCD_SendData(unsigned char data);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t page, uint8_t column);

int digitPatterns[] = {
    0b10000010,  // Number 0          // ---a----
    0b11101110,  // Number 1          // |      |
    0b00000111,  // Number 2          // f      b
    0b01000110,  // Number 3          // |      |
    0b01101010,  // Number 4          // ---g----
    0b01010010,  // Number 5          // |      |
    0b00010010,  // Number 6          // e      c
    0b11100110,  // Number 7          // |      |
    0b00000010,  // Number 8          // ---d----
    0b01000010,  // Number 9
    0b11111111   // Blank LED 
};

volatile int currentRow = 0;
volatile int currentCol = 0;
volatile int clearLCDFlag = 0;
volatile int gameState = 0;
volatile int currentDigit = 0;
volatile bool settingY = false;
volatile int xCoordinate = 1;
volatile int yCoordinate = 1;
volatile int shotsFired = 0;
volatile int hitsScored = 0;
volatile int buzzerState = 0;
volatile bool isHit = false;
volatile bool isGameOver = false;
volatile int countBlink = 0;
volatile int buzzerCount = 0;

volatile int gameMap[8][8];
volatile int displayMap[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}  
};

void DisplayMap(void) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (displayMap[row][col] == 0) {
                printC_5x7(row * 16, col * 8, '-');
            } else if (displayMap[row][col] == 1) {
                printC_5x7(row * 16, col * 8, 'x');
            }
        }
    }
}

// Reset map to the original setting
void ResetGame(void) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            displayMap[row][col] = 0;
        }
    }
    settingY = false;
    shotsFired = 0;
    hitsScored = 0;
    isHit = false;
    isGameOver = false;
    countBlink = 0;
    buzzerCount = 0;
}

// Keypad pressed button value setting
void SetKeypadValue(int value) {
    if (!settingY) {
        if (value == 9) {
            // If K9 is pressed, set to Y-coordinate setting
            settingY = true;
        } else {
            // Assign value to X-coordinate
            xCoordinate = value;
        }
    } else {
        if (value == 9) {
            // If K9 is pressed, set to X-coordinate setting
            settingY = false;
        } else {
            // Assign value to Y-coordinate
            yCoordinate = value;
        }
    }
}

static void SearchColumn1(void) {
    // Drive ROW1 output pin as LOW. Other ROW pins as HIGH
    PA->DOUT &= ~(1 << 3);
    PA->DOUT |= (1 << 4);
    PA->DOUT |= (1 << 5);
    if (COL1_PRESSED) {
        SetKeypadValue(1);
        return;
    } else {
        // Drive ROW2 output pin as LOW. Other ROW pins as HIGH
        PA->DOUT |= (1 << 3);
        PA->DOUT &= ~(1 << 4);
        PA->DOUT |= (1 << 5);
        if (COL1_PRESSED) {
            // If column1 is LOW, detect key press as K4 (KEY 4)
            SetKeypadValue(4);
            return;
        } else {   
            // Drive ROW3 output pin as LOW. Other ROW pins as HIGH
            PA->DOUT |= (1 << 3);
            PA->DOUT |= (1 << 4);
            PA->DOUT &= ~(1 << 5);
            if (COL1_PRESSED) {
                // If column1 is LOW, detect key press as K7 (KEY 7)
                SetKeypadValue(7);
                return;
            } else
                return;
        }
    }
}

static void SearchColumn2(void) {
    // Drive ROW1 output pin as LOW. Other ROW pins as HIGH
    PA->DOUT &= ~(1 << 3);
    PA->DOUT |= (1 << 4);
    PA->DOUT |= (1 << 5);
    if (COL2_PRESSED) {
        // If column2 is LOW, detect key press as K2 (KEY 2)
        SetKeypadValue(2);
        return;
    } else {
        // Drive ROW2 output pin as LOW. Other ROW pins as HIGH
        PA->DOUT |= (1 << 3);
        PA->DOUT &= ~(1 << 4);
        PA->DOUT |= (1 << 5);
        if (COL2_PRESSED) {
            // If column2 is LOW, detect key press as K5 (KEY 5)
            SetKeypadValue(5);
            return;
        } else {
            // Drive ROW3 output pin as LOW. Other ROW pins as HIGH
            PA->DOUT |= (1 << 3);
            PA->DOUT |= (1 << 4);
            PA->DOUT &= ~(1 << 5);
            if (COL2_PRESSED) {
                // If column3 is LOW, detect key press as K8 (KEY 8)
                SetKeypadValue(8);
                return;
            } else
                return;
        }
    }
}

static void SearchColumn3(void) {
    // Drive ROW1 output pin as LOW. Other ROW pins as HIGH
    PA->DOUT &= ~(1 << 3);
    PA->DOUT |= (1 << 4);
    PA->DOUT |= (1 << 5);
    if (COL3_PRESSED) {
        // If column3 is LOW, detect key press as K3 (KEY 3)
        SetKeypadValue(3);
        return;
    } else {
        // Drive ROW2 output pin as LOW. Other ROW pins as HIGH
        PA->DOUT |= (1 << 3);
        PA->DOUT &= ~(1 << 4);
        PA->DOUT |= (1 << 5);
        if (COL3_PRESSED) {
            // If column3 is LOW, detect key press as K6 (KEY 6)
            SetKeypadValue(6);
            return;
        } else {
            // Drive ROW3 output pin as LOW. Other ROW pins as HIGH
            PA->DOUT |= (1 << 3);
            PA->DOUT |= (1 << 4);
            PA->DOUT &= ~(1 << 5);
            if (COL3_PRESSED) {
                // If column3 is LOW, detect key press as K9 (KEY 9)
                SetKeypadValue(9);
                return;
            } else
                return;
        }
    }
}

void HandleKeypadPress(void) {
    // Turn all Rows to LOW 
    PA->DOUT &= ~(1 << 3);
    PA->DOUT &= ~(1 << 4);
    PA->DOUT &= ~(1 << 5);
    // Check for key press in key matrix
    if (COL1_PRESSED) {
        SearchColumn1();
    } else if (COL2_PRESSED) {
        SearchColumn2();
    } else if (COL3_PRESSED) {
        SearchColumn3();
    }
}

// Display digit on 7-segment display (U13 or U14)
void DisplayDigit(int digit, int value) {
    uint32_t mask = 0xFF;
    uint32_t clearMask = (mask << (8 * digit));
    uint32_t setMask = (digitPatterns[value] << (8 * digit));
    PC->DOUT = (PC->DOUT & ~clearMask) | setMask;
}

void UpdateDisplayDigit(void) {
    if (currentDigit == 0 && !settingY) {
        DisplayDigit(currentDigit, xCoordinate);
    } else if (currentDigit == 1 && settingY) {
        DisplayDigit(currentDigit, yCoordinate);
    } else if (currentDigit == 2) {
        DisplayDigit(currentDigit, shotsFired / 10);
    } else if (currentDigit == 3) {
        DisplayDigit(currentDigit, shotsFired % 10);
    }
}

// UART interrupt handler
void UART02_InterruptHandler(void) {
    char input = UART0_ReadChar();
    if (input == '0' || input == '1') {
        // Store value
        gameMap[currentRow][currentCol] = input - '0';
        // Increase load index
        if (currentRow == 7) {
            currentCol++;
            currentRow = 0;
        } else {
            currentRow++;
        }
    }
    if (currentCol == 7 && currentRow == 7) {
        PC->DOUT ^= (1 << 13); // Notify that the map is loaded all - LED6 ON
        CLK_SysTickDelay(50000);
        clearLCDFlag = 1;
        gameState = 1;
    }
}

void EINT1_IRQHandler(void) {
    if (gameState == 1) {
        // Shot to start game
        clearLCDFlag = 1;
        gameState = 2;
    } else if (gameState == 2) {
        shotsFired++;
        clearLCDFlag = 1;
        if (displayMap[yCoordinate - 1][xCoordinate - 1] == 0) {
            if (gameMap[yCoordinate - 1][xCoordinate - 1] == 1) {
                displayMap[yCoordinate - 1][xCoordinate - 1] = 1;
                printS_5x7(5, 20, "Hit!");
                isHit = true;
                hitsScored++;
            } else {
                printS_5x7(5, 20, "Miss");
            }
        }
        if ((shotsFired == 16) || (hitsScored == 10)) {
            clearLCDFlag = 1;
            gameState = 3;
            isGameOver = true;
        }
    } else if (gameState == 3) {
        // Reset game on button press
        ResetGame();
        clearLCDFlag = 1;
        gameState = 0;
        isGameOver = false;
    }
    PB->ISRC |= (1 << 15);
}

// Timer interrupt handler
void TIMER0_InterruptHandler(void) {
    HandleKeypadPress();
    if (currentDigit == 3) {
        currentDigit = 0;
    } else {
        currentDigit++;
    }
    UpdateDisplayDigit();
    TIMER0->TISR |= (1 << 0);
}

void TIMER1_InterruptHandler(void) {
    if (isHit) {
        if (countBlink < 6) {
            PC->DOUT ^= (1 << 12); // Toggle PC12
            countBlink++;
        } else {
            // Reset everything once blinking is done
            isHit = false;
            countBlink = 0;
            PC->DOUT |= (1 << 12); // Ensure LED is turned off
        }
    }

    if (isGameOver && buzzerCount < 10) {
        PB->DOUT ^= (1 << 11); // Toggle buzzer
        buzzerCount++;
    }

    TIMER1->TISR |= (1 << 0); // clear timer1 interrupt flag
}

// System initialization
void ConfigureSystem(void) {
    ConfigureClock();
    ConfigureGPIO();
    ConfigureSPI3();
    ConfigureUART0();
    // Configure TIMER0 for 7-segment scanning rate (4 ms interval)
    TIMER0->TCSR = 0x00008000;
    TIMER0->TCSR &= ~(1 << 26);
    TIMER0->TCSR |= (0x2 << 27);
    TIMER0->TCSR &= ~(1 << 24);
    TIMER0->TCSR |= (1 << 16);
    TIMER0->TCSR &= ~(1 << 29);
    TIMER0->TISR |= (1 << 0);
    TIMER0->TCMPR = TIMER0_COUNT;
    TIMER0->TCSR |= (1 << 30);
    TIMER0->TCSR |= (1 << 29);

    // Configure TIMER1 for hit indication (100 ms interval)
    TIMER1->TCSR = 0x00008000;
    TIMER1->TCSR &= ~(1 << 26);
    TIMER1->TCSR |= (0x2 << 27);
    TIMER1->TCSR &= ~(1 << 24);
    TIMER1->TCSR |= (1 << 16);
    TIMER1->TCSR &= ~(1 << 29);
    TIMER1->TISR |= (1 << 0);
    TIMER1->TCMPR = 100000; // 100 ms interval
    TIMER1->TCSR |= (1 << 30);
    TIMER1->TCSR |= (1 << 29);
}

void ConfigureClock(void) {
    SYS_UnlockReg();
    CLK->PWRCON |= HXT_STATUS;
    while (!(CLK->CLKSTATUS & HXT_STATUS));
    CLK->PLLCON &= ~(1 << 19);
    CLK->PLLCON &= ~(1 << 16);
    CLK->PLLCON &= ~((0x01FF) << 0);
    CLK->PLLCON |= 48;
    CLK->PLLCON &= ~(1 << 18);
    while (!(CLK->CLKSTATUS & PLL_STATUS));
    CLK->CLKSEL0 &= ~(0x07 << 0);
    CLK->CLKSEL0 |= (0x02 << 0);
    CLK->CLKDIV &= ~(0x0F << 0);
    SYS_LockReg();
}

void ConfigureGPIO(void) {
    SYS->GPBMFP |= (1UL << 15);
    GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
    PB->DBEN |= (1 << 15);
    PB->IMD &= ~(1 << 15);
    PB->IEN |= (1 << 15);
    NVIC_EnableIRQ(EINT1_IRQn);

    GPIO_SetMode(PA, BIT0, GPIO_MODE_INPUT);
    GPIO_SetMode(PA, BIT1, GPIO_MODE_INPUT);
    GPIO_SetMode(PA, BIT2, GPIO_MODE_INPUT);
    GPIO_SetMode(PA, BIT3, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT5, GPIO_MODE_OUTPUT);

    GPIO_SetMode(PC, BIT12, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, BIT13, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PC, 0xF00, GPIO_MODE_OUTPUT);
}

void ConfigureSPI3(void) {
    SPI3->CNTRL = 0;
    SPI3->CNTRL &= ~(1 << 23);
    SPI3->CNTRL &= ~(1 << 22);
    SPI3->CNTRL |= (1 << 18);
    SPI3->CNTRL &= ~(1 << 17);
    SPI3->CNTRL &= ~(1 << 13);
    SPI3->CNTRL &= ~(1 << 12);
    SPI3->CNTRL &= ~(1 << 11);
    SPI3->CNTRL &= ~((0x3F) << 8);
    SPI3->CNTRL |= (9 << 8);
    SPI3->CNTRL &= ~(1 << 7);
    SPI3->CNTRL |= (1 << 6);
    SPI3->DIVIDER = 0;
    SPI3->SSR |= (1 << 0);
}

void ConfigureUART0(void) {
    SYS->GPBMFP |= (1UL << 0);
    SYS->GPBMFP |= (1UL << 1);
    SYS_ResetModule(UART0_RST);
    UART0->LCR |= (0x3 << 0);
    UART0->LCR &= ~(1 << 2);
    UART0->LCR |= (1 << 3);
    UART0->BAUD &= ~(0x3 << 28);
    UART0->BAUD &= ~(0xF << 24);
    UART0->BAUD |= (0x3 << 24);
    UART0->BAUD &= ~(0xFFFF << 0);
    UART0->BAUD |= 70;
    UART0->FCR |= (0x3 << 1);
    NVIC_EnableIRQ(UART02_IRQn);
    UART0->IER |= (1 << 0);
}

char UART0_ReadChar(void) {
    while (UART0->FSR & (1 << 14));
    return UART0->DATA;
}

int main(void) {
    ConfigureSystem();
    LCD_Initialize();
    while (1) {
        if (clearLCDFlag) {
            LCD_Clear();
            clearLCDFlag = 0;
        }
        switch (gameState) {
            case 0: // State Uploading Map
                printS_5x7(5, 10, "Welcome to Battleship");
                printS_5x7(5, 20, "Team 8");
                printS_5x7(5, 30, "Uploading Game Map");
                printS_5x7(5, 40, "from UART0 .......");
                break;
            case 1: // State Uploading Map Finish
                printS_5x7(35, 10, "Battleship");
                printS_5x7(5, 30, "Map Loaded Successfully");
                printS_5x7(5, 40, "Push button SW_INT1");
                printS_5x7(5, 50, "to start the game!");
                break;
            case 2: // State Playing Game
                DisplayMap();
                printS_5x7(5, 0, "Battleship");
                printS_5x7(5, 10, "Firing at: ");
                printC_5x7(70, 10, xCoordinate + '0');
                printC_5x7(80, 10, yCoordinate + '0');
                break;
            case 3: // State Game Over
                printS_5x7(35, 10, "Game Over");
                if (hitsScored == 10) {
                    printS_5x7(5, 30, "You win!");
                } else {
                    printS_5x7(5, 30, "You lose!");
                }
                printS_5x7(5, 40, "Press SW_INT1 to reset.");
                break;
            default:
                break;
        }
    }
}
//------------------------------------------- main.c CODE ENDS --------------------------------------------------
