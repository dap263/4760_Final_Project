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
//m#include "config_1_2_3.h"
// threading library
#include "pt_cornell_1_2_3.h"
// need for sin function
#include <math.h>

////////////////////////////////////
// graphics libraries
//#include "tft_master.h"
//#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
////////////////////////////////////

//#include "UART.h"
#include "GPS.h"

//#include "Error_messages.h"

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
// from TFT library for delay_ms
#define PBCLK 40000000 // peripheral bus clock
#define dTime_ms PBCLK/2000

// --------- START I2C Stuff ----------------------
 //Important  I2C Addresses
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
 
 //CALIBRATING OFFSETS
 volatile float Mag_X_offset=0;//-37.7;
 volatile float Mag_Y_offset=0;//833.1;
 volatile float Mag_Z_offset=0;//2000;
 
 //CALCULATE PITCH, ROLL, AND YAW
 float theta; //PITCH
 float theta_correct; // theta is inverted, this one is neg theta
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
 
// ----------------- END I2C STUFf ----------------
 
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
volatile short possible; // Possible key press
volatile short prior_fix; // boolean for GPS fix to talk

// string buffer
char buffer[60];

// Altitude and Azimuth data from Accelerometer
int acc_alt;
int acc_az;

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_gps, pt_input, pt_WiFi, pt_input2, pt_button, pt_accel;

// system 1 second interval tick
volatile int sys_time_seconds ;

 //UNCOMMENT ONCE WE REMOVE TFT LIBRARY
void delay_ms(unsigned long i){
/* Create a software delay about i ms long
 * Parameters:
 *      i:  equal to number of milliseconds for delay
 * Returns: Nothing
 * Note: Uses Core Timer. Core Timer is cleared at the initialiazion of
 *      this function. So, applications sensitive to the Core Timer are going
 *      to be affected
 */
    unsigned int j;
    j = dTime_ms * i;
    WriteCoreTimer(0);
    while (ReadCoreTimer() < j);
}

// === print a line on TFT =====================================================
// print a line on the TFT
// string buffer
/*
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
*/
// ----------------- I2C Functions ----------------

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

// ----------------- End I2C Functions -------------

void ESP_setup (void) {
    
    printf("\r\n");
    printf("function cb_disconnected()\r\n");
//    delay_ms(100);
//    printf("print(\"wassup dog\")\r\n");
    delay_ms(100);
    printf("print(\"\\a\")\r\n");
    delay_ms(100);
    printf("end\r\n");
    delay_ms(100);
    printf("function cb_connected(sck, c)\r\n");
    delay_ms(100);
    printf("print(c)\r\n");
    delay_ms(100);
    printf("if (i == 0) then\r\n");
    delay_ms(100);
    printf("print(\"\\b\")\r\n");
    delay_ms(100);
    printf("end\r\n");
    delay_ms(100);
    printf("i = i + 1\r\n");
    delay_ms(100);
    printf("end\r\n");
    //printf("uart.setup(0, 256000, 8, uart.PARITY_NONE, uart.STOPBITS_1, 1)\r\n");
    
}

// first part of message needs to be 'tts: ' or 'ra: '

