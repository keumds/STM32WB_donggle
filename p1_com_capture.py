"""Raw Cyton 33-byte capture / counter analysis: pip install pyserial
Use: python p1_com_capture.py COM24 15
DO NOT have Symbio/BrainBay opening the same COM simultaneously.
"""
import sys,time,serial
port=sys.argv[1] if len(sys.argv)>1 else 'COM24'
seconds=float(sys.argv[2]) if len(sys.argv)>2 else 15.
raw=bytearray(); t0=time.monotonic()
with serial.Serial(port,115200,timeout=0.2, xonxoff=False, rtscts=False, dsrdtr=False) as ser:
    while time.monotonic()-t0 < seconds:
        raw.extend(ser.read(4096))
with open('p1_raw_capture.bin','wb') as f: f.write(raw)
frames=[];bad=0; i=0
while i+33<=len(raw):
    if raw[i]==0xA0 and raw[i+32] in range(0xC0,0xD0):
        frames.append(raw[i+1]);i+=33
    else:
        if raw[i]==0xA0:bad+=1
        i+=1
missing=0;gap_counts={};
for prev,nxt in zip(frames,frames[1:]):
    delta=(nxt-prev)&255
    gap_counts[delta]=gap_counts.get(delta,0)+1
    if delta:missing+=delta-1
print('bytes',len(raw),'frames',len(frames),'valid_sps',round(len(frames)/seconds,2))
print('invalid_header_candidates',bad,'unconsumed_bytes',len(raw)-len(frames)*33)
print('counter_deltas',sorted(gap_counts.items(),key=lambda x:-x[1])[:12])
print('estimated_missing',missing,'estimated_coverage',round(len(frames)/(len(frames)+missing),3) if frames else None)
print('Saved p1_raw_capture.bin')
