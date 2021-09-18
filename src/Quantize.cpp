#include "rack.hpp"

// code modified from https://github.com/jeremywen/JW-Modules

struct Quantize {
    float SCALE_MAJOR[8] = {0, 2, 4, 5, 7, 9, 11, 12};
    float SCALE_MINOR[8] = {0, 2, 3, 5, 7, 8, 10, 12};
    float SCALE_DORIAN[8] = {0, 2, 3, 5, 7, 9, 10, 12};
    float SCALE_PHRYGIAN[8] = {0, 1, 3, 5, 7, 8, 10, 12};
    float SCALE_LYDIAN[8] = {0, 2, 4, 6, 7, 9, 11, 12};
    float SCALE_MIXOLYDIAN[8] = {0, 2, 4, 5, 7, 9, 10, 12};
    float SCALE_LOCRIAN[8] = {0, 1, 3, 5, 6, 8, 10, 12};
    float SCALE_MAJ_PENTATONIC[6] = {0, 2, 4, 7, 9, 12};
    float SCALE_MIN_PENTATONIC[6] = {0, 3, 5, 7, 10, 12};
    float SCALE_OCTATONIC[9] = {0, 2, 3, 5, 6, 8, 9, 11, 12};
    float SCALE_WHOLE_TONE[7] = {0, 2, 4, 6, 8, 10, 12};
    float SCALE_ACOUSTIC[8] = {0, 2, 4, 6, 7, 9, 10, 12};
    float SCALE_BLUES[8] = {0, 2, 3, 5, 6, 7, 10, 12};
    float SCALE_MAJ_MAJ_7[4] = {0, 4, 7, 11};
    float SCALE_MAJ_MIN_7[4] = {0, 4, 7, 10};
    float SCALE_MIN_MIN_7[4] = {0, 3, 7, 10};
    float SCALE_HALF_DIM_7[4] = {0, 3, 6, 10};
    float SCALE_FULLY_DIM_7[4] = {0, 3, 6, 9};
    float SCALE_AUG_7[4] = {0, 4, 8, 10};
    float SCALE_AUG_MAJ_7[4] = {0, 4, 8, 11};
    float SCALE_MIN_MAJ_7[4] = {0, 3, 7, 11};
    float SCALE_MAJ_ADD_9[4] = {0, 4, 7, 14};
    float SCALE_MAJ_MIN_SUS_4[4] = {0, 5, 7, 10};
    float SCALE_MIN_ADD_6[4] = {0, 3, 7, 9};
    float SCALE_MESSIAEN3[10] = {0, 2, 3, 4, 6, 7, 8, 10, 11, 12};
    float SCALE_MESSIAEN4[9] = {0, 1, 2, 5, 6, 7, 8, 11, 12};
    float SCALE_MESSIAEN5[7] = {0, 1, 5, 6, 7, 11, 12};
    float SCALE_MESSIAEN6[9] = {0, 2, 4, 5, 6, 8, 10, 11, 12};
    float SCALE_MESSIAEN7[11] = {0, 1, 2, 3, 5, 6, 7, 8, 9, 11, 12};
    // float SCALE_EQUAL_MICROTONAL[25] = {0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6, 6.5, 7, 7.5, 8, 8.5, 9, 9.5, 10, 10.5, 11, 11.5, 12};
    float SCALE_PYTHAGOREAN[12] = {0, 0.902252, 2.0391, 2.94135, 4.0782, 4.98045, 6.1173, 7.01955, 7.9218, 9.05865, 9.9609, 11.0977}; // 0, 0.902252, 2.0391, 2.94135, 4.0782, 4.98045, (gb)5.8827, (f#)6.1173, 7.01955, 7.9218, 9.05865, 9.9609, 11.0977
    float SCALE_DIATONIC_JUST[8] = {0, 2.0391, 3.86314, 4.98045, 7.01955, 8.84359, 10.8827, 12};
    // TODO: Webern? Schoenberg? Berg? anything else?

    enum NoteNames {
        NOTE_C,
        NOTE_C_SHARP,
        NOTE_D,
        NOTE_E_FLAT,
        NOTE_E,
        NOTE_F,
        NOTE_F_SHARP,
        NOTE_G,
        NOTE_A_FLAT,
        NOTE_A,
        NOTE_B_FLAT,
        NOTE_B,
        NUM_OF_NOTES
    };

