# Sha#Bang! Modules

A collection of innovative, probabilistic, generative, and rhythmically complex modules for [VCVRack](https://vcvrack.com/). Created by [Jeremy Muller](http://jeremymuller.com/). These modules are free for download but please consider a donation through [PayPal](https://www.paypal.com/paypalme/jeremysmuller).

Add these modules to VCVRack through the [Rack library](https://library.vcvrack.com/?brand=Sha%23Bang!%20Modules).

*TODO: videos*

![modulesall](/docs/Modules_ALL.gif)

## Contents:

* [Cosmosis](#cosmosis)
* [Neutrinode](#neutrinode)
* [Orbitones](#orbitones)
* [Photron](#photron)
* [PolyrhythmClock](#polyrhythm-clock)
* [QubitCrusher](#qubit-crusher)
* [RandGates](#randgates)
* [StochSeq](#stochseq)
* [StochSeq4](#stochseq4)

---

### Cosmosis

![Cosmosis](/docs/Cosmosis.png)

*A continuous sequencer based on constellations where time can move left->right, right->left, up->down, and down->up.*

Click anywhere to add/drag new stars. To remove stars click/drag them out of the dark display area.

##### BUTTONS:
- `PLAY` starts the sequencer.
- `CLR` removes all stars.
- `POS` randomizes positions of stars.
- `RAD` randomizes the radii of stars.
##### INPUTS:
- `PLAY` trigger starts/stops the sequencer.
- `SPEED` control voltage manipulates the current speed.
- `RESET` trigger resets the current sequence position.
- Root note input will accept V/OCT to set the root note.
- `POS` accepts gate that triggers randomizing positions of stars.
- `RAD` accepts gate that triggers randomizing radii of stars.
##### KNOBS:
- `SPEED` tempo of sequencer.
- `PATT` sets the constellation pattern.
- Two knobs control the root note and the scale. The Messiaen modes are based on the wonderful French composer, Olivier Messiaen and his [modes of limited transposition](https://en.wikipedia.org/wiki/Mode_of_limited_transposition) found in his book *The Technique of My Musical Language*.
- Chords like MM7, Mm7, mm7, etc. are based on [seventh chords](https://en.wikipedia.org/wiki/Seventh_chord).
- `OCT` controls the octaves of individual sequencers.
- `MODE` sets which direction time will traverse the stars.
##### SWITCH:
- `PITCH` left side will take pitch from the radius of particles. The right side will take pitch based on the X or Y position (width or height respectively) in the display. If the `MODE` is set to either Blue or Red, in which case time travels down or up, pitch is taken from the horizontal position of the stars. The left switch is good if you'd like to generate randomness in the pitches, however, if you want more control over the pitch then select the right switch.
##### OUTPUTS:
- `GATES` output pulses.
- `V/OCT` outputs ±5 volts.

---

### Neutrinode

![Neutrinode](/docs/Neutrinode.png)

*A unique visual-based sequencer where time moves from the center of the 4 independent nodes out to the connected particles. Can generate interesting rhythmic textures and can be animated so that the textures change over time.*

Click on nodes to position them. Click anywhere else to add/drag new particles. To remove particles click/drag them out of the dark display area.

##### RIGHT-CLICK MENU:
- Collisions are turned on or off for the nodes.
##### BUTTONS:
- `PLAY` turns on/off all nodes.
- `MOVE` each node will randomly move around the dispay area.
- `CLR` removes all particles.
- `ON` turns on/off individual nodes.
##### INPUTS:
- `PLAY` trigger starts/stops the generator.
- `BPM` control voltage manipulates the current bpm.
- `MOVE` trigger turns on/off the random movement of nodes.
- Root note input will accept V/OCT to set the root note.
##### KNOBS:
- `BPM` tempo of all nodes.
- `SPEED` is the velocity of movement of nodes when `MOVE` is switched on.
- Two knobs control the root note and the scale. The Messiaen modes are based on the wonderful French composer, Olivier Messiaen and his [modes of limited transposition](https://en.wikipedia.org/wiki/Mode_of_limited_transposition) found in his book *The Technique of My Musical Language*.
- Chords like MM7, Mm7, mm7, etc. are based on [seventh chords](https://en.wikipedia.org/wiki/Seventh_chord).
- `OCT` controls the octaves of individual nodes.
##### SWITCHES:
- `PITCH` left side will take pitch from the radius of particles. The right side will take pitch based on the Y position (height) in the display. The left switch is good if you'd like to generate randomness in the pitches, however, if you want more control over the pitch then select the right switch.
##### OUTPUTS:
- `GATES` and `V/OCT` output only from the corresponding node color.
- `ALL` outputs all nodes.

*:warning: There are a maximum of 16 particles (for 16 polyphonic channels) per node. However, when using the `ALL` outputs, there is potential to reach that maximum (16 x 4 = 64) in which case it does voice stealing.*


---

### Orbitones

*LFO blah blah...todo*

---

### Photron

![Photron](/docs/Photron.png)

*An animated visualizer with inputs. Uses color flocking based on the Craig Reynolds boids flocking algorithm.*

##### BUTTON:
- color version or black & white version
##### INPUTS:
- TODO

---

### Polyrhythm Clock

![PolyrhythmClock](/docs/PolyrhythmClock.png)

*A clock featuring three layers of embedded rhythms. It's easy to multiply and/or divide the clock with virtually any rhythm you want using this clock.*

##### RIGHT-CLICK MENU:
- External Clock Mode:
  - `CV` controls bpm (beats per minute) based on the input voltage using this formula: 120 * 2<sup>V</sup>.
  - `12 PPQN` controls bpm based on 12 pulses per quarter note.
  - `24 PPQN` controls bpm based on 24 pulses per quarter note.
- If the mode is set to either of the `PPQN` modes, the clock will turn on automatically when it receives a pulse. It will also turn off automatically after it times out from not receiving any more pulses.
##### INPUT:
- `EXT` is an external clock to control the PolyrhythmClock determined by the External Clock Mode.
##### BUTTON:
- on or off
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

---

### Qubit Crusher

![QubitCrusher](/docs/QubitCrusher.png)

*A bit crusher and downsampler using fractional rates with the ability to modulate bit rate & sample rate, or randomly trigger new bit rates & sample rates.*

##### INPUTS:
- `IN` input signal to be processed.
- `TRG`s both inputs accept gates that trigger random bit rates and/or sample rates.
- Inputs connected to `MOD` knob accept modulation sources (i.e. LFO).
##### KNOBS:
- `BITS` sets bit rate. This is overridden by `TRG` input.
- `SAMP` sets sample rate. This is overriden by `TRG` input.
- `MOD`s set the amount of modulation from modulation signal.
##### OUTPUT:
- `OUT` output signal.

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

![StochSeq](/docs/StochSeq.png)

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
  - Right of center: 0% to 100% in sliders correspond to low to high pitches.
  - Left of center: 0% to 100% in sliders correspond to high to low pitches (inverted).
- Two knobs control the root note and the scale. The Messiaen modes are based on the wonderful French composer, Olivier Messiaen and his [modes of limited transposition](https://en.wikipedia.org/wiki/Mode_of_limited_transposition) found in his book *The Technique of My Musical Language*.
- Chords like MM7, Mm7, mm7, etc. are based on [seventh chords](https://en.wikipedia.org/wiki/Seventh_chord).
##### BUTTONS:
- `RND` randomizes all probabilities.
- `INV` inverts all probabilities.
- `DIM` cuts the current pattern in half and repeats. Keep clicking this button to continue to diminish the pattern.
##### OUTPUTS:
- `GATE` outputs a pulse based on the probability of the current sequence position. (i.e. a slider at 50% will only trigger a pulse half of the time)
- `V/OCT` outputs pitch based on the slider position and `SPREAD` knob, regardless of probability of the event.
- `GATES` outputs correspond to each position in the sequence.

---

### StochSeq4

![StochSeq4](/docs/StochSeq4.png)

*A sequencer that uses four independent stochastic (probabilistic) patterns. The outputs can be used as gates (triggered based on probability) or as ±5 volts (probability is converted to voltage).*

Click and/or drag to draw your own patterns!

##### INPUTS:
- `MCLK` controls timing of all patterns (overrides all individual clocks).
- `CLK` controls timing of individual patterns.
- `RST` resets sequences to beginning of timeline.
- `RND` gate input randomizes all probabilities.
- `INV` gate input inverts all probabilities.
- `DIM` gate input cuts the current pattern in half and repeats.
##### KNOBS:
- `LEN` length of the individual sequence.
- `PATT` selects from preset patterns.
- `SPREAD` determines the pitch spread of the `V/OCT` output.
  - Center will always be the same pitch.
  - Right of center: 0% to 100% in sliders correspond to low to high pitches.
  - Left of center: 0% to 100% in sliders correspond to high to low pitches (inverted).
- Two knobs control the root note and the scale just like [StochSeq](#stochseq).
##### BUTTONS:
- `RND` randomizes all probabilities.
- `INV` inverts all probabilities.
- `DIM` cuts the current pattern in half and repeats. Keep clicking this button to continue to diminish the pattern.
##### OUTPUTS:
- `GATE` outputs a pulse based on the probability of the current individual sequence position. (i.e. a slider at 50% will only trigger a pulse half of the time)
- `V/OCT` outputs pitch based on the slider position and `SPREAD` knob, regardless of probability of the event.

---
