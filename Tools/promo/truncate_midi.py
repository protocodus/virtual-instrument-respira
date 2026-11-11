"""Cut a performed part at a phrase end: drop every event after <seconds> and
lift any key still down there, so the engine decays naturally from that point."""
import sys, mido
src, dst, cut = sys.argv[1], sys.argv[2], float(sys.argv[3])
m = mido.MidiFile(src); out = mido.MidiFile(ticks_per_beat=m.ticks_per_beat)
for t in m.tracks:
    tempo, now, nt, down, keep = 500000, 0.0, mido.MidiTrack(), set(), 0
    for msg in t:
        now += mido.tick2second(msg.time, m.ticks_per_beat, tempo)
        if msg.type == 'set_tempo': tempo = msg.tempo
        if now > cut and not msg.is_meta: keep += msg.time; continue
        if msg.type == 'end_of_track': continue
        nt.append(msg.copy(time=msg.time + keep)); keep = 0
        if msg.type == 'note_on' and msg.velocity: down.add((msg.channel, msg.note))
        elif msg.type in ('note_on', 'note_off'): down.discard((msg.channel, msg.note))
    for i, (ch, n) in enumerate(sorted(down)):
        nt.append(mido.Message('note_off', channel=ch, note=n, velocity=0, time=keep if i == 0 else 0)); keep = 0
    nt.append(mido.MetaMessage('end_of_track', time=keep)); out.tracks.append(nt)
out.save(dst)
