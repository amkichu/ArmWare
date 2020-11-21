// MeasurementOfDistance.c
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to periodically initiate a software-
// triggered ADC conversion, convert the sample to a fixed-
// point decimal distance, and store the result in a mailbox.
// The foreground thread takes the result from the mailbox,
// converts the result to a string, and prints it to the
// Nokia5110 LCD.  The display is optional.
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

// Slide pot pin 3 connected to +3.3V
// Slide pot pin 2 connected to PE2(Ain1) and PD3
// Slide pot pin 1 connected to ground


#include "ADC.h"
#include "..//tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "TExaS.h"

#define SYSDIV2 4

void EnableInterrupts(void);  // Enable interrupts

unsigned char String[10]; // null-terminated ASCII string
unsigned long Distance;   // units 0.001 cm
unsigned long ADCdata;    // 12-bit 0 to 4095 sample
unsigned long Flag;       // 1 means valid Distance, 0 means Distance is empty

//********Convert****************
// Convert a 12-bit binary ADC sample into a 32-bit unsigned
// fixed-point distance (resolution 0.001 cm). Calibration
// data is gathered using known distances and reading the
// ADC value measured on PE1.
// Overflow and dropout should be considered
// Input: sample  12-bit ADC sample
// Output: 32-bit distance (resolution 0.001cm)
// Distance = ((A*ADCdata)>>10)+B     where A and B are calibration constants
// Distance = 0.4682*ADCdata + 112.31
// ADCdata -> Distance
// 0 -> 0
// 1023 -> 500
// 2948 -> 1439
unsigned long Convert(unsigned long sample){
  Distance = (unsigned long)((0.5 * sample) + 0);
  return Distance;
}

// Initialize SysTick interrupts to trigger at 40 Hz, 25 ms
// need to generate a 40 Hz interrupt and bus is 80MHz,
// so SysTick period is 80000000Hz/40Hz = 2000000
void SysTick_Init(unsigned long period){
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
// this routine sets the RELOAD and starts SysTick
    NVIC_ST_CTRL_R = 0;                                            // disable SysTick during setup
    NVIC_ST_RELOAD_R = period - 1;                                 // reload value
    NVIC_ST_CURRENT_R = 0;                                         // any write to current clears it
    NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0x20000000; // priority 1
    NVIC_ST_CTRL_R = 0x0007;                                       // enable SysTick with core clock and interrupts
}

void Gpio_Init()
{
  // For heartbeat in PF1
  SYSCTL_RCGCGPIO_R |= 0x00000020; // activate port F
  GPIO_PORTF_DIR_R |= 0x02;        // make PF1 out (built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x02;     // disable alt funct on PF1
  GPIO_PORTF_DEN_R |= 0x02;        // enable digital I/O on PF1
                                   // configure PF1 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R & 0xFFFFF0FF) + 0x00000000;
  GPIO_PORTF_AMSEL_R = 0;     // disable analog functionality on PF
  GPIO_PORTF_DATA_R &= ~0x02; // turn off LED
}
// executes every 25 ms, collects a sample, converts and stores in mailbox
void SysTick_Handler(void)
  {
    GPIO_PORTF_DATA_R ^= 0x02; // toggle PF1 first time, debugging
    GPIO_PORTF_DATA_R ^= 0x02; // toggle PF1 second time
    ADCdata = ADC0_In();
    Distance = Convert(ADCdata);
    Flag = 1;
    GPIO_PORTF_DATA_R ^= 0x02; // toggle PF1 third time
}

//-----------------------UART_ConvertDistance-----------------------
// Converts a 32-bit distance into an ASCII string
// Input: 32-bit number to be converted (resolution 0.001cm)
// Output: store the conversion in global variable String[10]
// Fixed format 1 digit, point, 3 digits, space, units, null termination
// Examples
//    4 to "0.004 cm"  
//   31 to "0.031 cm" 
//  102 to "0.102 cm" 
// 2210 to "2.210 cm"
//10000 to "*.*** cm"  any value larger than 9999 converted to "*.*** cm"
void UART_ConvertDistance(unsigned long n){
    // as part of Lab 11 you implemented this function
    unsigned char i = 0;
    for (i = 0; i < 10; i++)
    {
        String[i] = ' ';
    }
    if (n < 10000)
    {
        String[0] = n / 1000 + '0'; /* thousands digit */
        n = n % 1000;
        String[1] = '.';           /* Decimal point */
        String[2] = n / 100 + '0'; /* tenths digit */
        n = n % 100;
        String[3] = n / 10 + '0'; /* hundredths digit */
        String[4] = n % 10 + '0'; /* thousandth digit */
    }
    else
    {
        String[0] = '*';
        String[1] = '.';
        String[2] = '*';
        String[3] = '*';
        String[4] = '*';
    }
    String[5] = ' ';
    String[6] = 'c';
    String[7] = 'm';
}

// main1 is a simple main program allowing you to debug the ADC interface
// int main1(void){ 
// int main(void){ 
//   TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
//   ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
//   EnableInterrupts();
//   while(1){ 
//     ADCdata = ADC0_In();
//   }
// }
// once the ADC is operational, you can use main2 to debug the convert to distance
// int main2(void){ 
// int main(void){ 
//   TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_NoScope);
//   ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
//   Nokia5110_Init();             // initialize Nokia5110 LCD
//   EnableInterrupts();
//   while(1){ 
//     ADCdata = ADC0_In();
//     Nokia5110_SetCursor(0, 0);
//     Distance = Convert(ADCdata);
//     UART_ConvertDistance(Distance); // from Lab 11
//     Nokia5110_OutString(String);    // output to Nokia5110 LCD (optional)
//   }
// }
// // once the ADC and convert to distance functions are operational,
// // you should use this main to build the final solution with interrupts and mailbox
int main(void){ 
    volatile unsigned long delay;
    TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
// initialize ADC0, channel 1, sequencer 3
    ADC0_Init();
    Nokia5110_Init(); // initialize Nokia5110 LCD (optional)
    SysTick_Init(2000000);// initialize SysTick for 40 Hz interrupts and
    Gpio_Init(); // initialize profiling on PF1 (optional)
    //    wait for clock to stabilize
    EnableInterrupts();
// print a welcome message  (optional)
  while(1){ 
// read mailbox
// output to Nokia5110 LCD (optional)
    Flag = 0;
    while (Flag == 0);
    Nokia5110_SetCursor(0, 0);
    Distance = Convert(ADCdata);
    UART_ConvertDistance(Distance); // from Lab 11
    Nokia5110_OutString(String);
  }
}
