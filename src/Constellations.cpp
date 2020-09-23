#include "rack.hpp"

// using namespace rack;

struct Point {
    // Vec pos;
    float x;
    float y;
    float r;

    // Point() {}

    void setPoint(float _x, float _y, float _r) {
        x = _x;
        y = _y;
        r = _r;
    }
};

struct Constellations {
    Point ANDROMEDA[9];
    Constellations() {
        ANDROMEDA[0].setPoint(353.3, 332.6, 12);
        ANDROMEDA[1].setPoint(107.1, 44.6, 10);
        ANDROMEDA[2].setPoint(123.4, 68.8, 7.5);
        ANDROMEDA[3].setPoint(255.8, 309.3, 11);
        ANDROMEDA[4].setPoint(259.5, 268.2, 10);
        ANDROMEDA[5].setPoint(164.1, 240.1, 12);
        ANDROMEDA[6].setPoint(202, 199.7, 7.2);
        ANDROMEDA[7].setPoint(141.4, 91.4, 7.4);
        ANDROMEDA[8].setPoint(25, 122.1, 12);
    }
    // Star bar(1.2, 5, 8);

    // Star ANDROMEDA[2] = {a1, a2};
    // Vec AQUARIUS[] = {};
    // CENTAURUS
    // DRACO
    // GEMINI
    // HERCULES
    // LEO
    // ORION
    // PEGASUS
    // SAGITTARIUS
};