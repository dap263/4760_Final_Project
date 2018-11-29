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
#include "copied_uart.h"


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

// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
// B-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000

#define EnablePullUpA(bits) CNPDACLR=bits; CNPUASET=bits;

#define RAD2DEG 57.295779513082320876798154814105
#define DEG2RAD 0.01745329251994329576923690768489
#define RAD2HOURANGLE 3.8197186342054880584532103209403

// Pulldown macro
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;

// Debouncing FSM variables
volatile enum FSM_state {released, maybe_pushed, pushed, maybe_released} state = released; // FSM state
volatile int possible; // Possible key press

// string buffer
char buffer[60];

// Altitude and Azimuth data from Accelerometer
int acc_alt;
int acc_az;

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_gps, pt_input, pt_WiFi, pt_input2, pt_button;

// system 1 second interval tick
volatile int sys_time_seconds ;

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
    tft_fillRoundRect(0, v_pos, 319, 16, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(2);
    tft_writeString(print_buffer);
}

// Floating point modulus: a mod b
// Assumes b > 0
float float_mod(float a, int b) {
    while (a > b) {
        a -= (float) b;
    }
    while (a < 0) {
        a += (float) b;
    }
    return a;
}


// Convert Altitude/Azimuth to Right Ascension/Declination
// Formulas from comments by users Surveyor 1 and Ranger 4 on the web page:
// https://www.cloudynights.com/topic/448682-help-w-conversion-of-altaz-to-radec-for-dsc/

float RA; // Right Ascension
float DEC; // Declination

void AltAz2RaDec(int alt, int az, float lat, float lon, int hours, int min, int sec, int month, int day, int year) {
    
    // Calculating Julian Day
    int A = year / 100;
    int B = 2 - A + (int) (A/4);
    int JD = (int) (365.25 * (year + 4716)) + (int) (30.6001 * (month + 1) + day + B - 1524.5);
    
    // Universal Time in hours
    float UT = hours + ((float) min / 60) + ((float) sec / 3600); 
    
    // Days since Jan 1, 2000 at 1200 UT
    // Split into 2 variables for greater precision
    int epoch_days_whole = JD - 2451545;
    float epoch_days_part = UT/24 - 0.5;
    
    float T = (float) epoch_days_whole/36525 + epoch_days_part/36525;
    
    // Siderial Time at Greenwich in degrees
    // Split into 2 variables for greater precision then merged
    float q0_whole = float_mod(280 + 360 * epoch_days_whole, 360); 
    float q0_part = float_mod(0.46061837 + 0.98564736629 * (float) epoch_days_whole + 360.98564736629 * epoch_days_part + (0.000387933 * T * T) - ((T * T * T)/38710000),360);
    float q0 = float_mod(q0_whole + q0_part, 360);
    
    // Local Siderial Time in degrees
    float q = float_mod(q0 + lon, 360); 
    
    // Declination in degrees
    DEC = asin(sin((float)alt*DEG2RAD)*sin(lat*DEG2RAD) + cos((float)alt*DEG2RAD)*cos(lat*DEG2RAD)*cos((float)az*DEG2RAD)) * RAD2DEG;
    
    // Hour angle in hours
    float H = acos((sin((float)alt*DEG2RAD) - sin((float)lat*DEG2RAD)*sin(DEC*DEG2RAD))/(cos(lat*DEG2RAD)*cos(DEC*DEG2RAD))) * RAD2HOURANGLE;
    
    // Right ascension in hours
    RA = q/15 - H;
    
}


static PT_THREAD (protothread_GPS(struct pt *pt))
{
  PT_BEGIN(pt);
  while (1) {
    PT_SPAWN(pt, &pt_input, PT_GetSerialBufferGPS(&pt_input));
//    printLine(1, PT_term_buffer_GPS_RMC, ILI9340_WHITE, ILI9340_BLACK);
    // if received sentence is in GPRMC format
    if (GPRMC == 1) {
        GPRMC = 0; //reset GPRMC flag
        parse_RMC(PT_term_buffer_GPS_RMC); //parse the GPRMC sentence
        if (GPS_valid) {
            sprintf(buffer, "Date: %d/%d/%d", GPS_month, GPS_day, GPS_year);
            printLine(1, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "Time: %d:%d:%d", GPS_time_h, GPS_time_m, GPS_time_s);
            printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
            AltAz2RaDec(acc_alt, acc_az, GPS_Lat, GPS_Lon, GPS_time_h, GPS_time_m, GPS_time_s, GPS_month, GPS_day, GPS_year);
            sprintf(buffer, "RA: %.3f", RA);
            printLine(3, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "DEC: %.3f", DEC);
            printLine(4, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "Lat: %.6f", GPS_Lat);
            printLine(5, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "Lon: %.6f", GPS_Lon);
            printLine(6, buffer, ILI9340_WHITE, ILI9340_BLACK);
        } else {
            printLine(10, "Not Valid", ILI9340_WHITE, ILI9340_BLACK);
        }
    } //end if
  }  
  PT_END(pt);
} // GPS thread

