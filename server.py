import socket
import os
# path = r"G:\My Drive\Classes\4760\Final Project\hygdata_v3.csv"
# path_speech = r"G:\My Drive\Classes\4760\Final Project\andromeda.wav"
# cwd = os.getcwd()
# path_speech = cwd + "\andromeda.wav"

# # setup csv
# import csv_parser
# csv_parser.setup_csv()
# print('csv parsed')

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
# def do_TTS(msg):
#     a = np.uint8()
#     # should produce array of len 63*500
#     # x = np.arange(0, 2000 * np.pi, .1)
#     # y = np.sin(x)
#     # # plt.plot(x,y)
#     # # y = np.uint8(127*(y+1))
#     # # take every other value
#     # y = [ np.uint8(127*(y[i]+1)) for i in range(0, len(y), 4) ]
#     # # plt.plot(x,y)
#     # assert max(y) <= 255
#     # assert min(y) >= 0
#     y = []
#     x = []
#     for i in range (30000):
#         x.append(i)
#         if i % 2 == 0:
#             y.append(np.uint8(127 + 32))
#         else:
#             y.append(np.uint8(0 + 32))
            
#     # plt.plot(x,y)
#     z = []
#     u8_str = ''
#     # for i in range(len(y)):
#         # z.append(BitArray('uintle:8='+str(y[i])))
#         # z.append(Bits('uint:16=' + str(y[i])))
#     #plt.plot(x,z)
#     # plt.show()
#     # u8_str = ''.join(a.bin+'\n' for a in z)
#     ascii_str = ''.join(chr(a) for a in y)
#     cltskt.send(ascii_str)

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
    engine.speak('hello, Bruce', 0)
    stream.Close()
#     # need to change this to temp if i want it to be overwritten each time
    f = wave.open("temp.wav", 'rb')
#     # f.read(44) # strip off the wav header
#     # downsample from 16 bit to 8 bit stream
    # l = ''
    frames = f.readframes(f.getnframes())
#     # temp = []
#     # for i in range(0, len(frames), 2):
#     #     temp[int(i/2)] = struct.unpack("<h", frames[i] + frames[i+1])
#     #     l = l + '{}'.format((temp[i]>>7)+2**6)
    
#     # should this just be frames[i+1] to do the top 8 bits? 
#     # frames = [ struct.unpack("<h", frames[i] + frames[i+1])[0] for i in range(0,len(frames),2) ]
    frames = [ struct.unpack("<h", frames[i] + frames[i+1])[0] for i in range(0,len(frames),2) ]
    frames = [ np.uint8( ( ( frames[i] + 2**15 ) >> 8 ) + 32 ) for i in range(0, len(frames)) ]
    t = np.linspace(0, 10000, f.getnframes())
    assert max(frames) <= 255 and min(frames) >= 32
    plt.plot(t, frames)
    plt.show()
    z = []
    for i in range(len(frames)):
        # if (i%4 != 0):
        z.append(Bits('uint:8=' + str(frames[i])))
    # not sure if \n is helpful
    # u8_str = ''.join(a.bin+'\n' for a in z)
    #u8_str = ''.join(str(a)+'\n' for a in frames)
    ascii_str = ''.join(chr(a) for a in frames)
    cltskt.send(ascii_str)
#     # and then can just add 2**7 to center it in the positive range
#     # frames = [ (frames[i]>>7)+2**6 for i in range(0, len(frames)) ]
#     frames = [ np.uint8((frames[i]>>8) +2**7) for i in range(0, len(frames)) ]
#     for i in range (len(frames)):
#         l = l + str(frames[i])
#     # for i in range (1450):
#     #     if (i % 4 != 0):
#     #         l = l + f.readframes(1)[1:]
#     #     else: 
#     #         f.readframes(1)
#         #f.read(1)
#     # l = l + '\r\n\r\n'
#     while (frames):
#         print 'sending'
#         cltskt.send(l)
#         l = ''
#         frames = f.readframes(1000)
#         frames = [ struct.unpack("<h", frames[i] + frames[i+1])[0] for i in range(0,len(frames),2) ]
#         frames = [ (frames[i]>>8) +2**7 for i in range(0, len(frames)) ]
#         for i in range (len(frames)):
#             l = l + str(frames[i])
#     f.close()
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
    response = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n" # % len(body)
    cltskt.send(response )# + body)

    if (msg.find('ra') != -1):
        # need to change ra and dec to specific positions in get request
        csv_parser.search_csv(ra, dec)
    elif (msg.find('tts') != -1):
        do_TTS(msg)
    
    cltskt.close()