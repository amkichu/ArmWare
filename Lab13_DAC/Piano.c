// Piano.c
// Runs on LM4F120 or TM4C123, 
// edX lab 13 
// There are four keys in the piano
// Daniel Valvano
// December 29, 2014

// Port E bits 3-0 have 4 piano keys

#include "Piano.h"
#include "..//tm4c123gh6pm.h"


// **************Piano_Init*********************
// Initialize piano key inputs
// Input: none
// Output: none
void Piano_Init(void){unsigned long volatile delay;
// configure input from PE3-PE0
    SYSCTL_RCGC2_R |= 0x00000020;   // 1) activate clock for Port E
    delay = SYSCTL_RCGC2_R;         // allow time for clock to start
    GPIO_PORTE_LOCK_R = 0x4C4F434B; // 2) unlock GPIO Port E
    GPIO_PORTE_CR_R = 0x1F;         // allow changes to PE4-0
// only PE3-PE0 needs to be unlocked, other bits can't be locked
    GPIO_PORTE_AMSEL_R &= ~0x0F;      // 3) disable analog on PE3-PE0
    GPIO_PORTE_PCTL_R &= ~0x0000FFFF; // 4) PCTL GPIO on PE3-PE0
    GPIO_PORTE_DIR_R &= ~0x0F;        // 5) direction PE3-PE0 input
    GPIO_PORTE_AFSEL_R &= ~0x0F;      // 6) PE3-PE0 regular port function
    GPIO_PORTE_DEN_R |= 0x0F;         // 7) enable PE3-PE0 digital port
}
// **************Piano_In*********************
// Input from piano key inputs
// Input: none 
// Output: 0 to 15 depending on keys
// 0x01 is key 0 pressed, 0x02 is key 1 pressed,
// 0x04 is key 2 pressed, 0x08 is key 3 pressed
unsigned long Piano_In(void){
    // unsigned long SwitchPressed = 0;
    //      if (GPIO_PORTE_DATA_R & 0x01) SwitchPressed = 1;         // PE0 (0x01) pressed or not pressed
    // else if (GPIO_PORTE_DATA_R & 0x02) SwitchPressed = 2;         // PE1 (0x02) pressed or not pressed
    // else if (GPIO_PORTE_DATA_R & 0x04) SwitchPressed = 4;         // PE2 (0x04) pressed or not pressed
    // else if (GPIO_PORTE_DATA_R & 0x08) SwitchPressed = 8;         // PE3 (0x08) pressed or not pressed
    // else                               SwitchPressed = 0;
    // return SwitchPressed;
    return (GPIO_PORTE_DATA_R & 0xF)^0xF;
}