static PT_THREAD (protothread_WiFi(struct pt *pt))
{
  PT_BEGIN(pt);
  int counter;
  
  while (1) {
    
    counter++;
    sprintf(buffer, "About to start %d", counter);
    printLine(0, buffer, ILI9340_WHITE, ILI9340_BLACK);
    sprintf(buffer, "%d", num_char);
    printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
    //DmaChnEnable(0);
    printLine(4, WiFi_Buffer, ILI9340_WHITE, ILI9340_BLACK);
    PT_SPAWN(pt, &pt_input2, PT_GetMachineBuffer(&pt_input2));
    if (counter==40) {
        
    }
    
    //PT_YIELD_TIME_msec(10000);
  }  
  PT_END(pt);
} // GPS thread

static PT_THREAD (protothread_button(struct pt *pt)) {
    
    PT_BEGIN(pt);
    
    // PortY as inputs
    // note that bit 7 will be shift key input, 
    // separate from keypad
    
    // shouldn't this be done in main??
    mPORTBSetPinsDigitalIn(BIT_4);    //Set port as input
    EnablePullDownB(BIT_4);
    
    while(1) {
      // yield time
      PT_YIELD_TIME_msec(30);
      short button = mPORTBReadBits(BIT_4);
      // Debouncing FSM
      switch (state) {
          case released :
              // Button released
              if (button) {
                  state = maybe_pushed;
              }
              break;
          case maybe_pushed :
              // Potential button press
              if (button) {
                  state = pushed;
                  AltAz2RaDec(acc_alt, acc_az, GPS_Lat, GPS_Lon, GPS_time_h, GPS_time_m, GPS_time_s, GPS_month, GPS_day, GPS_year); // Get RA and DEC currently viewed
                  
              } else {
                  state = released;
              }
              break;
          case pushed :
              // Confirmed button press - button still held
              if (!button) {
                  state = released;
              }
              break;
          case maybe_released :
              // Potential button release/change
              if (button) {
                  state = pushed;
              } else {
                  state = released;
              }
              break;
        } // End switch
        // NEVER exit while
    } // END WHILE(1)
    PT_END(pt);
} // keypad thread

// === Main  ======================================================
void main(void) {
  
  // === config threads ==========
    
    ANSELA = 0; ANSELB = 0;
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
//  PT_INIT(&pt_gps);
  PT_INIT(&pt_WiFi);
//  PT_INIT(&pt_button);

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(1); // Use tft_setRotation(1) for 320x240
    
  int i;
  for (i = 0; i < 10000; i++) {
      if (i % 10 < 5){
          //WiFi_Buffer[i] = 0 | DAC_config_chan_A;
      }
      else {
          //WiFi_Buffer[i] = 4095 | DAC_config_chan_A;
      }
  }
  
  // DAC and DMA setup
  PPSOutput(2, RPB5, SDO2);
  PPSOutput(4, RPB10, SS2);
  OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 1814); // Timer2 @ 22.051 kHz
                                      // Interrupt flag, no ISR
  SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV | SPICON_FRMEN | SPICON_FRMPOL, 2);
  // Initializes SPI in framed mode
  DmaChnOpen(0,0,DMA_OPEN_AUTO); // Auto mode to repeatedly send data
  DmaChnSetTxfer(0, (void*) & WiFi_Buffer, (void*) & SPI2BUF, 16, 2, 2);
      // Transfer from DAC_data1 table to SPI2BUF, 256 bytes total, 2 at a time
  DmaChnSetEventControl(0, DMA_EV_START_IRQ(_TIMER_2_IRQ)); // Timer2 interrupt triggers DMA burst
  DmaChnEnable(0);  
  // round-robin scheduler for threads
  while (1){
//      PT_SCHEDULE(protothread_GPS(&pt_gps));
      PT_SCHEDULE(protothread_WiFi(&pt_WiFi));
//      PT_SCHEDULE(protothread_button(&pt_button));
      }
  } // main

// === end  ======================================================