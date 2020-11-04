// Sound.c
// Runs on LM4F120 or TM4C123, 
// edX lab 13 
// Use the SysTick timer to request interrupts at a particular period.
// Daniel Valvano, Jonathan Valvano
// December 29, 2014
// This routine calls the 4-bit DAC

#include "Sound.h"
#include "DAC.h"
#include "..//tm4c123gh6pm.h"

unsigned char Index;

// 4-bit 32-element sine wave
const unsigned char SineWave[32] = {8, 9, 11, 12, 13, 14, 14, 15, 15, 15, 14, 14, 13, 12, 11, 9, 8, 7, 5, 4, 3, 2, 2, 1, 1, 1, 2, 2, 3, 4, 5, 7};    // TODO 20201023


    // **************Sound_Init*********************
    // Initialize Systick periodic interrupts
    // Also calls DAC_Init() to initialize DAC
    // Input: none
    // Output: none
void Sound_Init(void){
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
    DAC_Init();                         // Port B is DAC
    Index = 0;
// For heartbeat in PF2
    SYSCTL_RCGCGPIO_R |= 0x00000020; // activate port F
    GPIO_PORTF_DIR_R |= 0x04;        // make PF2 out (built-in LED)
    GPIO_PORTF_AFSEL_R &= ~0x04;     // disable alt funct on PF2
    GPIO_PORTF_DEN_R |= 0x04;        // enable digital I/O on PF2
                                     // configure PF2 as GPIO
    GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R & 0xFFFFF0FF) + 0x00000000;
    GPIO_PORTF_AMSEL_R = 0;     // disable analog functionality on PF
    GPIO_PORTF_DATA_R &= ~0x04; // turn off LED
}

// **************Sound_Tone*********************
// Change Systick periodic interrupts to start sound output
// Input: interrupt period
//           Units of period are 12.5ns
//           Maximum is 2^24-1
//           Minimum is determined by length of ISR
// Output: none
void Sound_Tone(unsigned long period){
// this routine sets the RELOAD and starts SysTick
    NVIC_ST_CTRL_R = 0;                                            // disable SysTick during setup
    NVIC_ST_RELOAD_R = period - 1;                                 // reload value
    NVIC_ST_CURRENT_R = 0;                                         // any write to current clears it
    NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0x20000000; // priority 1
    NVIC_ST_CTRL_R = 0x0007;                                       // enable SysTick with core clock and interrupts
}


// **************Sound_Off*********************
// stop outputing to DAC
// Output: none
void Sound_Off(void){
 // this routine stops the sound output
    Index = 0;
    DAC_Out(0);
}


// Interrupt service routine
// Executed every 12.5ns*(period)
void SysTick_Handler(void){
    GPIO_PORTF_DATA_R ^= 0x04; // toggle PF2, debugging
    Index = (Index + 1) & 0x0F;
    DAC_Out(SineWave[Index]);
}
