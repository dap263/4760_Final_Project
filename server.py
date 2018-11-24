import socket
import os
path = r"G:\My Drive\Classes\4760\Final Project\hygdata_v3.csv"
path_speech = r"G:\My Drive\Classes\4760\Final Project\andromeda.wav"
cwd = os.getcwd()
path_speech = cwd + "\andromeda.wav"

# setup csv
import csv_parser
csv_parser.setup_csv()
print('csv parsed')

# setup TTS
from comtypes.client import CreateObject
import win32com.client
from comtypes.gen import SpeechLib

def do_TTS(msg):
    print('starting tts')
    engine = CreateObject("SAPI.SpVoice")
    stream = CreateObject("SAPI.SpFileStream")
    stream.Open('temp.wav', SpeechLib.SSFMCreateForWrite)
    engine.AudioOutputStream = stream
    # need to parse request to find requested message
    ind = msg.find(': ')
    ind1 = msg.find('\r\n')
    word = msg[ind+1:ind1]
    engine.speak(word, 0)
    stream.Close()
    # need to change this to temp if i want it to be overwritten each time
    f = open("temp.wav", 'rb')
    l = f.read(44) # strip off the wav header
    l = f.read(1024)
    l = l + '\r\n\r\n'
    while (l):
        print 'sending'
        cltskt.send(l)
        l = f.read(1024)
    f.close()
    print 'done sending'

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
    if (msg.find('ra') != -1):
        # need to change ra and dec to specific positions in get request
        csv_parser.search_csv(ra, dec)
    elif (msg.find('tts') != -1):
        do_TTS(msg)
    body = "<html><body>Hello {}</body></html>".format(addr)
    response = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n" % len(body)
    cltskt.send(response + body)
    cltskt.close()