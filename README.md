# Sha#Bang! Modules

A collection of innovative, probabilistic, generative, and rhythmically complex modules for [VCVRack](https://vcvrack.com/). Created by [Jeremy Muller](http://jeremymuller.com/). If you enjoy using these modules and/or have made money with these, please consider [sponsoring](https://github.com/sponsors/jeremymuller) this project or giving a donation through [PayPal](https://www.paypal.com/paypalme/jeremysmuller). Huge shoutout to **Andras Szabo** for being the very first sponsor of Sha#Bang! Modules! Thank you :pray:

Add these modules to VCVRack through the [Rack library](https://library.vcvrack.com/?brand=Sha%23Bang!%20Modules).

![modulesall](/docs/Modules_ALL3.gif)

## Contents:

* [Collider](#collider) :tv:
* [Cosmosis](#cosmosis) :tv:
* [Neutrinode](#neutrinode) :tv:
* [Orbitones](#orbitones) :tv:
* [Photron](#photron)
* [PhotronPanel](#photron-panel)
* [PolyrhythmClock](#polyrhythm-clock) :tv:
* [QubitCrusher](#qubit-crusher)
* [RandGates](#randgates) :tv:
* [RandRoute](#randroute)
* [StochSeq](#stochseq) :tv:
* [StochSeq4](#stochseq4) :tv:
* [StochSeq4X](#stochseq4x)
* [StochSeqGrid](#stochseqgrid)
* [Talea](#talea) :tv:

## Video playlists:

* [All tutorials](https://youtube.com/playlist?list=PLqgOVd1WHf5vFl7vLlk3UglMzuUWLnZng)
* [VCV Rack patches that use Sha#BANG! modules](https://youtube.com/playlist?list=PLqgOVd1WHf5t23rEmbKc0bABRZTvfx69n)

---

### Collider

![Collider](/docs/Collider.png)

*A physical model of various of shakers and wind chimes (maracas, sleigh bells, bamboo chimes, metallic chimes, etc).*

Watch the tutorial:

[![Collider_video](docs/Collider-video.png)](https://youtu.be/nC_NozyveTc "Collider tutorial")

##### RIGHT-CLICK MENU:
- Polyphony.
##### BUTTON:
- `SHAKE` shakes the particles. Hold down to continuously shake.
##### INPUTS:
- `SHAKE` gate controls the shake.
- `V/OCT` center frequency of the particles.
- `SPREAD` control voltage determines the amount of spread from the center frequency.
- `VEL` control voltage for the initial shake energy.
- `PARTICLES` control voltage for the number of particles. More particles = faster decay in system energy, less particles = slower decay.
##### KNOBS:
- `C` center frequency of the particles in Hz.
- `SPREAD` sets the amount of spread from the center frequency.
- `RND` sets the amount of random frequencydeviation on each particle collision.
- `PARTICLES` sets the number of particles. More particles = faster decay in system energy, less particles = slower decay.
##### OUTPUTS:
- `V/OCT` outputs ±5 volts.
- `GATE` outputs pulses.
- `VEL` outputs velocity of entire system.

---

### Cosmosis

![Cosmosis](/docs/Cosmosis.png)

*A continuous sequencer based on constellations where time can move left->right, right->left, up->down, and down->up.*

Click anywhere to add/drag new stars. To remove stars click/drag them out of the dark display area.

Watch the tutorial:

[![Cosmosis_video](docs/Cosmosis-video.png)](https://youtu.be/isZ22FNc9eI "Cosmosis tutorial")

##### RIGHT-CLICK MENU:
- Polyphony.
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

Watch the tutorial:

[![Neutrinode_video](docs/Neutrinode-video.png)](https://youtu.be/WVPZnpNk_UE "Neutrinode tutorial")

##### RIGHT-CLICK MENU:
- Continuous play or one-shot mode (useful for synchronization). These options only affect the `PLAY` button/input.
- Collisions are turned on or off for the nodes.
- Polyphony.
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

![Orbitones](/docs/Orbitones.gif)

*Physics-based animated LFO with 4 independent attractors.*

Click on the attractors to move position them where you want. Click anywhere else to add particles.

Watch the tutorial:

[![Orbitones_video](docs/Orbitones-video.png)](https://youtu.be/x7X0--AugRU "Orbitones tutorial")

##### RIGHT-CLICK MENU:
- Polyphony.
- Particle trails:
  - off
  - white
  - red/blue shift (based on velocity of particles)
- Particle boundaries:
  - on (particles are bound by the display area and will bounce off edges)
  - off (particles are able to leave the display area)
##### BUTTONS:
- `RMV` removes one particle.
- `CLR` clears all particles.
- `MOVE` trigger turns on/off the random movement of attractors.
##### KNOBS:
- `ON` turns on/off individual attractors.
- `G` scales the individual attractors' gravity.
- `G` (big knob) main gravity control for all attractors.
- `OFFSET` offsets the voltage output.
##### INPUTS:
- `MOVE` trigger turns on/off the random movement of attractors.
- `G` takes a CV using this formula: `G` * 2<sup>V</sup>.
##### OUTPUTS:
- `MONO OUTS`:
  - `AVG` average x/y voltage of all particles.
  - `MAX` maximum x/y voltage of all particles.
  - `MIN` minimum x/y voltage of all particles.
- `POLY OUTS`:
  - `X Y` positions of particles correspond to ±5 volts.
  - `-X -Y` opposite positions of particles correspond to ±5 volts.
  - `velX velY` velocities of particles correspond to ±5 volts.

---

### Photron

![Photron](/docs/Photron.png)

*An animated visualizer with inputs. Uses color flocking based on the Craig Reynolds boids flocking algorithm.*

##### RIGHT-CLICK MENU:
- Processing rate (for those with slower CPUs). Keep in mind, if you slow the processing rate down, it'll help your CPU but the animation will also slow down.
- Lissajous mode on or off.
##### BUTTONS:
- Waveform mode: lines, blocks, or off.
- Background mode: color, black & white, or black.
##### KNOBS:
- Purple knobs adjust the X and Y offset of the waveform(s).
- Blue knobs adjust the X and Y scaling of the waveform(s).
##### INPUTS:
- TOP 4 inputs (zero volts is default values):
  - CV controls the separation of colors between adjacent blocks. Higher voltage = more separation.
  - CV controls the alignment of block colors which is their rate of change. Higher voltage = more alignment and thus their color velocities will become the same.
  - CV controls the cohesion of block colors which is converging on a single color. Higher voltage = more cohesion and thus their colors will quickly become the same and Photron will look like one color.
  - V/OCT controls the target color for Photron. Blocks will drift towards the target color and hover around it. See the image below for corresponding notes & colors:

![notes_spectrum](/docs/Notes_Spectrum.png)
- MIDDLE 2 inputs are the X and Y waveforms.

- BOTTOM 3 inputs are triggers:
  - Waveform mode: lines, blocks, or off.
  - Background mode: color, black & white, or black.
  - invert colors.

---

### Photron Panel

![PhotronPanel](/docs/PhotronPanel.png)

*An animated panel visualizer. Small panel version of Photron.*

- `Initialize` will set the colors in a quadrant of Purple, Blue, Aqua, and Red.
- `Randomize` will randomize all colors.

##### RIGHT-CLICK MENU:
- `Processing rate` (for those with slower CPUs). Keep in mind, if you slow the processing rate down, it'll help your CPU but the animation will also slow down.
- `Mode` color or black & white.

---

### Polyrhythm Clock

![PolyrhythmClock](/docs/PolyrhythmClock.png)

*A clock featuring three layers of embedded rhythms. It's easy to multiply and/or divide the clock with virtually any rhythm you want using this clock.*

Watch the tutorial:

[![PolyrhythmClock_video](docs/PolyClockRandGates-video.png)](https://youtu.be/v-MF6ziY_bI "Polyrhythm Clock & Rand Gates tutorial")

##### RIGHT-CLICK MENU:
- External Clock Mode:
  - `CV` controls bpm (beats per minute) based on the input voltage using this formula: 120 * 2<sup>V</sup>.
  - `2, 4, 8, 12, 24` `PPQN` controls bpm based on the number pulses per quarter note.
- If the mode is set to any of the `PPQN` modes, the clock will turn on automatically when it receives a pulse. It will also turn off automatically after it times out from not receiving any more pulses.
##### INPUT:
- `RST` resets the clock phases.
- `EXT` is an external clock to control the PolyrhythmClock determined by the External Clock Mode.
- 0v-2v map to 0-24 for the inputs under each knob.
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

Watch the tutorial:

[![RandGates_video](docs/PolyClockRandGates-video.png)](https://youtu.be/v-MF6ziY_bI "Polyrhythm Clock & Rand Gates tutorial")

##### INPUTS:
- `TRG` randomizes the output
- `INS` (purple, blue, aqua, red) are any type of input, i.e. gates or ±5 volts.
##### KNOB:
- Weight knob: Gives probability control of the probability to the chosen input. All the way to the right is uniform randomness.
- Weight probability knob: controls the probability of the chosen input.
##### OUTPUT:
- `OUT` outputs either the randomly chosen input as either a pulse or ±5 volts.

---

### RandRoute

![RandRoute](/docs/RandRoute.png)

*Randomly routes one inputs to 4 possible outputs.*

##### INPUTS:
- `TRG` randomizes the output
- `IN` any type of input, i.e. gates or ±5 volts.
##### KNOB:
- Weight knob: Gives probability control of the probability to the chosen output. All the way to the right is uniform randomness.
- Weight probability knob: controls the probability of the chosen output.
##### OUTPUT:
- `OUTS` (purple, blue, aqua, red) randomly chosen to output the input as either a pulse or ±5 volts. If no cable is connected to `TRG` then the `IN` will act as the trigger and output acts like a multinoulli gate only outputting 10 volts or 0 volts.

---

### StochSeq

![StochSeq](/docs/StochSeq.png)

*A sequencer that uses stochastic (probabilistic) patterns. The outputs can be used as gates (triggered based on probability) or as ±5 volts (probability is converted to voltage).*

Click and/or drag to draw your own patterns!

Watch the tutorial:

[![StochSeq_video](docs/StochSeq-video.png)](https://youtu.be/QtNtazAv73M "Stochastic Sequencer tutorial")

##### RIGHT-CLICK MENU:
- Show or hide slider percentages.
- Enable keyboard shortcuts.
##### KEYBOARD SHORTCUTS:
- `Ctrl+Left` shifts sliders to the left.
- `Ctrl+Right` shifts sliders to the right.
- `Ctrl+Up` shifts sliders up by 5%.
- `Ctrl+Down` shifts sliders down by 5%.
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
- `INV` outputs invert of `V/OCT`.
- `GATES` outputs correspond to each position in the sequence.
- `NOT` outputs only when `GATE` isn't triggered.

---

### StochSeq4

![StochSeq4](/docs/StochSeq4.png)

*A sequencer that uses four independent stochastic (probabilistic) patterns. The outputs can be used as gates (triggered based on probability) or as ±5 volts (probability is converted to voltage).*

Click and/or drag to draw your own patterns!

Watch the tutorial:

[![StochSeq4_video](docs/StochSeq4-video.png)](https://youtu.be/LBL_VYe_stU "Stoch Seq 4 tutorial")

##### RIGHT-CLICK MENU:
- `MCLK` override: if `MCLK` has a connected cable then it will disable all individual clocks.
- Show or hide slider percentages.
- Enable keyboard shortcuts.
##### KEYBOARD SHORTCUTS:
- `Ctrl+C` copies focused pattern and length.
- `Ctrl+V` pastes the copied pattern and length to the focused one.
- `Ctrl+Enter` focuses and highlights a single pattern.
- `Ctrl+Left` shifts focused sliders to the left.
- `Ctrl+Right` shifts focused sliders to the right.
- `Ctrl+Up` shifts focused sliders up by 5%.
- `Ctrl+Down` shifts focused sliders down by 5%.
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
- Resets sequences to beginning of timeline.
- `RND` randomizes all probabilities.
- `INV` inverts all probabilities.
- `DIM` cuts the current pattern in half and repeats. Keep clicking this button to continue to diminish the pattern.
##### OUTPUTS:
- `GATE` outputs a pulse based on the probability of the current individual sequence position. (i.e. a slider at 50% will only trigger a pulse half of the time)
- `NOT` outputs only when `GATE` isn't triggered.
- `V/OCT` outputs pitch based on the slider position and `SPREAD` knob, regardless of probability of the event.
- `INV` outputs invert of `V/OCT`.
- `OR` outputs pulse when at least one of the gates is on.
- `XOR` outputs pulse when ONLY one gate is on.

---

### StochSeq4X

![StochSeq4X](/docs/StochSeq4X.png)

*An expander for the StochSeq4. This module must be adjacent to the right side of StochSeq4.*

##### KNOB:
- The main knob at the top controls which sequence you want to expand from StochSeq4 (Purple, Blue, Aqua, or Red). Additionally, the final option is to have all four columns output the first 8 steps from all sequences. If the `All` option is selected then each column of the outputs go from 1-8.
##### BUTTONS:
- The buttons under each column toggle whether they are gates or not gates.
##### OUTPUTS:
- All 32 (or first 8) gate outputs from StochSeq4.

---

### StochSeqGrid

![StochSeqGrid](/docs/StochSeqGrid.png)

*A rhythmic sequencer that uses four independent stochastic (probabilistic) patterns. Each cell contains rhythmic subdivisions that are triggered based on probability.*

Inspired by JW-Modules GridSeq!

Watch the tutorial (coming soon):

<!-- [![StochSeqGrid_video](docs/StochSeqGrid-video.png)](https://youtu.be/LBL_VYe_stU "Stoch Seq Grid tutorial") -->

##### RIGHT-CLICK MENU:
- Gate mode: gates or triggers.
- CV mode: independent or sample & hold (based on whether the cell was triggered or not).
- Volt range: +1V, +2V, ±5V, +10V (only effects the `CV` output)
- Mouse drag: horizontal or vertical controls the increase/decrease of subdivisions within each cell.
- External Clock Mode:
  - `CV` controls bpm (beats per minute) based on the input voltage using this formula: 120 * 2<sup>V</sup>.
  - `2, 4, 8, 12, 24` `PPQN` controls bpm based on the number pulses per quarter note.
- If the mode is set to any of the `PPQN` modes, the clock will turn on automatically when it receives a pulse. It will also turn off automatically after it times out from not receiving any more pulses.
- Display: blooms or circles (doesn't affect the module other than visual aesthetic).
##### MOUSE/KEYBOARD CONTROLS:
- `Click` a cell to increase subdivisions.
- `Shift+Click` a cell to double its subdivisions (up to 16).
- `Click+Drag` in a cell to increase/decrease subdivisions.
- `Ctrl+Click` on a subdivision to toggle.
- `Ctrl+Click` off of a subdivision to toggle all of them on in current cell.
- `Ctrl+Click+Drag` on multiple subdivisions to toggle.
##### INPUTS:
- `RST` resets sequences to beginning of timeline.
- `EXT` is an external clock to control the StochSeqGrid determined by the External Clock Mode.
##### PATHS:
- `length` length of the individual sequences.
- `path` toggles the type of path
  - `default` will traverse the grid based on the `length` small color indicators arrows just outside the grid display.
  - `random` will randomly pick a cell based on the `length` range.
  - `random walk` will ignore the `length` and only pick cells that are adjacent to the current cell the sequencer is in.
##### RATES:
- left-side knobs control the numerator part of the fraction (or ratio) of the corresponding sequence based on the global tempo.
- right-side knobs control the denominator part of the fraction (or ratio) of the corresponding sequence based on the global tempo.
- buttons toggle the corresponding sequence on/off.
##### MAIN BUTTONS & KNOBS:
- `Run` toggle the sequencer on/off.
- `RST` resets sequences to beginning of timeline.
- `Tempo` controls the global tempo of the StochSeqGrid.
##### CELL KNOBS:
- `Cell Probability` controls the overall probability of whether or not the cell will happen.
- `CV / Rhythm Probability` controls the probability of whether or not the subdivision will happen. If the cell is triggered but the subdivision isn't, the output will be a single rhythmic gate/trigger. This knob also controls the volts for the `CV` outputs in relation to the `Volt range`.
##### OUTPUTS:
- `GATES` outputs a pulse based on the probability of the current individual sequence position. (i.e. a cell at 50% will only trigger a pulse half of the time)
- `CV` outputs voltage based on the `Rhythmic Probability` position and `Volt Range`.

---

### Talea

![Talea](/docs/Talea.png)

*An arpeggiator with polyrhythmic capabilities dependent upon note intervals.*

Watch the tutorial:

[![Talea_video](docs/Talea-video.png)](https://youtu.be/SVNwWxELw4U "Talea tutorial")

##### RIGHT-CLICK MENU:
- External Clock Mode:
  - `CV` controls bpm (beats per minute) based on the input voltage using this formula: 120 * 2<sup>V</sup>.
  - `2, 4, 8, 12, 24` `PPQN` controls bpm based on the number pulses per quarter note.
  - If the mode is set to any of the `PPQN` modes, the clock will turn on automatically when it receives a pulse. It will also turn off automatically after it times out from not receiving any more pulses.
- Polyrhythm Mode:
  - `Fixed` means each note is fixed and centered around middle C (C4, volts = 0.0). This note will take the current tempo of the BPM knob and all other notes are a ratio based on this note/tempo.
  - `Movable` means that the first note played will take the current tempo of the BPM knob and all other notes are a ratio based on this first note/tempo.
##### INPUT:
- `EXT` is an external clock to control the Talea BPM determined by the External Clock Mode.
- `V/OCT` takes input voltage.
- `GATE` input gates when note is held.
##### BUTTON:
- on or off
- `HOLD` will hold pattern. Is overriden when you release all notes and start a new note.
- `OCT` will add octaves to the pattern:
  - No light = 1 octave
  - Purple light = 2 octaves
  - Blue light = 3 octaves
  - Aqua light = 4 octaves
  - Red light = 5 octaves
- `POLYRHYTHM` will turn on or off the polyrhythm arpeggiator. If off, Talea acts like a conventional arpeggiator.
##### KNOBS:
- large main knob at the top controls the bpm (beats per minute).
- `GATE` knob control the percentage amount the gate is on.
- the pattern mode controls the order of notes when `POLYRHYTHM` is turned off. These are:
  - `↑` ascending order
  - `↓` descending order
  - `2x` each note plays twice in ascending order
  - `⚡︎` in order of which they were played
  - `R` random
##### OUTPUTS:
- `V/OCT` outputs pitch.
- `GATE` outputs gates determined by arpeggiator rhythms.

##### TUNINGS AND RHYTHMS
When `POLYRHYTHM` is turned on, the arpeggiator will repeat notes at a tempo based on a specific tuning. For example, the note A440 vibrates at 440 Hz and an octave higher the note vibrates at 880 Hz. This is a 2:1 ratio, meaning one vibrates twice as fast as the other. Rhythms can work like this as well, one rhythm can repeat at a tempo twice as fast as another. Once we start using other intervals that occur in a scale, we can create some more complex rhythms. However, using equal temperament tuning (which is standard tuning for the overwhelming majority of music and instruments in the Western Hemisphere) divides the octave into 12 equal steps which gives us non-integer ratios. So if you play an A and an E (perfect 5th) in equal temperament, you get a rhythmic ratio of 1 : 1.4983070768766815, or 440Hz : 659.2551138257398Hz, YUCK! So to simplify this, I use the natural harmonic series for the ratios. They make nice integer ratios and are a naturally occuring phenomenon. The table below shows the ratios that are use for each interval.

No. of steps | Musical interval | Ratio
------------ | ---------------- | -----
0 | unison | 1:1
1 | minor 2<sup>nd</sup> | 16:15
2 | major 2<sup>nd</sup> | 9:8
3 | minor 3<sup>rd</sup> | 6:5
4 | major 3<sup>rd</sup> | 5:4
5 | perfect 4<sup>th</sup> | 4:3
6 | augmented 4<sup>th</sup>/diminished 5<sup>th</sup> | 7:5
7 | perfect 5<sup>th</sup> | 3:2
8 | minor 6<sup>th</sup> | 8:5
9 | major 6<sup>th</sup> | 5:3
10 | minor 7<sup>th</sup> | 9:5
11 | major 7<sup>th</sup> | 15:8
12 | octave | 2:1
13 | minor 9<sup>th</sup> | 32:15 (16:15 * 2)
14 | major 9<sup>th</sup> | 18:8 (9:8 * 2)
15 | minor 10<sup>th</sup> | 12:5 (6:5 * 2)
...

---
