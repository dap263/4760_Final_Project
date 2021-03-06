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

#define CNTL2_DATA 0b00001111  //enable all sensors and set acceleration range to +-2g

//Important variables
 #define max_size 600

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
 int Mag_Z_index;
 
 //CALIBRATING OFFSETS
 volatile float Mag_X_offset=0;//-37.7;
 volatile float Mag_Y_offset=0;//833.1;
 volatile float Mag_Z_offset=0;//2000;
 
 //CALCULATE PITCH, ROLL, AND YAW
 float theta; //PITCH
 float phi;   //ROLL
 float psi;   //YAW
 float phi_deg;
 float theta_deg;
 float psi_deg;
 
 //FILTER RAW VALUES (DIGITAL LOWPASS)
 float Accel_X_avg;
 float Accel_Y_avg;
 float Accel_Z_avg;
 float Mag_X_avg;
 float Mag_Y_avg;
 float Mag_Z_avg;
 float beta = .2;


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

      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(100) ;
        
        
        
        Accel_X_avg = Accel_X_avg - (beta*(Accel_X_avg-getAccel_X()));
        Accel_Z_avg = Accel_Z_avg - (beta*(Accel_Z_avg-getAccel_Z()));
        Accel_Y_avg = Accel_Y_avg - (beta*(Accel_Y_avg-getAccel_Y()));
        Mag_X_avg = Mag_X_avg - (beta*(Mag_X_avg-getMag_X()));
        Mag_Y_avg = Mag_Y_avg - (beta*(Mag_Y_avg-getMag_Y()));
        Mag_Z_avg = Mag_Z_avg - (beta*(Mag_Z_avg-getMag_Z()));
       
        
        
        //ROLL
        phi = atan2(Accel_Y_avg, Accel_Z_avg);
        phi_deg = phi*57.3;
        
        //PITCH
        theta = atan2(-Accel_X_avg,Accel_Y_avg*sin(phi)+Accel_Z_avg*cos(phi));
        theta_deg = theta*57.3;
        
        //YAW
        float bottom = ((Mag_Z_avg-Mag_Z_offset)*sin(theta))+((Mag_X_avg-Mag_X_offset)*cos(theta));
        float top = ((Mag_Y_avg-Mag_Y_offset)*cos(phi)-(Mag_Z_avg-Mag_Z_offset)*sin(phi)*cos(theta));

        psi=atan2( top,bottom );
        psi_deg=psi*57.3;
        if (psi_deg<0){psi_deg+=360;}
    
  ////////////////////////////////////////////////////////////////////////////////////////////////////
        
        printLine2(0, buffer, ILI9340_BLACK, ILI9340_YELLOW);
        sprintf(buffer, "heading=%.1f", psi_deg);
        printLine2(1, buffer, ILI9340_BLACK, ILI9340_YELLOW);
        sprintf(buffer, "theta=%.1f", theta_deg);
        printLine2(2, buffer, ILI9340_BLACK, ILI9340_YELLOW);
        sprintf(buffer, "Z offset=%.1f", Mag_Z_offset);
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === Main  ======================================================
void main(void) {
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
  
  //CALCULATE MAGNETOMETER OFFSET

  int i;
  for (i=0; i<5000; i++){
     
      Mag_X_avg = Mag_X_avg - (beta*(Mag_X_avg-getMag_X()));
      Mag_Y_avg = Mag_Y_avg - (beta*(Mag_Y_avg-getMag_Y()));
      Mag_Z_avg = Mag_Z_avg - (beta*(Mag_Z_avg-getMag_Z()));
      
      Mag_X_offset+=(float)(Mag_X_avg/5000);
      Mag_Y_offset+=(float)(Mag_Y_avg/5000);
      Mag_Z_offset+=(float)(Mag_Z_avg/5000);
  }


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





