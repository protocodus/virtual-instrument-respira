# Life: performer motion and noise audit

Life supplies live variation during ordinary playing, including long held
notes. The owner selected candidate C, 100 %, from an otherwise identical,
whole-file RMS-matched comparison with shipping 0.1.3 and Life 50 %.
That is a listening decision about performance depth, not a measurement
of accordionists' movement.

## What the model does

One shared player state drives the whole accordion. At 200 Hz, three
independent random draws pass through two-pole smoothers for effort
(0.65 Hz), lateral movement (0.27 Hz) and depth (0.37 Hz). Each is variance
normalized and bounded to +/-1.5; Life and activity glide over 40 ms.
The rates and ranges are authored playing choices. The separate random
stream does not disturb the reed turbulence sequence.

Auto's existing stroke level and wander remain. Life adds up to +/-33 %
of requested effort at its maximum, bounded together with that existing
variation to 65–145 %. The existing hand controller must move actual air
to reach that pressure. All held reeds share it; there is no per-note
amplitude envelope, pitch LFO or independently randomized voice gain.
Wheel mode preserves the user's hand trajectory, pressure and velocity
exactly: there Life changes only microphone pickup.

The sources move relative to the existing fixed coincident cardioids.
Ideal cardioid reception and relative 1/r pressure spreading change level
and stereo balance. Full Life permits at most 5 cm lateral and 8 cm depth
movement; these are chosen excursions, not observed motion statistics.
The bass end also follows normalized virtual bellows opening, with a
nominal travel of 6 L / (0.41 x 0.19 m) = 77 mm. That geometry is an
approximation: Assist changes the virtual hand's gearing, so this opening
coordinate is not a measured absolute bass-end trajectory. Targets retain
the existing 20 ms glide; Width zero remains mono. No moving reflection
paths, Doppler, or frequency-dependent source radiation are simulated.

## Evidence and its limits

- [Shure's accordion microphone comparison](https://www.shure.com/en-US/insights/eight-ways-to-mic-an-accordion)
  demonstrates why the instrument's movement and different radiating sides
  matter to pickup. It supports a moving-source model, not these excursions
  or a specific setup in the user's recordings.
- [Castronovo et al., 2018](https://pmc.ncbi.nlm.nih.gov/articles/PMC6230772/),
  DOI 10.1152/jn.00093.2018, measures common low-frequency input and force
  steadiness during isometric ankle contraction. It supports correlated
  force variation as a modelling direction. It does not measure accordion
  bellows speed, arm movement, or any coefficient used here.
- [Hohner's starting instructions](https://hohner.de/en/education/lets-get-started)
  place the left hand through the bass-hand strap. Repeated bellows control
  therefore should not automatically imply that a carrying handle rattles
  on every stroke.

The chosen motion remains a virtual performance. Owned, isolated recordings
with synchronized pressure/position would be needed to calibrate it as an
identified physical player's motion.

## Movement noises in the current engine

| Mechanism | Present | Driving quantity |
| --- | --- | --- |
| Key, bass/chord button, pallet and register stops | Yes | Part collision speed and effective mass |
| Push-reed leather valve slaps | Yes | Valve motion at release/reversal |
| Bellows fold hinges and panels | Yes | Physical pressure/motion and reversal |
| Bellows rubbing/rustle | Yes | Flow-derived bellows speed; stops below the stick threshold |
| Air-button hiss | Yes | Open valve and air flow |
| Reed-gap air noise | Yes | Gap-flow turbulence |
| Bass-hand strap creak/friction | No | No strap contact/tension model |
| Shoulder-strap rubbing | No | No body/strap contact model |
| Loose carrying handle, buckles or other hardware | No | No contact/slack/impact model |

Life does not add a constant noise bed. Its changes in actual Auto motion
drive the existing pressure/flow mechanisms. Mechanics still controls their
audible level. Strap/hardware sounds need identifiable contacts or isolated
reference captures before adding another voiced source; their absence is
documented rather than disguised with generic creaking.

## Fast-note correction and validation

An Auto Csardas excerpt exposed three notes weakened by a reversal after a
rest. Auto retained the old pressure while the keys were up, then began
reversing only as the next key opened. It now uses an available short rest
to prepare an imminent scheduled turn. The preparation time comes from
the required pressure reversal volume, 2P/K, divided by maximum hand flow,
plus the existing arm lag. The hand still moves air with the pallets shut.
No MIDI look-ahead is involved. This correction also applies at Life zero.

At Life above zero, capacity is checked on beats and includes the possible
swell's increased flow, using the square-root pressure relation at a fixed
opening. A whole bar can exceed a small bellows' remaining air; turning on
a beat avoids an arbitrary turn at its end stop. This is performer planning,
not a claim that every accordionist follows a four-bar reversal rule.

The fixed-pressure and fixed-geometry historical tests explicitly select
Life zero, retaining their prior limits. `Respira.Life` adds held-note
depth at 0/50/100 %, sample-identical block/reset behavior, manual-air
identity, audible moving pickup, mono and no-motion silence, air reserve
and short-note-after-rest witnesses. The two supplied recording libraries
and the user's MP3 references remain analysis-only and are not distributed.
