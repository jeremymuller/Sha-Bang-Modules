#include "rack.hpp"

struct Quantize {
    int SCALE_MAJOR[8] = {0, 2, 4, 5, 7, 9, 11, 12};
    int SCALE_MINOR[8] = {0, 2, 3, 5, 7, 8, 10, 12};
    int SCALE_DORIAN[8] = {0, 2, 3, 5, 7, 9, 10, 12};
    int SCALE_PHRYGIAN[8] = {0, 1, 3, 5, 7, 8, 10, 12};
    int SCALE_LYDIAN[8] = {0, 2, 4, 6, 7, 9, 11, 12};
    int SCALE_MIXOLYDIAN[8] = {0, 2, 4, 5, 7, 9, 10, 12};
    int SCALE_LOCRIAN[8] = {0, 1, 3, 5, 6, 8, 10, 12};
    int SCALE_MAJ_PENTATONIC[6] = {0, 2, 4, 7, 9, 12};
    int SCALE_MIN_PENTATONIC[6] = {0, 3, 5, 7, 10, 12};
    int SCALE_OCTATONIC[9] = {0, 2, 3, 5, 6, 8, 9, 11, 12};
    int SCALE_WHOLE_TONE[7] = {0, 2, 4, 6, 8, 10, 12};
    int SCALE_ACOUSTIC[8] = {0, 2, 4, 6, 7, 9, 10, 12};
    int SCALE_BLUES[8] = {0, 2, 3, 5, 6, 7, 10, 12};
    int SCALE_MAJ_MAJ_7[4] = {0, 4, 7, 11};
    int SCALE_MAJ_MIN_7[4] = {0, 4, 7, 10};
    int SCALE_MIN_MIN_7[4] = {0, 3, 7, 10};
    int SCALE_MAJ_ADD_9[4] = {0, 4, 7, 14};
    int SCALE_MAJ_MIN_SUS_4[4] = {0, 5, 7, 10};
    int SCALE_MIN_ADD_6[4] = {0, 3, 7, 9};
    int SCALE_MESSIAEN3[10] = {0, 2, 3, 4, 6, 7, 8, 10, 11, 12};
    int SCALE_MESSIAEN4[9] = {0, 1, 2, 5, 6, 7, 8, 11, 12};
    int SCALE_MESSIAEN5[7] = {0, 1, 5, 6, 7, 11, 12};
    int SCALE_MESSIAEN6[9] = {0, 2, 4, 5, 6, 8, 10, 11, 12};
    int SCALE_MESSIAEN7[11] = {0, 1, 2, 3, 5, 6, 7, 8, 9, 11, 12};
    // TODO: Webern? Schoenberg? Berg? anything else?

    enum NoteNames {
        // TODO
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
        MAJ_ADD_9,
        MAJ_MIN_SUS_4,
        MIN_ADD_6,
        MESSIAEN3,
        MESSIAEN4,
        MESSIAEN5,
        MESSIAEN6,
        MESSIAEN7
    };

    float quantizeRawVoltage(float voltsIn, int root, int scale) {
        // todo
        
        return 0;
    }
};
