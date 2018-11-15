/*
 * File:        TFT_test_BRL4.c
 * Author:      Bruce Land
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config.h"
#include "config_1_2_3.h"
// threading library
#include "pt_cornell_1_2_3.h"
// need for sin function
#include <math.h>

////////////////////////////////////
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
////////////////////////////////////

//#include "UART.h"


/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#define use_uart_serial

// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
// B-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000

#define EnablePullUpA(bits) CNPDACLR=bits; CNPUASET=bits;


// string buffer
char buffer[60];

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_calc, pt_timer, pt_adjust ;

// system 1 second interval tick
volatile int sys_time_seconds ;
volatile int generate_period = 40000;
volatile int pwm_on_time = 0;
volatile int adc; // motor pot
volatile int adc1;
static const int horizontal = 287; // adc value at horizontal
volatile int P = 125;
static const int P_max = 400;
_Accum I = 0.06;
volatile int error_I = 0;
static const _Accum I_max = 1;
volatile int D = 23500;
static const int D_max = 40000;
volatile int target = 0;
volatile int target_angle = 0;
volatile int pid;
volatile int error[20];
volatile int error_ptr = 0;
volatile int motor_disp = 0;
volatile int adc_disp = 0;
volatile int angle = 0;
static const int target_max = 180;
volatile int manual_target = 0;

#define adc2angle (adc-horizontal)/3
#define angle2adc (angle*3)+287

// === print a line on TFT =====================================================
// print a line on the TFT
// string buffer
char buffer[60];
void printLine(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 20 ;
    // erase the pixels
    tft_fillRoundRect(40, v_pos, 319, 16, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(40, v_pos);
    tft_setTextSize(2);
    tft_writeString(print_buffer);
}

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    
    mT2ClearIntFlag();
    adc = ReadADC10(1);
    error[error_ptr] = target - adc;
    //AcquireADC10();
    error_I += (error[error_ptr]+ error[(error_ptr - 1) %20]) >> 1;
    pid = I * error_I;
    pid = pid + ((D*(error[error_ptr]-error[(error_ptr - 3) % 20])) >> 2);
    pid = P*(error[error_ptr]) + pid;
    if (pid > 40000)
        pid = 40000;
    else if (pid < 0){
        pid = 0;
        error_I = .997*error_I;
    }
    pwm_on_time = pid;
    SetDCOC3PWM(pwm_on_time);
    error_ptr = (error_ptr + 1) % 20;
}
    
// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
     // set up LED to blink
     //mPORTASetBits(BIT_0 );	//Clear bits to ensure light is off.
     //mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        // toggle the LED on the big board
        //mPORTAToggleBits(BIT_0);
        // draw sys_time
        
        sprintf(buffer,"%d", sys_time_seconds);
        printLine(0, buffer, ILI9340_WHITE, ILI9340_BLACK);
        if (!manual_target) {
            if (sys_time_seconds == 5){
                angle = 30;
                target = angle2adc;
            }
            else if (sys_time_seconds == 10){
                angle = -30;
                target = angle2adc;
            }
            else if (sys_time_seconds == 15)
                target = horizontal;
            else if (sys_time_seconds == 20){
                target = 0;
                sys_time_seconds = 0;
                PT_YIELD_TIME_msec(5000);
                target = horizontal;
                //error_I = 0;
            }  
        }
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

static PT_THREAD (protothread_calc(struct pt *pt))
{
  PT_BEGIN(pt);
  while (1) {
    PT_GetMachineBuffer2(pt);
    PT_YIELD_TIME_msec(1000);
//    sscanf(PT_term_buffer2, "%x");
    printLine(2, PT_term_buffer2, ILI9340_WHITE, ILI9340_BLACK);
//    PT_YIELD_TIME_msec(1);
  }  
  PT_END(pt);
} // timer thread

static PT_THREAD (protothread_adjust(struct pt *pt))
{
    PT_BEGIN(pt);
    while (1) {
        // P TERM
        PT_YIELD_TIME_msec(30);
        PT_YIELD_UNTIL(&pt_adjust, !mPORTAReadBits(BIT_4)); // wait for right button push
        //AcquireADC10();
        PT_YIELD_TIME_msec(200);
        sprintf(buffer,"Changing the P term to: ");
        printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
        int P_temp = P;
        adc1 = ReadADC10(0);
        //AcquireADC10();
        while(mPORTAReadBits(BIT_4)){ // while left button not pushed
            adc1 = ReadADC10(0);
            P_temp = adc1*P_max>>10;
            sprintf(buffer,"%d", P_temp);
            printLine(3, buffer, ILI9340_WHITE, ILI9340_BLACK);
            if (!mPORTAReadBits(BIT_3)) {
                P = P_temp;
                sprintf(buffer,"P term set to %d", P);
                printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
                PT_YIELD_TIME_msec(200);
            }
            PT_YIELD_TIME_msec(200);
            // adjust P term
        }
        // D TERM
        PT_YIELD_UNTIL(&pt_adjust, !mPORTAReadBits(BIT_4)); // wait for right button push
        //AcquireADC10();
        PT_YIELD_TIME_msec(200);
        sprintf(buffer,"Changing the D term to: ");
        int D_temp = D;
        printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
        //AcquireADC10();
        while(mPORTAReadBits(BIT_4)){ // while left button not pushed
            adc1 = ReadADC10(0);
            D_temp = adc1*D_max>>10;
            sprintf(buffer,"%d", D_temp);
            printLine(3, buffer, ILI9340_WHITE, ILI9340_BLACK);
            if (!mPORTAReadBits(BIT_3)) {
                D = D_temp;
                sprintf(buffer,"D term set to %d", D);
                printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
                PT_YIELD_TIME_msec(200);
            }
            PT_YIELD_TIME_msec(200);
            // adjust P term
        }
        // I TERM
        PT_YIELD_UNTIL(&pt_adjust, !mPORTAReadBits(BIT_4)); // wait for right button push
        //AcquireADC10();
        PT_YIELD_TIME_msec(200);
        sprintf(buffer,"Changing the I term to: ");
        _Accum I_temp = I;
        printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
        //AcquireADC10();
        while(mPORTAReadBits(BIT_4)){ // while left button not pushed
            adc1 = ReadADC10(0);
            I_temp = (_Accum)(((_Accum)adc1)>>10);
            sprintf(buffer,"%5.4f", (float) I_temp);
            printLine(3, buffer, ILI9340_WHITE, ILI9340_BLACK);
            if (!mPORTAReadBits(BIT_3)) {
                I = I_temp;
                sprintf(buffer,"I term set to %5.4f", (float) I);
                printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
                PT_YIELD_TIME_msec(200);
            }
            PT_YIELD_TIME_msec(200);
            // adjust P term
        }
        // TARGET ANGLE
        PT_YIELD_UNTIL(&pt_adjust, !mPORTAReadBits(BIT_4)); // wait for right button push
        //AcquireADC10();
        PT_YIELD_TIME_msec(200);
        sprintf(buffer,"Changing the angle to: ");
        int target_temp = target;
        printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
        //AcquireADC10();
        while(mPORTAReadBits(BIT_4)){ // while left button not pushed
            adc1 = ReadADC10(0);
            target_temp = (adc1*target_max>>10) - 90;
            sprintf(buffer,"%d", target_temp);
            printLine(3, buffer, ILI9340_WHITE, ILI9340_BLACK);
            if (!mPORTAReadBits(BIT_3)) {
                angle = target_temp;
                target = angle2adc;
                if (target <= 20) {
                    manual_target = 0;
                    sys_time_seconds = 0;
                } else {
                    manual_target = 1;
                }
                sprintf(buffer,"Angle set to %d", target);
                printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
                PT_YIELD_TIME_msec(200);
            }
            PT_YIELD_TIME_msec(200);
            // adjust P term
        }
    }
    PT_END(pt);
}

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);

  // === config threads ==========
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_calc);
  PT_INIT(&pt_timer);
  PT_INIT(&pt_adjust);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(1); // Use tft_setRotation(1) for 320x240
    
  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_calc(&pt_calc));
      }
  } // main

// === end  ======================================================