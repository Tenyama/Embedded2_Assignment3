#include <stdio.h>
#include "NUC100Series.h"
#include "SYS.h"
#include "ADC.h"
#include "SPI.h"

#define TEAM_NAME "Team 8"

void SYS_Init(void);
void ADC_Init(void);
void SPI2_Init(void);
void send_SPI2(const char *data);

int main(void) {
    uint32_t adc_value;
    float voltage;

    SYS_Init();
    ADC_Init();
    SPI2_Init();

    ADC->ADCR |= (1 << 11); // Start ADC channel 7 conversion

    while (1) {
        while (!(ADC->ADSR & (1 << 0))); // Wait for conversion to complete (ADF=1)
        ADC->ADSR |= (1 << 0); // Write 1 to clear ADF

        adc_value = ADC->ADDR[7] & 0xFFF; // Read ADC result
        voltage = (adc_value * 3.3f) / 4095.0f;

        if (voltage > 2.0f) {
            send_SPI2(TEAM_NAME);
        }
    }
}

void SYS_Init(void) {
    SYS_UnlockReg();

    CLK->PWRCON |= (1 << 0); // Enable external 12 MHz XTAL
    while (!(CLK->CLKSTATUS & (1 << 0))); // Wait for 12 MHz clock ready

    CLK->PLLCON &= ~(1 << 19); // PLL input is HXT
    CLK->PLLCON &= ~(1 << 16); // PLL in normal mode
    CLK->PLLCON &= ~(0x01FF << 0);
    CLK->PLLCON |= 48;
    CLK->PLLCON &= ~(1 << 18); // Enable PLLOUT
    while (!(CLK->CLKSTATUS & (1 << 2))); // Wait for PLL ready

    CLK->CLKSEL0 &= ~(0x07 << 0); // Switch HCLK clock source to HXT
    CLK->CLKSEL0 |= (0x02 << 0);

    CLK->CLKDIV &= ~(0x0F << 0); // Set HCLK divider to 1

    CLK->APBCLK |= (1 << 28); // Enable ADC clock
    CLK->CLKSEL1 &= ~(0x03 << 2); // ADC clock source is 12 MHz
    CLK->CLKDIV &= ~(0x0FF << 16);
    CLK->CLKDIV |= (0x0B << 16); // ADC clock divider is (11+1) --> 1 MHz

    CLK->APBCLK |= (1 << 14); // Enable SPI2 clock
    CLK->CLKDIV1 &= ~(0x0F << 20);
    CLK->CLKDIV1 |= (0x0B << 20); // SPI2 clock divider is (11+1) --> 1 MHz

    SystemCoreClockUpdate();
    SYS_LockReg();
}

void ADC_Init(void) {
    PA->PMD &= ~(0b11 << 14); // Clear PA.7 mode
    PA->PMD |= (0b00 << 14);  // Set PA.7 to input mode
    PA->OFFD |= (1 << 7); // PA.7 digital input path disabled
    SYS->GPA_MFP |= (1 << 7); // GPA_MFP[7] = 1 for ADC7
    SYS->ALT_MFP &= ~(1 << 11); // ALT_MFP[11] = 0 for ADC7

    ADC->ADCR |= (0x03 << 2); // Continuous scan mode
    ADC->ADCR &= ~(1 << 1); // ADC interrupt is disabled
    ADC->ADCR |= (1 << 0); // ADC is enabled
    ADC->ADCHER &= ~(0x03 << 8); // ADC7 input source is external pin
    ADC->ADCHER |= (1 << 7); // Enable ADC channel 7
}

void SPI2_Init(void) {
    SYS->GPD_MFP |= (1 << 0) | (1 << 1) | (1 << 3); // Configure SPI2 pins

    SPI2->CNTRL &= ~(1 << 23); // Disable variable clock feature
    SPI2->CNTRL &= ~(1 << 22); // Disable two bits transfer mode
    SPI2->CNTRL &= ~(1 << 18); // Select Master mode
    SPI2->CNTRL &= ~(1 << 17); // Disable SPI interrupt
    SPI2->CNTRL |= (1 << 11); // SPI clock idle high
    SPI2->CNTRL &= ~(1 << 10); // MSB sent first
    SPI2->CNTRL &= ~(3 << 8); // One transmit/receive word in one data transfer
    SPI2->CNTRL &= ~(31 << 3); // Transmit/Receive bit length
    SPI2->CNTRL |= (7 << 3); // 8 bits transmitted/received per data transfer
    SPI2->CNTRL |= (1 << 2); // Transmit at negative edge of SPI CLK
    SPI2->DIVIDER = 11; // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 50 MHz
}

void send_SPI2(const char *data) {
    while (*data) {
        SPI2->TX[0] = *data++;
        SPI2->CNTRL |= (1 << 0); // Start transfer
        while (SPI2->CNTRL & (1 << 0)); // Wait for transfer to complete
    }
}