#include "rack.hpp"

struct Quantize {
    int MAJOR[8] = {0, 2, 4, 5, 7, 9, 11, 12};
    int MINOR[8] = {0, 2, 3, 5, 7, 8, 10, 12};
    int DORIAN[8] = {0, 2, 3, 5, 7, 9, 10, 12};
    int PHRYGIAN[8] = {0, 1, 3, 5, 7, 8, 10, 12};
    int LYDIAN[8] = {0, 2, 4, 6, 7, 9, 11, 12};
    int MIXOLYDIAN[8] = {0, 2, 4, 5, 7, 9, 10, 12};
    int LOCRIAN[8] = {0, 1, 3, 5, 6, 8, 10, 12};
    int MAJ_PENTATONIC[6] = {0, 2, 4, 7, 9, 12};
    int MIN_PENTATONIC[6] = {0, 3, 5, 7, 10, 12};
    int OCTATONIC[9] = {0, 2, 3, 5, 6, 8, 9, 11, 12};
    int WHOLE_TONE[7] = {0, 2, 4, 6, 8, 10, 12};
    int BLUES[8] = {0, 2, 3, 5, 6, 7, 10, 12};
    int MAJ_MAJ_7[4] = {0, 4, 7, 11};
    int MAJ_MIN_7[4] = {0, 4, 7, 10};
    int MIN_MIN_7[4] = {0, 3, 7, 10};
    int MAJ_ADD_9[4] = {0, 4, 7, 14};
    int MAJ_MIN_SUS_4[4] = {0, 5, 7, 10};
    int MIN_ADD_6[4] = {0, 3, 7, 9};
    // TODO: add messiaen modes, Webern? Schoenberg? Berg? anything else?
};
