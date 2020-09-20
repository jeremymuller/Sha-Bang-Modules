# Sha#Bang! Modules

A collection of innovative, probabilistic, and rhythmically complex modules for [VCVRack](https://vcvrack.com/). Created by [Jeremy Muller](http://jeremymuller.com/). These modules are free for download but please consider a donation through [PayPal](https://www.paypal.com/paypalme/jeremysmuller).

## Contents:

* [Cosmosis](#cosmosis)
* [Neutrinode](#neutrinode)
* [PolyrhythmClock](#polyrhythm-clock)
* [RandGates](#randgates)
* [StochSeq](#stochseq)
* [StochSeq4](#stochseq4)

---

### Cosmosis

*TODO*

---

### Neutrinode

![Neutrinode](/docs/Neutrinode.png)

*A unique visual-based sequencer where time moves from the center of the 4 independent nodes out to the connected particles. Can generate interesting rhythmic textures and can be animated so that the textures change over time.*

Click on nodes to position them. Click anywhere else to add/drag new particles. To remove particles click/drag them out of the dark display area.

##### BUTTONS:
- `PLAY` turns on/off all nodes.
- `CLR` removes all particles.
- `ON` turns on/off individual nodes.
##### KNOBS:
- `BPM` tempo of all nodes.
- `SPEED` is the velocity of movement of nodes when `MOVE` is switched on.
- Two knobs control the root note and the scale. The Messiaen modes are based on the wonderful French composer, Olivier Messiaen and his [modes of limited transposition](https://en.wikipedia.org/wiki/Mode_of_limited_transposition) found in his book *The Technique of My Musical Language*.
  - Chords like MM7, Mm7, mm7, etc. are based on [seventh chords](https://en.wikipedia.org/wiki/Seventh_chord). 
- `OCT` controls the octaves of individual nodes.
##### SWITCHES:
- `MOVE` each node will randomly move around the dispay area. 
- `PITCH` left side will take pitch from the radius of particles. The right side will take pitch based on the Y position (height) in the display. The left switch is good if you'd like to generate randomness in the pitches, however, if you want more control over the pitch then select the right switch.
##### OUTPUTS:
- 


---

### Polyrhythm Clock

![PolyrhythmClock](/docs/PolyrhythmClock.png)

*A clock featuring three layers of embedded rhythms. It's easy to multiply and/or divide the clock with virtually any rhythm you want using this clock.*

- button turns it on
##### KNOBS:
- large main knob at the top controls the bpm (beats per minute).
- left-side knobs within each tuplet controls the numerator part of the fraction (or ratio).
- right-side knobs within each tuplet controls the denominator part of the fraction (or ratio).
- middle solid-color knobs within each tuplet controls the probability of outputting a pulse.
##### OUTPUTS:
- first one outputs the current bpm.
- `TUPLET 1` outputs the ratio compared to the bpm.
- `TUPLET 2` outputs the ratio compared to `TUPLET 1`.
- `TUPLET 3` outputs the ratio compared to `TUPLET 2`.
  
TODO: See the image below if this is confusing.

---
### RandGates

![RandGates](/docs/RandGates.png)

*Randomly outputs one of the 4 inputs.*

##### INPUTS:
- `TRG` randomizes the output
- `INS` (purple, blue, aqua, red) are any type of input, i.e. gates or ±5 volts.
##### KNOB:
- Gives a higher probability weight to the chosen input. All the way to the right is uniform randomness.
##### OUTPUT:
- `OUT` outputs either the randomly chosen input as either a pulse or ±5 volts.

---

### StochSeq

*A sequencer that uses stochastic (probabilistic) patterns. The outputs can be used as gates (triggered based on probability) or as ±5 volts (probability is converted to voltage).*

Click and/or drag to draw your own patterns!

##### INPUTS:
- `CLK` controls timing.
- `RST` resets sequence to beginning of timeline.
##### KNOBS:
- `LEN` length of the sequence.
- `PATT` selects from preset patterns.
- `SPREAD` determines the pitch spread of the `V/OCT` output. 
  - Center will always be the same pitch. 
  - Right of center: 100% = higher pitch, 0% = lower pitch. 
  - Left of center: 100% = lower pitch, 0% = higher pitch.
##### BUTTONS:
- `RND` randomizes all probabilities.
- `INV` inverts all probabilities.
- `DIM` cuts the current pattern in half and repeats. Keep clicking this button to continue to diminish the pattern.
##### OUTPUTS:
- `GATE` outputs a pulse based on the probability of the current sequence position. (i.e. a slider at 50% will only trigger a pulse half of the time)
- `V/OCT` outputs TODO..........

---

### StochSeq4

![StochSeq4](/docs/StochSeq4.png)

*A sequencer that uses four independent stochastic (probabilistic) patterns. The outputs can be used as gates (triggered based on probability) or as ±5 volts (probability is converted to voltage).*

*TODO*

---
