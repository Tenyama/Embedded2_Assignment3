#include <stdio.h>
#include "NUC100Series.h"
#include "LCD.h"

#define ON 1
#define OFF 0

void System_Config(void);
void SPI3_Config(void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void UART0_Config(void);
void UART02_IRQHandler(void);
void EINT1_Config(void);
void EINT1_IRQHandler(void);

void displayWelcomeScreen(void);
void displayMapLoadedScreen(void);
void displayGameField(void);

volatile unsigned char loadedMap[8][8];
volatile int byteCount = 0;
volatile int isMapReady = 0;
volatile int gameCheck = OFF;

int main(void) {
    System_Config();
    SPI3_Config();
    LCD_start();
    LCD_clear();
    UART0_Config();
    EINT1_Config();
    displayWelcomeScreen();

    while (1) {
        if (gameCheck == ON) {
            displayGameField();
            gameCheck = OFF;  // Reset gameCheck to prevent re-displaying
        }
    }
}

void System_Config(void) {
    SYS_UnlockReg();
    CLK->PWRCON |= (0x01 << 0);
    while (!(CLK->CLKSTATUS & (1 << 0)));
    CLK->PLLCON &= ~(1 << 19);
    CLK->PLLCON &= ~(1 << 16);
    CLK->PLLCON &= (~(0x01FF << 0));
    CLK->PLLCON |= 48;
    CLK->PLLCON &= ~(1 << 18);
    while (!(CLK->CLKSTATUS & (0x01 << 2)));
    CLK->CLKSEL0 &= (~(0x07 << 0));
    CLK->CLKSEL0 |= (0x02 << 0);
    CLK->CLKDIV &= (~0x0F << 0);
    CLK->CLKSEL1 |= (0b11 << 24);
    CLK->CLKDIV &= ~(0xF << 8);
    CLK->APBCLK |= (1 << 16);
    CLK->APBCLK |= 1 << 15;
    SYS_LockReg();
}

void SPI3_Config(void) {
    SYS->GPD_MFP |= 1 << 11;
    SYS->GPD_MFP |= 1 << 9;
    SYS->GPD_MFP |= 1 << 8;
    SPI3->CNTRL &= ~(1 << 23);
    SPI3->CNTRL &= ~(1 << 22);
    SPI3->CNTRL &= ~(1 << 18);
    SPI3->CNTRL &= ~(1 << 17);
    SPI3->CNTRL |= 1 << 11;
    SPI3->CNTRL &= ~(1 << 10);
    SPI3->CNTRL &= ~(3 << 8);
    SPI3->CNTRL &= ~(31 << 3);
    SPI3->CNTRL |= 9 << 3;
    SPI3->CNTRL |= (1 << 2);
    SPI3->DIVIDER = 0;
}

void LCD_start(void) {
    LCD_command(0xE2);
    LCD_command(0xA1);
    LCD_command(0xEB);
    LCD_command(0x81);
    LCD_command(0xA0);
    LCD_command(0xC0);
    LCD_command(0xAF);
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

void UART0_Config(void) {
    SYS->GPD_MFP |= 1 << 0;
    SYS->GPD_MFP |= 1 << 1;
    CLK->CLKSEL1 &= ~(0b111 << 24);
    CLK->CLKSEL1 |= (0b011 << 24);
    CLK->APBCLK |= 1 << 16;
    UART0->BAUD &= ~(0xFFF << 0);
    UART0->BAUD |= 0x3F;
    UART0->LCR |= (0b11 << 0);
    UART0->FCR |= 1 << 1;
    NVIC->ISER[0] |= 1 << 12;
    NVIC->IP[3] &= ~(0b11 << 6);
    UART0->IER |= 1 << 0;
}

void UART02_IRQHandler(void) {
    if (UART0->ISR & 1 << 0) {
        char ReceivedByte = UART0->DATA;
        int row = byteCount / 8;
        int col = byteCount % 8;
        loadedMap[row][col] = ReceivedByte - '0';
        byteCount++;
        if (byteCount == 64) {
            isMapReady = 1;
            displayMapLoadedScreen();
        }
    }
}

void EINT1_Config(void) {
    SYS->GPA_MFP &= ~(1 << 15);
    SYS->GPA_MFP |= 1 << 14;
    GPIO_SetMode(PA, BIT15, GPIO_MODE_INPUT);
    GPIO_EnableInt(PA, 15, GPIO_INT_FALLING);
    NVIC_EnableIRQ(EINT1_IRQn);
}

void EINT1_IRQHandler(void) {
    if (PA->ISRC & BIT15) {
        PA->ISRC = BIT15;
        if (isMapReady) {
            gameCheck = ON;
        }
    }
}

void displayWelcomeScreen(void) {
    LCD_clear();
    printS_5x7(20, 24, "Welcome to");
    printS_5x7(30, 32, "Battle Ship Game");
}

void displayMapLoadedScreen(void) {
    LCD_clear();
    printS_5x7(16, 32, "Map Loaded Successfully");
}

void displayGameField(void) {
    LCD_clear();
    int row, col;
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            printC_5x7(col * 8, row * 8, '-');
        }
    }
}
