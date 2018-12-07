/* 
 * File:   GPS.h
 * Author: Alec
 *
 * Created on November 16, 2018, 12:42 PM
 */

#ifndef GPS_H
#define	GPS_H

#ifdef	__cplusplus
extern "C" {
#endif

/* Authors: Junyin Chen (jc2954@cornell.edu)
 *          Haonan Liu  (hl955@cornell.edu)
 * 
 * Borrowed & modified from the following ECE4760 project: https://goo.gl/ccsXBv
 * 
 */

//====================================================================
// build a string from the UART2 /////////////
//////////////////////////////////////////////
#define max_chars_GPS 256   // for input/output buffer
char PT_term_buffer_GPS[max_chars_GPS];	//buffer for storing string from UART2
char PT_term_buffer_GPS_RMC[max_chars_GPS];	//buffer for storing GPRMC string
int num_char_GPS;					//number of characters in the string of UART2
//int num_send_chars;
//unsigned char GPGGA; // Boolean GPS string is GGA format
unsigned char GPRMC; // Boolean GPS string is RMC format
//unsigned char OKrn;
char *RMCptr;
char *RMCend;

//the child thread to get serial buffer
int PT_GetSerialBufferGPS(struct pt *pt)
{
    
    // mark the beginning of the input thread
    PT_BEGIN(pt);

    num_char_GPS = 0;

    while(num_char_GPS < max_chars_GPS)
    {	//if frame work happens, just throw that away
        if(UART1GetErrors() & 0x02){
            UART1ClearAllErrors();
            PT_YIELD_TIME_msec(1);
            break;
        }
        // get the character
        // yield until there is a valid character so that other threads can
        // execute
        PT_YIELD_UNTIL(pt, UARTReceivedDataIsAvailable(UART1));
                
        PT_term_buffer_GPS[num_char_GPS++] = UARTGetDataByte(UART1);
        
    } //end while(num_char < max_size)
    
    PT_term_buffer_GPS[num_char_GPS] = 0; // zero terminate the string
    
    RMCptr = strstr(PT_term_buffer_GPS, "$GPRMC");
    if (RMCptr) {
        RMCend = strstr(RMCptr, "\n");
    }

    if (RMCptr && RMCend) {
        *RMCend = 0;
        memcpy(PT_term_buffer_GPS_RMC, RMCptr, (RMCend-RMCptr)*sizeof(*PT_term_buffer_GPS));
    }
    if (PT_term_buffer_GPS_RMC[0] == '$' &&
        PT_term_buffer_GPS_RMC[1] == 'G' &&
        PT_term_buffer_GPS_RMC[2] == 'P' &&
        PT_term_buffer_GPS_RMC[3] == 'R' &&
        PT_term_buffer_GPS_RMC[4] == 'M' &&
        PT_term_buffer_GPS_RMC[5] == 'C'   ){
        //flag this variable	
        GPRMC=1;
    }

    // kill this input thread, to allow spawning thread to execute
    PT_EXIT(pt);
    // and indicate the end of the thread
    PT_END(pt);
}

//====================================================================
// build a string from the UART2 /////////////
//////////////////////////////////////////////
//#define max_chars_WiFi 256
//char PT_term_buffer_WiFi[max_chars_GPS]; //buffer for storing string from UART1
//int num_char_WiFi; // number of characters in the string from UART1
//char PT_terminate_char_WiFi; // WiFi UART termination character
//
//int PT_GetSerialBufferWiFi(struct pt *pt)
//{
//    
//    static char character;
//    
//    // mark the beginning of the input thread
//    PT_BEGIN(pt);
//
//    num_char_WiFi = 0;
//
//    while(num_char_WiFi < max_chars_WiFi)
//    {	//if frame work happens, just throw that away
//        if(UART1GetErrors() & 0x02){
//            UART1ClearAllErrors();
//            PT_YIELD_TIME_msec(1);
//            break;
//        }
//        // get the character
//        // yield until there is a valid character so that other threads can
//        // execute
//        PT_YIELD_UNTIL(pt, UARTReceivedDataIsAvailable(UART1));
//        
//        character = UARTGetDataByte(UART1);
//        
//        if ((character>0) && (character == PT_terminate_char_WiFi)) {
//            // leave the while loop
//            break;
//        }
//        
//        PT_term_buffer_WiFi[num_char_WiFi++] = character;
//        
//    } //end while(num_char < max_size)
//    
//    PT_term_buffer_WiFi[num_char_WiFi] = 0; // zero terminate the string
//
//    // kill this input thread, to allow spawning thread to execute
//    PT_EXIT(pt);
//    // and indicate the end of the thread
//    PT_END(pt);
//}

float GPS_Lat;	// Latitude in degrees (positive = North, negative = South)
float GPS_Lon;	// Longitude in degrees (positive = East, negative = West)
char GPS_type[6];	
int  GPS_quality;	
char GPS_valid; // A=active, V=void
int GPS_fix;    // boolean, does GPS have a fix
int GPS_time_h; // UTC time hours
int GPS_time_m; // UTC time minutes
int GPS_time_s;	// UTC time seconds
int GPS_day;    // UTC day
int GPS_month;  // UTC month
int GPS_year;   // UTC year
int GPS_date;

// Parse a $GPRMC NMEA string
void parse_RMC(char* PT_term_buffer_GPS) {
    float GPS_time, GPS_dmLat, GPS_dmLon, GPS_speed,
            GPS_course, GPS_mag_var;
    char GPS_NS, GPS_WE, GPS_mag_var_WE;
    int GPS_checksum;
    sscanf(PT_term_buffer_GPS, "%6c,%f,%c,%f,%c,%f,%c,%f,%f,%d,%f,%c*%d", 
            GPS_type, &GPS_time, &GPS_valid, &GPS_dmLat, &GPS_NS,
            &GPS_dmLon, &GPS_WE, &GPS_speed, &GPS_course, &GPS_date,
            &GPS_mag_var, &GPS_mag_var_WE, &GPS_checksum);
    
    // Convert from valid 'A'/'V' to true/false
    GPS_fix = GPS_valid == 'A';
    
    // Convert from ddmm.m to d.d format for latitude and longitude
    int GPS_Lat_deg, GPS_Lon_deg; // Temp variables
    float GPS_Lat_frac, GPS_Lon_frac; // Temp variables
    
    GPS_Lat_deg = GPS_dmLat / 100;
    GPS_Lon_deg = GPS_dmLon / 100;
    GPS_Lat_frac = GPS_dmLat - (float) GPS_Lat_deg;
    GPS_Lon_frac = GPS_dmLat - (float) GPS_Lon_deg;
    GPS_Lat = (float) GPS_Lat_deg + GPS_Lat_frac/60; // Return value
    if (GPS_NS == 'S') GPS_Lat = -GPS_Lat;
    GPS_Lon = (float) GPS_Lon_deg + GPS_Lon_frac/60; // Return value
    if (GPS_WE == 'W') GPS_Lon = -GPS_Lon;
    
    // Convert from hhmmss.s format to hours, minutes, and seconds
    GPS_time_h = GPS_time / 10000;
    GPS_time_m = (GPS_time - (GPS_time_h*10000)) / 100;
    GPS_time_s = GPS_time - (GPS_time_h*10000) - (GPS_time_m*100);
    
    // Convert from ddmmyy format to days, months, and years
    GPS_day = GPS_date / 10000;
    GPS_month = (GPS_date - (GPS_day*10000)) / 100;
    GPS_year = GPS_date - (GPS_day*10000) - (GPS_month*100) + 2000;
}

// Parse a $GPGGA NMEA string
//void parse_GGA(char* PT_term_buffer_GPS){
//    // Temp variables
//    float GPS_time, GPS_dmLat, GPS_dmLon;
//    char GPS_NS, GPS_WE;
//    int GPS_Lat_deg, GPS_Lon_deg;
//    float GPS_Lat_frac, GPS_Lon_frac;
//    
//    sscanf(PT_term_buffer_GPS, "%6c,%f,%f,%c,%f,%c,%d", 
//            GPS_type, &GPS_time, &GPS_dmLat, &GPS_NS, &GPS_dmLon,
//            &GPS_WE, &GPS_quality);
//    
//    // Convert from ddmm.m to d.d format for latitude and longitude
//    GPS_Lat_deg = GPS_dmLat / 100;
//    GPS_Lon_deg = GPS_dmLon / 100;
//    GPS_Lat_frac = GPS_dmLat - (float) GPS_Lat_deg;
//    GPS_Lon_frac = GPS_dmLat - (float) GPS_Lon_deg;
//    GPS_Lat = (float) GPS_Lat_deg + GPS_Lat_frac/60; // Return value
//    GPS_Lon = (float) GPS_Lon_deg + GPS_Lon_frac/60; // Return value
//    
//    GPS_time_h = GPS_time / 10000;
//    GPS_time_m = (GPS_time - GPS_time_h) / 100;
//    GPS_time_s = GPS_time - GPS_time_h - GPS_time_m;
//}


#ifdef	__cplusplus
}
#endif

#endif	/* GPS_H */

