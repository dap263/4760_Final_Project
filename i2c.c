/*
 * File:        
 * Author:      
 * For use with Sean Carroll's Big Board
 * http://people.ece.cornell.edu/land/courses/ece4760/PIC32/target_board.html
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config_1_2_3.h"
// threading library
#include "pt_cornell_1_2_3.h"
// yup, the expander
#include "port_expander_brl4.h"

////////////////////////////////////
// graphics libraries
// SPI channel 1 connections to TFT
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
// need for sin function
#include <math.h>

#include "plib.h"

////////////////////////////////////

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
//Important Addresses
#define KMX62_ADDR_W 0x1C
#define KMX62_ADDR_R 0x1D

#define ACCEL_XOUT_L 0x0A
#define ACCEL_XOUT_H 0x0B
#define ACCEL_YOUT_L 0x0C
#define ACCEL_YOUT_H 0x0D
#define ACCEL_ZOUT_L 0x0E
#define ACCEL_ZOUT_H 0x0F

#define MAG_XOUT_L 0x10
#define MAG_XOUT_H 0x11
#define MAG_YOUT_L 0x12
#define MAG_YOUT_H 0x13
#define MAG_ZOUT_L 0x14
#define MAG_ZOUT_H 0x15

#define CNTL2 0x3A

//0D?
#define CNTL2_DATA 0b00001111  //enable all sensors and set acceleration range to +-2g

//Important variables
 #define max_size 10
 int Accel_X_buf[max_size];
 int Accel_Y_buf[max_size];
 int Accel_Z_buf[max_size];
 int Mag_X_buf[max_size];
 int Mag_Y_buf[max_size];
 int Mag_Z_buf[max_size];

 int Accel_X_index;
 int Accel_Y_index;
 int Accel_Z_index;
 int Mag_X_index;
 int Mag_Y_index;
 int mag_Z_index;


// === print a line on TFT =====================================================
// print a line on the TFT
// string buffer
char buffer[60];
void printLine(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 10 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 8, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(1);
    tft_writeString(print_buffer);
}

void printLine2(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 20 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 16, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(2);
    tft_writeString(print_buffer);
}

void i2c_wait(unsigned int cnt){
    while(--cnt){
        asm("nop");
        asm("nop");
    }
}

char i2c_read(char target){
    char data;

    StartI2C1(); //Send Start Condition
    IdleI2C1(); 

    MasterWriteI2C1(KMX62_ADDR_W); //Send Device Address (Write)
    IdleI2C1(); 
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge

    MasterWriteI2C1(target); //Send Register address the Master wants to read
    IdleI2C1(); 
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge 

    RestartI2C1(); //Restart 
    i2c_wait(10);
    IdleI2C1(); 

    MasterWriteI2C1(KMX62_ADDR_R); //Send Device Address (Read)
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge

    data = MasterReadI2C1(); 
    IdleI2C1();  
    NotAckI2C1();
    
    StopI2C1();
    IdleI2C1();
    return data;
}

void i2c_write(char data,char target){
    StartI2C1(); //Send Start Condition
    IdleI2C1(); 

    MasterWriteI2C1(KMX62_ADDR_W); //Send Device Address (Write)
    IdleI2C1(); 
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge

    MasterWriteI2C1(target); //Send Register address the Master wants to write
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge 
    IdleI2C1(); 
   
    
    MasterWriteI2C1(data); //write data
    IdleI2C1();
    while (I2C1STATbits.ACKSTAT); //wait for slave acknowledge
    
    StopI2C1();
    IdleI2C1();
}

int getAccel_X(){
   int Accel_X= (int)(i2c_read(ACCEL_XOUT_H)<<8)+(i2c_read(ACCEL_XOUT_L));
   return Accel_X;
}

int getAccel_Y(){
   int Accel_Y= (int)(i2c_read(ACCEL_YOUT_H)<<8)+(i2c_read(ACCEL_YOUT_L));
   return Accel_Y;
}

int getAccel_Z(){
   int Accel_Z= (int)(i2c_read(ACCEL_ZOUT_H)<<8)+(i2c_read(ACCEL_ZOUT_L));
   return Accel_Z;
}

int getMag_X(){
   int Mag_X= (int)(i2c_read(MAG_XOUT_H)<<8)+(i2c_read(MAG_XOUT_L));
   return Mag_X;
}

int getMag_Y(){
   int Mag_Y= (int)(i2c_read(MAG_YOUT_H)<<8)+(i2c_read(MAG_YOUT_L));
   return Mag_Y;
}

int getMag_Z(){
   int Mag_Z= (int)(i2c_read(MAG_ZOUT_H)<<8)+(i2c_read(MAG_ZOUT_L));
   return Mag_Z;
}

int running_avg(int val, int ring_buffer[], int *ring_index){
    //scale values now to prevent overflow
    ring_buffer[*ring_index]=val/max_size;
    
    //Update index
    *ring_index=(*ring_index++)%max_size;
    
    //find the running avg
    int sum;
    int i;
    for (i=0; i<max_size; i++){
        sum=sum+ring_buffer[i];
    }        
    return sum;
}

// Predefined colors definitions (from tft_master.h)
//#define	ILI9340_BLACK   0x0000
//#define	ILI9340_BLUE    0x001F
//#define	ILI9340_RED     0xF800
//#define	ILI9340_GREEN   0x07E0
//#define ILI9340_CYAN    0x07FF
//#define ILI9340_MAGENTA 0xF81F
//#define ILI9340_YELLOW  0xFFE0
//#define ILI9340_WHITE   0xFFFF

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer ;
// The following threads are necessary for UART control
static struct pt pt_input, pt_output, pt_DMA_output ;

// system 1 second interval tick
int sys_time_seconds ;

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
  
    
    PT_BEGIN(pt);
     // set up LED to blink
     mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output
          mPORTASetBits(BIT_0 );	//Clear bits to ensure light is off.

      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(100) ;
        sys_time_seconds++ ;
        // toggle the LED on the big board
        mPORTAToggleBits(BIT_0);
        
        //I2C Test
        float Accel_X_avg = running_avg(getAccel_X(), Accel_X_buf, &Accel_X_index); 
        float Accel_Z_avg = running_avg(getAccel_Z(), Accel_Z_buf, &Accel_Z_index);
        float theta;
        
        //float num= Accel_Z_avg/Accel_X_avg;
        if (Accel_X_avg==0){
            theta=0;
        }
        else {
            theta = atan(Accel_Z_avg/Accel_X_avg);
        }
        theta=(theta*57.3)+90;
        // draw sys_time
        sprintf(buffer,"Time=%d", sys_time_seconds);
        sprintf(buffer, "Theta=%.1f", theta );
        printLine2(0, buffer, ILI9340_BLACK, ILI9340_YELLOW);
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
 //I2C Setup
    
  //variables for running av

  ANSELA = 0; ANSELB = 0; 
   
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_timer);
  
  //I2C
  OpenI2C1 (I2C_ON, 0x0C2);
  IdleI2C1();
  
  //Set up KMX62
  i2c_write (CNTL2_DATA, CNTL2);
  
  

  // init the display
  // NOTE that this init assumes SPI channel 1 connections
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240
  
  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      }
  } // main

// === end  ======================================================