void ESP_request_data(char *message) {
    printf("i = 0\r\n");
    delay_ms(10);
    printf("srv=net.createConnection(net.TCP,0)\r\n");
    delay_ms(10);
    printf("srv:on(\"receive\", cb_connected)\r\n");
    delay_ms(10);
    printf("srv:on(\"disconnection\", cb_disconnected)\r\n");
    delay_ms(10);
    printf("srv:on(\"connection\",function(sck,c)\r\n");
    delay_ms(10);
    printf("sck:send(\"GET %s\\r\\n HTTP /1.1\\r\\nHost: 192.168.43.1\\r\\nConnection: close\\r\\nAccept: */*\\r\\n\\r\\n\")\r\n", message);
    delay_ms(10);
    printf("end)\r\n");
    delay_ms(10);
    printf("srv:connect(5000,\"192.168.43.14\")\r\n");
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

// ======== Accelerometer thread =========
static PT_THREAD (protothread_accel(struct pt *pt))
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
        theta_correct = -theta_deg;
        //YAW
        float bottom = ((Mag_Z_avg-Mag_Z_offset)*sin(theta))+((Mag_X_avg-Mag_X_offset)*cos(theta));
        float top = ((Mag_Y_avg-Mag_Y_offset)*cos(phi)-(Mag_Z_avg-Mag_Z_offset)*sin(phi)*cos(theta));

        psi=atan2( top,bottom );
        psi_deg=psi*57.3;
        if (psi_deg<0){psi_deg+=360;}
    
        /*
        printLine2(0, buffer, ILI9340_BLACK, ILI9340_YELLOW);
        sprintf(buffer, "heading=%.1f", psi_deg);
        printLine2(1, buffer, ILI9340_BLACK, ILI9340_YELLOW);
        sprintf(buffer, "theta=%.1f", theta_deg);
        printLine2(2, buffer, ILI9340_BLACK, ILI9340_YELLOW);
        sprintf(buffer, "Z offset=%.1f", Mag_Z_offset);
        */
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // accelerometer thread

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
        if (GPS_fix) {
            if (!prior_fix){
                // say GPS got a fix
//                DmaChnSetTxfer(2, (void*) & GPS_got_fix, (void*) & SPI2BUF, sizeof(GPS_got_fix), 2, 2);
                prior_fix = 1;
            }
            AltAz2RaDec(acc_alt, acc_az, GPS_Lat, GPS_Lon, GPS_time_h, GPS_time_m, GPS_time_s, GPS_month, GPS_day, GPS_year);
			/*
			sprintf(buffer, "Date: %d/%d/%d", GPS_month, GPS_day, GPS_year);
            printLine(1, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "Time: %d:%d:%d", GPS_time_h, GPS_time_m, GPS_time_s);
            printLine(2, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "RA: %.3f", RA);
            printLine(3, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "DEC: %.3f", DEC);
            printLine(4, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "Lat: %.6f", GPS_Lat);
            printLine(5, buffer, ILI9340_WHITE, ILI9340_BLACK);
            sprintf(buffer, "Lon: %.6f", GPS_Lon);
            printLine(6, buffer, ILI9340_WHITE, ILI9340_BLACK);
			*/
        } else {
            if (prior_fix){
                // say GPS lost fix
//                DmaChnSetTxfer(2, (void*) & GPS_error, (void*) & SPI2BUF, sizeof(GPS_error), 2, 2);
                prior_fix = 0;
            }
        }
    } //end if
  }  
  PT_END(pt);
} // GPS thread

static PT_THREAD (protothread_WiFi(struct pt *pt))
{
  PT_BEGIN(pt);
  // send commands to connect to server and receive speech
  //ESP_setup();
  while (1) {
    PT_SPAWN(pt, &pt_input2, PT_GetMachineBuffer(&pt_input2));
    
    
    PT_YIELD_TIME_msec(1000);
    DmaChnEnable(0);
  }  
  PT_END(pt);
} // GPS thread