    enum ScaleNames {
        MAJOR,
        MINOR,
        DORIAN,
        PHRYGIAN,
        LYDIAN,
        MIXOLYDIAN,
        LOCRIAN,
        MAJ_PENTATONIC,
        MIN_PENTATONIC,
        OCTATONIC,
        WHOLE_TONE,
        ACOUSTIC,
        BLUES,
        MAJ_MAJ_7,
        MAJ_MIN_7,
        MIN_MIN_7,
        HALF_DIM_7,
        FULLY_DIM_7,
        MIN_MAJ_7,
        AUG_7,
        AUG_MAJ_7,
        MAJ_ADD_9,
        MAJ_MIN_SUS_4,
        MIN_ADD_6,
        MESSIAEN3,
        MESSIAEN4,
        MESSIAEN5,
        MESSIAEN6,
        MESSIAEN7,
        PYTHAGOREAN,
        DIATONIC_JUST,
        NUM_OF_SCALES
    };

    float quantizeRawVoltage(float voltsIn, int root, int scale) {
        float *chosenScale;
        int scaleLength = 0;
        switch (scale) {
            case MAJOR:             chosenScale = SCALE_MAJOR;          scaleLength = LENGTHOF(SCALE_MAJOR);            break;
            case MINOR:             chosenScale = SCALE_MINOR;          scaleLength = LENGTHOF(SCALE_MINOR);            break;
            case DORIAN:            chosenScale = SCALE_DORIAN;         scaleLength = LENGTHOF(SCALE_DORIAN);           break;
            case PHRYGIAN:          chosenScale = SCALE_PHRYGIAN;       scaleLength = LENGTHOF(SCALE_PHRYGIAN);         break;
            case LYDIAN:            chosenScale = SCALE_LYDIAN;         scaleLength = LENGTHOF(SCALE_LYDIAN);           break;
            case MIXOLYDIAN:        chosenScale = SCALE_MIXOLYDIAN;     scaleLength = LENGTHOF(SCALE_MIXOLYDIAN);       break;
            case LOCRIAN:           chosenScale = SCALE_LOCRIAN;        scaleLength = LENGTHOF(SCALE_LOCRIAN);          break;
            case MAJ_PENTATONIC:    chosenScale = SCALE_MAJ_PENTATONIC; scaleLength = LENGTHOF(SCALE_MAJ_PENTATONIC);   break;
            case MIN_PENTATONIC:    chosenScale = SCALE_MIN_PENTATONIC; scaleLength = LENGTHOF(SCALE_MIN_PENTATONIC);   break;
            case OCTATONIC:         chosenScale = SCALE_OCTATONIC;      scaleLength = LENGTHOF(SCALE_OCTATONIC);        break;
            case WHOLE_TONE:        chosenScale = SCALE_WHOLE_TONE;     scaleLength = LENGTHOF(SCALE_WHOLE_TONE);       break;
            case ACOUSTIC:          chosenScale = SCALE_ACOUSTIC;       scaleLength = LENGTHOF(SCALE_ACOUSTIC);         break;
            case BLUES:             chosenScale = SCALE_BLUES;          scaleLength = LENGTHOF(SCALE_BLUES);            break;
            case MAJ_MAJ_7:         chosenScale = SCALE_MAJ_MAJ_7;      scaleLength = LENGTHOF(SCALE_MAJ_MAJ_7);        break;
            case MAJ_MIN_7:         chosenScale = SCALE_MAJ_MIN_7;      scaleLength = LENGTHOF(SCALE_MAJ_MIN_7);        break;
            case MIN_MIN_7:         chosenScale = SCALE_MIN_MIN_7;      scaleLength = LENGTHOF(SCALE_MIN_MIN_7);        break;
            case HALF_DIM_7:        chosenScale = SCALE_HALF_DIM_7;     scaleLength = LENGTHOF(SCALE_HALF_DIM_7);       break;
            case FULLY_DIM_7:       chosenScale = SCALE_FULLY_DIM_7;    scaleLength = LENGTHOF(SCALE_FULLY_DIM_7);      break;
            case MIN_MAJ_7:         chosenScale = SCALE_MIN_MAJ_7;      scaleLength = LENGTHOF(SCALE_MIN_MAJ_7);        break;
            case AUG_7:             chosenScale = SCALE_AUG_7;          scaleLength = LENGTHOF(SCALE_AUG_7);            break;
            case AUG_MAJ_7:         chosenScale = SCALE_AUG_MAJ_7;      scaleLength = LENGTHOF(SCALE_AUG_MAJ_7);        break;
            case MAJ_ADD_9:         chosenScale = SCALE_MAJ_ADD_9;      scaleLength = LENGTHOF(SCALE_MAJ_ADD_9);        break;
            case MAJ_MIN_SUS_4:     chosenScale = SCALE_MAJ_MIN_SUS_4;  scaleLength = LENGTHOF(SCALE_MAJ_MIN_SUS_4);    break;
            case MIN_ADD_6:         chosenScale = SCALE_MIN_ADD_6;      scaleLength = LENGTHOF(SCALE_MIN_ADD_6);        break;
            case MESSIAEN3:         chosenScale = SCALE_MESSIAEN3;      scaleLength = LENGTHOF(SCALE_MESSIAEN3);        break;
            case MESSIAEN4:         chosenScale = SCALE_MESSIAEN4;      scaleLength = LENGTHOF(SCALE_MESSIAEN4);        break;
            case MESSIAEN5:         chosenScale = SCALE_MESSIAEN5;      scaleLength = LENGTHOF(SCALE_MESSIAEN5);        break;
            case MESSIAEN6:         chosenScale = SCALE_MESSIAEN6;      scaleLength = LENGTHOF(SCALE_MESSIAEN6);        break;
            case MESSIAEN7:         chosenScale = SCALE_MESSIAEN7;      scaleLength = LENGTHOF(SCALE_MESSIAEN7);        break;
            case PYTHAGOREAN:       chosenScale = SCALE_PYTHAGOREAN;    scaleLength = LENGTHOF(SCALE_PYTHAGOREAN);      break;
            case DIATONIC_JUST:     chosenScale = SCALE_DIATONIC_JUST;  scaleLength = LENGTHOF(SCALE_DIATONIC_JUST);    break;
            default: return voltsIn;
        }

        int octave = static_cast<int>(floorf(voltsIn));
        voltsIn = voltsIn - octave;
        float distanceToNote = 10.0;
        int chosenNote = 0;
        for (int i = 0; i < scaleLength; i++) {
            float dist = fabs(voltsIn - (chosenScale[i] / 12.0));
            if (dist < distanceToNote) {
                distanceToNote = dist;
                chosenNote = i;
            } else {
                break;
            }

        }

        float quantizedVoltage = octave + (chosenScale[chosenNote] / 12.0) + (root / 12.0);
        return quantizedVoltage;
    }

