#include <stdio.h>
#include "NUC100Series.h"

int main(void){
	PC->PMD &= (~(0b11<<24));
	PC->PMD |= (0b01 << 24);
	PC->PMD &= (~(0b11<<30));
	PC->PMD |= (0b01 << 30);
	
	
	PB->PMD &= (~(0x03 << 30));
	 
	while(1){
		if(!(PB->PIN &(1<<15))){
			PC->DOUT &= ~(1<<12);
			PC->DOUT |= (1 << 15);
		} else {
			PC->DOUT |= (1<<12);
			PC->DOUT &= ~ (1 << 15);
		}
	}
	
}