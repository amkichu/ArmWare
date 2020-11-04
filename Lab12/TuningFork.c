// TuningFork.c Lab 12
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to create a squarewave at 440Hz.  
// There is a positive logic switch connected to PA3, PB3, or PE3.
// There is an output on PA2, PB2, or PE2. The output is 
//   connected to headphones through a 1k resistor.
// The volume-limiting resistor can be any value from 680 to 2000 ohms
// The tone is initially off, when the switch goes from
// not touched to touched, the tone toggles on/off.
//                   |---------|               |---------|     
// Switch   ---------|         |---------------|         |------
//
//                    |-| |-| |-| |-| |-| |-| |-|
// Tone     ----------| |-| |-| |-| |-| |-| |-| |---------------
//
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */


#include "TExaS.h"
#include "..//tm4c123gh6pm.h"

// The #define statement SYSDIV2 initializes
// the PLL to the desired frequency.
#define SYSDIV2 4
// bus frequency is 400MHz/(SYSDIV2+1) = 400MHz/(4+1) = 80 MHz

// basic functions defined at end of startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode

volatile unsigned long lastSwitch = 0;
volatile unsigned long ledOut = 0;

// input from PA3, output from PA2, SysTick interrupts
void Sound_Init(void){ volatile unsigned long delay;
// configure the system to get its clock 80 MHz from the PLL
  // 0) configure the system to use RCC2 for advanced features
  //    such as 400 MHz PLL and non-integer System Clock Divisor
  SYSCTL_RCC2_R |= SYSCTL_RCC2_USERCC2;
  // 1) bypass PLL while initializing
  SYSCTL_RCC2_R |= SYSCTL_RCC2_BYPASS2;
  // 2) select the crystal value and oscillator source
  SYSCTL_RCC_R &= ~SYSCTL_RCC_XTAL_M;   // clear XTAL field
  SYSCTL_RCC_R += SYSCTL_RCC_XTAL_16MHZ;// configure for 16 MHz crystal
  SYSCTL_RCC2_R &= ~SYSCTL_RCC2_OSCSRC2_M;// clear oscillator source field
  SYSCTL_RCC2_R += SYSCTL_RCC2_OSCSRC2_MO;// configure for main oscillator source
  // 3) activate PLL by clearing PWRDN
  SYSCTL_RCC2_R &= ~SYSCTL_RCC2_PWRDN2;
  // 4) set the desired system divider and the system divider least significant bit
  SYSCTL_RCC2_R |= SYSCTL_RCC2_DIV400;  // use 400 MHz PLL
  SYSCTL_RCC2_R = (SYSCTL_RCC2_R&~0x1FC00000) // clear system clock divider field
                  + (SYSDIV2<<22);      // configure for 80 MHz clock
  // 5) wait for the PLL to lock by polling PLLLRIS
  while((SYSCTL_RIS_R&SYSCTL_RIS_PLLLRIS)==0){};
  // 6) enable use of PLL by clearing BYPASS
  SYSCTL_RCC2_R &= ~SYSCTL_RCC2_BYPASS2;

  lastSwitch = 0;
  ledOut = 0;
// configure output from PA2
  SYSCTL_RCGC2_R |= 0x01;           // 1) activate clock for Port A
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
                                    // 2) no need to unlock PA2
  GPIO_PORTA_PCTL_R &= ~0x00000F00; // 3) regular GPIO
  GPIO_PORTA_AMSEL_R &= ~0x04;      // 4) disable analog function on PA2
  GPIO_PORTA_DIR_R |= 0x04;         // 5) set direction to output
  GPIO_PORTA_AFSEL_R &= ~0x04;      // 6) regular port function
  GPIO_PORTA_DEN_R |= 0x04;         // 7) enable digital port

// configure input from PA3
  GPIO_PORTA_AMSEL_R &= ~0x08;      // 3) disable analog on PA3
  GPIO_PORTA_PCTL_R &= ~0x0000F000; // 4) PCTL GPIO on PA3
  GPIO_PORTA_DIR_R &= ~0x08;        // 5) direction PA3 input
  GPIO_PORTA_AFSEL_R &= ~0x08;      // 6) PA3 regular port function
  GPIO_PORTA_DEN_R |= 0x08;         // 7) enable PA3 digital port

// configure SysTick interrupts
  NVIC_ST_CTRL_R = 0;           // disable SysTick during setup
  NVIC_ST_RELOAD_R = 90908;     // reload value
  NVIC_ST_CURRENT_R = 0;        // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
                                // enable SysTick with core clock and interrupts
  NVIC_ST_CTRL_R = 0x07;
}

// called at 880 Hz
// Executed every 1/880 Hz = 1.13636 ms
// clock @ 80MHz
// Two global variables
// 1. to remember the previous switch value at the last ISR execution (pressed or not pressed), 
// 2. to know what to do (toggle or quiet). 
// If the switch was not pressed during the last ISR execution and 
// is pressed during the current ISR, 
// // then you know the switch was just pressed.
// void SysTick_Handler(void){
//     lastSwitch ^= 0x01;
//     if (GPIO_PORTA_DATA_R&0x08){             // PA3 (0x08) pressed or not pressed
//         if(lastSwitch) ledOut = 1;
//         else           ledOut = 0;
//     }

//     if (ledOut) GPIO_PORTA_DATA_R ^= 0x04;          // toggle PA2
//     else        GPIO_PORTA_DATA_R = 0;
// }
void SysTick_Handler(void){
    if (GPIO_PORTA_DATA_R&0x08){                                // PA3 (0x08) pressed or not pressed
             if ((lastSwitch==1)&&(ledOut==1)) ledOut = 1;
        else if ((lastSwitch==1)&&(ledOut==0)) ledOut = 0;
        else if ((lastSwitch==0)&&(ledOut==1)) ledOut = 0;
        else ledOut = 1;
    }
    if (ledOut==1) GPIO_PORTA_DATA_R = GPIO_PORTA_DATA_R ^= 0x04;           // toggle PA2
    else           GPIO_PORTA_DATA_R = 0;
    if(GPIO_PORTA_DATA_R&0x08) lastSwitch = 1;
    else lastSwitch = 0;
}

int main(void){// activate grader and set system clock to 80 MHz
  TExaS_Init(SW_PIN_PA3, HEADPHONE_PIN_PA2,ScopeOn); 
  Sound_Init();
  EnableInterrupts();   // enable after all initialization are done
  while(1){
    // main program is free to perform other tasks
    // do not use WaitForInterrupt() here, it may cause the TExaS to crash
  }
}
