#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// Function prototypes
void Initialize_LCD(void);
void LCD_command(unsigned char temp);
void LCD_SendData(unsigned char temp);
void Clear_LCD(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void printC_5x7(uint8_t x, uint8_t y, char ascii);
void printS_5x7(uint8_t x, uint8_t y, char *string);

#endif // LCD_H
