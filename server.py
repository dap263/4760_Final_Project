import socket
import os
# path = r"G:\My Drive\Classes\4760\Final Project\hygdata_v3.csv"
# path_speech = r"G:\My Drive\Classes\4760\Final Project\andromeda.wav"
# cwd = os.getcwd()
# path_speech = cwd + "\andromeda.wav"

# # setup csv
import csv_parser
csv_parser.setup_csv()
print('csv parsed')

# setup TTS
from comtypes.client import CreateObject
import win32com.client
from comtypes.gen import SpeechLib

dac_ctrl = 12288

import wave 
import struct

from bitstring import BitArray, Bits
import numpy as np
import matplotlib.pyplot as plt

# DAC chan A control bits '0b0011000000000000' # AKA 12288

def do_TTS(msg):
    print('starting tts')
    engine = CreateObject("SAPI.SpVoice")
    stream = CreateObject("SAPI.SpFileStream")
    
    stream.Open('temp.wav', SpeechLib.SSFMCreateForWrite)
    engine.AudioOutputStream = stream
    # need to parse request to find requested message
    ind = msg.find(': ')
    ind1 = msg.find('\r\n')
    word = msg[ind+2:ind1]
    engine.speak(word, 0)
    stream.Close()
#     # need to change this to temp if i want it to be overwritten each time
    f = wave.open("temp.wav", 'rb')
    frames = f.readframes(f.getnframes())
    frames = [ struct.unpack("<h", frames[i] + frames[i+1])[0] for i in range(0,len(frames),2) ]
    # change samp rate by changing last argument, 4 means 5.5 kHz
    frames = [ np.uint8( ( ( frames[i] + 2**15 ) >> 9 ) + 32 ) for i in range(0, len(frames),4) ]
    ind = -1
    for i in range(len(frames)):
        if frames[i] != 0:
            ind = i
            break
    frames = frames[ind-10:]
    for i in range(len(frames)):
        if (frames[len(frames)-i-1] != 0):
            ind = i
            break
    frames = frames[:ind+20]
    #t = np.linspace(0, 10000, f.getnframes())
    # 7-bit ascii character offset by 32 to avoid control bits
    assert max(frames) <= 159 and min(frames) >= 32
    z = []
    for i in range(len(frames)):
        # if (i%4 != 0):
        z.append(Bits('uint:8=' + str(frames[i])))
    # not sure if \n is helpful
    # u8_str = ''.join(a.bin+'\n' for a in z)
    #u8_str = ''.join(str(a)+'\n' for a in frames)
    ascii_str = ''.join(chr(a) for a in frames)
    cltskt.send(ascii_str)
#     print 'done sending'

# base code taken from CS 4410 lecture with Robbert Van Renesse
skt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
skt.bind(('',5000))
skt.listen(5)
ra = 0
dec = 0
while True:
    cltskt, addr = skt.accept()
    msg = cltskt.recv(1000)
    print("got '%s' from %s" % (msg, addr))
    
    # should this come before or after? 
    # body = "<html><body>Hello {}</body></html>".format(addr)
    # response = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n" # % len(body)
    # cltskt.send(response ) # + body)

    if (msg.find('ra') != -1):
        ind = msg.find(': ')
        ind1 = msg.find(', ')
        ind2 = msg.find('\r\n')
        ra = float(msg[ind+2:ind1])
        dec = float(msg[ind1+1:ind2])
        # need to change ra and dec to specific positions in get request
        csv_parser.search_csv(ra, dec)
    elif (msg.find('tts') != -1):
        do_TTS(msg)
    
    cltskt.close()