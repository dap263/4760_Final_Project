import wave
import struct
f = wave.open("Calibration.wav", 'rb')
frames = f.readframes(f.getnframes())
frames = [ struct.unpack("<h", frames[i] + frames[i+1])[0] for i in range(0,len(frames),2) ]
assert min(frames) >= -(2**14) and max(frames) <= 2**14
# need to change the range to fit within max 12 bit range of the DAC
frames = [ (( frames[i] + (2**14))/9 + 12288 ) for i in range(0, len(frames),4) ]
assert min(frames) >= 12288 and max(frames) <= (12288+2**12)
ind = -1
zero = frames[0]
for i in range(len(frames)):
    # whatever value zero is encoded as
    if frames[i] != zero:
        ind = i
        break
ind = -1
frames = frames[ind-10:]
# zeroing stuff doesn't always work
for i in range(len(frames)):
    if (frames[len(frames)-i-1] != zero):
        ind = i
        break
if (ind == -1):
    print('error')
    assert ind == 0
# comment this out to prevent incorrect zeroing
#frames = frames[:ind+20]

with open('Error_messages.h', 'a') as f:
    f.write("\n")
    f.write("short Calibration = [")
    count = 0
    for item in frames:
        if (count % 20 == 0):
            f.write("\n")
        count += 1
        f.write("%s, " % item)
    f.write("];")
