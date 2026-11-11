import mido, sys, numpy as np
def load(f):
    m=mido.MidiFile(f); out={}
    tempo=500000
    for t in m.tracks:
        now=0.0; notes=[]; on={}; cc=[]; other=[]
        tick=0
        for msg in t:
            now+=mido.tick2second(msg.time,m.ticks_per_beat,tempo)
            if msg.type=='set_tempo': tempo=msg.tempo
            if msg.type=='note_on' and msg.velocity: on[(msg.channel,msg.note)]=(now,msg.velocity)
            elif msg.type in('note_off','note_on'):
                s,v=on.pop((msg.channel,msg.note)); notes.append((s,now-s,msg.note,v,msg.channel))
            elif msg.type=='control_change': cc.append((now,msg.control,msg.value))
            elif msg.type not in ('end_of_track','track_name'): other.append((now,msg))
        out[t.name]=dict(notes=sorted(notes),cc=cc,other=other)
    return m,out
if __name__=='__main__':
    m,o=load(sys.argv[1])
    for name,d in o.items():
        n=d['notes']
        if not n: print(name,d['other'][:5]); continue
        print('==',name,len(n),'chans',set(x[4] for x in n))
        v=np.array([x[3] for x in n]); print(' vel',v.min(),v.mean().round(1),v.max())
        # section summary per 5 s
        for s0 in np.arange(0,n[-1][0]+5,5):
            seg=[x for x in n if s0<=x[0]<s0+5]
            c1=[x[2] for x in d['cc'] if s0<=x[0]<s0+5 and x[1]==1]
            low=sum(1 for x in seg if x[2]<48)
            print(f' {s0:5.0f}s on={len(seg):3d} low={low:3d} vel={np.mean([x[3] for x in seg]) if seg else 0:5.1f} cc1={np.mean(c1) if c1 else -1:5.1f} [{min(c1) if c1 else -1},{max(c1) if c1 else -1}] pitches {sorted(set(x[2] for x in seg))[:3]}..{max([x[2] for x in seg]) if seg else 0}')
