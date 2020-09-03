# Jeremy's Modules

A collection of experimental, innovative, & probabilistic modules for [VCVRack](https://vcvrack.com/). These modules are free for download but please consider a donation through [PayPal](https://www.paypal.com/paypalme/jeremysmuller). Also, find out more about me [here](http://jeremymuller.com/).

## Contents:

* [StochSeq](#stochseq)
* [StochSeq4](#stochseq4)
* [PolyrhythmClock](#polyrhythm-clock)
* [RandGates](#randgates)

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

*TODO*

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
