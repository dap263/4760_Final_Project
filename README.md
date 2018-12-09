# 4760 Final Project
Constellation Glasses

WiFi_GPS_Project Folder contains everything needed to open an MPLAB Project. Currently has a lot of unnecessary files in it, but most important files are pt_cornell_1_2_3.h, main.c, and GPS.h. 

init.lua is loaded on the ESP, and the other lua code is example code from the main NodeMCU github that I modified or a set of basic commands to send to the ESP. Error_messages.h contains conversions of the wave files to arrays of unsigned shorts (ORed with the DAC control bits, 12288), to be used as spoken error messages. They were created using the wav_to_header.py script. 

server.py is the local server that runs to parse the CSV from HYG-Database, and return the speech in the form of 7-bit ascii characters. csv_parser.py converts the csv to a list of arrays, only paying attention to the stars with proper names. 

i2c.c contains the code required to run just the accelerometer and magnetometer from the PIC32. 