static PT_THREAD (protothread_button(struct pt *pt)) {
    
    PT_BEGIN(pt);
    
    // PortY as inputs
    // note that bit 7 will be shift key input, 
    // separate from keypad
    
    // shouldn't this be done in main??
    mPORTBSetPinsDigitalIn(BIT_13);    //Set port as input
    EnablePullDownB(BIT_13);
    //DmaChnEnable(0);
    while(1) {
      // yield time
      PT_YIELD_TIME_msec(30);
      short button = mPORTBReadBits(BIT_13);
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
                  if (GPS_fix) {
//                    AltAz2RaDec(acc_alt, acc_az, GPS_Lat, GPS_Lon, GPS_time_h, GPS_time_m, GPS_time_s, GPS_month, GPS_day, GPS_year); // Get RA and DEC currently viewed
                  }
                  else {
//                      DmaChnSetTxfer(2, (void*) & GPS_error, (void*) & SPI2BUF, sizeof(GPS_error), 2, 2);
                      // no GPS fix, speak
                  }
              } else {
                  state = released;
              }
              break;
          case pushed :
              // Confirmed button press - button still held
              //ESP_request_data("tts: alec sucks");
              //PT_SPAWN(pt, &pt_input2, PT_GetMachineBuffer(&pt_input2));
              /*if (!GPS_fix) {
                  DmaChnSetTxfer(2, (void*) & GPS_error, (void*) & SPI2BUF, sizeof(GPS_error), 2, 2);
                  DmaChnEnable(2);
                  // no GPS fix, speak
              } else*/ if (!button) {
//                  printf("sending request\r\n");
                    printf("\r\n");
                    delay_ms(10);
                        delay_ms(10);
                        //sprintf(buffer, "ra: %f, %f", RA, DEC);
                        //PT_SPAWN(pt, &pt_input2, PT_GetMachineBuffer(&pt_input2));
                        ESP_request_data("tts: hello, fuck this shit");
                        
                  //ESP_request_data("tts: you're looking at andromeda, bitch!");
//                    }
                        //DmaChnEnable(0);
                  //ESP_request_data("tts: you're looking at andromeda, bitch!");
                  state = released;
              }
              if (!button) 
                  state = released;
              break;
          case maybe_released :
              // Potential button release/change
              if (button) {
                  state = pushed;
              } else {
                 // DmaChnEnable(0);
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
  PT_INIT(&pt_gps);
  //PT_INIT(&pt_WiFi);
  PT_INIT(&pt_button);
  PT_INIT(&pt_accel);
  
  //I2C
  //OpenI2C1 (I2C_ON, 0x0C2);
  //IdleI2C1();
  
  //Set up KMX62
  //i2c_write (CNTL2_DATA, CNTL2);
  
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
  
  /*
  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(1); // Use tft_setRotation(1) for 320x240
  */
  // initialize buffer to be at half DAC output
//  memset(WiFi_Buffer, 2048, sizeof(WiFi_Buffer));
  
  // init buffer with square wave
//  int i;
//  for (i = 0; i < max_chars_WiFi; i++) {
//      if (i % 40 < 5){
//          WiFi_Buffer[i] = 1800 | DAC_config_chan_A;
//      }
//      else {
//          WiFi_Buffer[i] = 2200 | DAC_config_chan_A;
//      }
//  }

  // DAC and DMA setup
  PPSOutput(2, RPB5, SDO2);
  PPSOutput(4, RPB10, SS2);
  OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 7256); // 1814 for 22.05 k
                                      // Interrupt flag, no ISR
  SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV | SPICON_FRMEN | SPICON_FRMPOL, 2);
  // Initializes SPI in framed mode
  DmaChnOpen(0,0,DMA_OPEN_DEFAULT); // Change default to auto // Auto mode to repeatedly send data
  //DmaChnSetEventControl(DMA_CHANNEL0, DMA_EV_START_IRQ_EN|DMA_EV_MATCH_EN);
  DmaChnSetTxfer(0, (void*) & WiFi_Buffer, (void*) & SPI2BUF, 15000, 2, 2);
      // Transfer from DAC_data1 table to SPI2BUF, 256 bytes total, 2 at a time
  // new, not sure if it is right, but should trigger interrupt on end of block
  //DmaChnSetEvEnableFlags(DMA_CHANNEL0, DMA_EV_BLOCK_DONE);
  //DmaChnSetMatchPattern(DMA_CHANNEL0, '\a');
  DmaChnSetEventControl(0, DMA_EV_START_IRQ(_TIMER_2_IRQ)); // Timer2 interrupt triggers DMA burst  
  //DmaChnEnable(0);
  /*
  // set up DMA channel 2 to playback error messages
  DmaChnOpen(2,2,DMA_OPEN_DEFAULT); // Change default to auto // Auto mode to repeatedly send data
  DmaChnSetTxfer(2, (void*) & Calibration, (void*) & SPI2BUF, sizeof(Calibration), 2, 2);
      // Transfer from DAC_data1 table to SPI2BUF, 256 bytes total, 2 at a time
  DmaChnSetEventControl(2, DMA_EV_START_IRQ(_TIMER_2_IRQ)); // Timer2 interrupt triggers DMA burst  
  DmaChnEnable(2);
  */
  ESP_setup();
  
  // round-robin scheduler for threads
  while (1){
//	  PT_SCHEDULE(protothread_accel(&pt_accel));
//      PT_SCHEDULE(protothread_GPS(&pt_gps));
      PT_SCHEDULE(protothread_WiFi(&pt_WiFi));
      PT_SCHEDULE(protothread_button(&pt_button));
      }
  } // main

// === end  ======================================================