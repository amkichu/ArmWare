// Lab13.c
// Runs on LM4F120 or TM4C123
// Use SysTick interrupts to implement a 4-key digital piano
// edX Lab 13 
// Daniel Valvano, Jonathan Valvano
// December 29, 2014
// Port B bits 3-0 have the 4-bit DAC
// Port E bits 3-0 have 4 piano keys

#include "..//tm4c123gh6pm.h"
#include "Sound.h"
#include "Piano.h"
#include "TExaS.h"

// basic functions defined at end of startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void delay(unsigned long msec);
int main(void){ // Real Lab13 
unsigned long input;
// for the real board grader to work 
// you must connect PD3 to your DAC output
  TExaS_Init(SW_PIN_PE3210, DAC_PIN_PB3210,ScopeOn); // activate grader and set system clock to 80 MHz
// PortE used for piano keys, PortB used for DAC
  Sound_Init(); // initialize SysTick timer and DAC
  Piano_Init();
  EnableInterrupts();  // enable after all initialization are done
  while(1){
// need to generate a G 784 Hz sine wave
// table size is 32, so need 784Hz*32 = 25.088 kHz interrupt
// bus is 80MHz, so SysTick period is 80000kHz/25.088kHz = 3188.7755 = 3189

// // input from keys to select tone
//     input = Piano_In();
//          if(input=0x01) Sound_Tone(2389);    // Key 0: C
//     else if(input=0x02) Sound_Tone(4257);    // Key 1: D
//     else if(input=0x04) Sound_Tone(3792);    // Key 2: E
//     else if(input=0x08) Sound_Tone(3189);    // Key 3: G
//     input = 0;
// static debugging
    unsigned long i;
    for (i = 0; i < 16; i++){
        DAC_Out(i);
        delay(10); // connect PD3 to DAC output
    }
  }
}

// Inputs: Number of msec to delay
// Outputs: None
void delay(unsigned long msec){ 
  unsigned long count;
  while(msec > 0 ) {  // repeat while there are still delay
    count = 16000;    // about 1ms
    while (count > 0) { 
      count--;
    } // This while loop takes approximately 3 cycles
    msec--;
  }
}