    std::string noteName(int note) {
        switch (note) {
            case NOTE_C:        return "C";
            case NOTE_C_SHARP:  return "C#";
            case NOTE_D:        return "D";
            case NOTE_E_FLAT:   return "Eb";
            case NOTE_E:        return "E";
            case NOTE_F:        return "F";
            case NOTE_F_SHARP:  return "F#";
            case NOTE_G:        return "G";
            case NOTE_A_FLAT:   return "Ab";
            case NOTE_A:        return "A";
            case NOTE_B_FLAT:   return "Bb";
            case NOTE_B:        return "B";
            default:            return "";
        }
    }

    std::string scaleName(int scale) {
        switch (scale) {
            case MAJOR:           return "Major";
            case MINOR:           return "Minor";
            case DORIAN:          return "Dorian";
            case PHRYGIAN:        return "Phrygian";
            case LYDIAN:          return "Lydian";
            case MIXOLYDIAN:      return "Mixolydian";
            case LOCRIAN:         return "Locrian";
            case MAJ_PENTATONIC:  return "Maj Pentatonic";
            case MIN_PENTATONIC:  return "Min Pentatonic";
            case OCTATONIC:       return "Octatonic";
            case WHOLE_TONE:      return "Whole Tone";
            case ACOUSTIC:        return "Acoustic";
            case BLUES:           return "Blues";
            case MAJ_MAJ_7:       return "MM7";
            case MAJ_MIN_7:       return "Mm7";
            case MIN_MIN_7:       return "mm7";
            case HALF_DIM_7:      return "Half dim7";
            case FULLY_DIM_7:     return "Diminished 7";
            case MIN_MAJ_7:       return "mM7";
            case AUG_7:           return "Augmented 7";
            case AUG_MAJ_7:       return "Aug Maj 7";
            case MAJ_ADD_9:       return "Maj add9";
            case MAJ_MIN_SUS_4:   return "MmSus4";
            case MIN_ADD_6:       return "Min add6";
            case MESSIAEN3:       return "Messiaen 3";
            case MESSIAEN4:       return "Messiaen 4";
            case MESSIAEN5:       return "Messiaen 5";
            case MESSIAEN6:       return "Messiaen 6";
            case MESSIAEN7:       return "Messiaen 7";
            case PYTHAGOREAN:     return "Pythagorean";
            case DIATONIC_JUST:   return "Diatonic Just";
            default:              return "Raw Volts";
        }
    }
};
