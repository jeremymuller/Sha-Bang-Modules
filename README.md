# Jeremy's Modules

A collection of experimental, innovative, & probabilistic modules for [VCVRack](https://vcvrack.com/). These modules are free for download but please consider a donation through [PayPal](https://www.paypal.com/paypalme/jeremysmuller). Also, find out more about me [here](http://jeremymuller.com/).

## Contents:

* StochSeq
* StochSeq4
* [PolyrhythmClock](#polyrhythm-clock)
* [RandGates](#randgates)

### Polyrhythm Clock

*A clock featuring three layers of embedded rhythms. It's easy to multiply and/or divide the clock with virtually any rhythm you want using this clock.*

- button turns it on
- knobs:
  - large main knob at the top controls the bpm (beats per minute).
  - left-side knobs within each tuplet controls the numerator part of the fraction (or ratio).
  - right-side knobs within each tuplet controls the denominator part of the fraction (or ratio).
  - middle solid-color knobs within each tuplet controls the probability of outputting a pulse.
- outputs:
  - first one outputs the current bpm.
  - `TUPLET 1` outputs the ratio compared to the bpm.
  - `TUPLET 2` outputs the ratio compared to `TUPLET 1`.
  - `TUPLET 3` outputs the ratio compared to `TUPLET 2`.
  
TODO: See the image below if this is confusing.


### RandGates

*Randomly outputs one of the 4 inputs.*

- inputs:
  - `TRG` randomizes the output
  - `INS` (purple, blue, aqua, red) are any type of input, i.e. gates or ±5 volts.
- **Weight Knob** gives a higher probability to the chosen input. All the way to the right is uniform randomness.
- `OUT` outputs either the chosen gate or ±5 volts.
