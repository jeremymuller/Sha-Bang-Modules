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
    Point AQUARIUS[14];
    
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

        AQUARIUS[0].setPoint(111.3, 92, 7.2);
        AQUARIUS[1].setPoint(360.3, 186.6, 5.4);
        AQUARIUS[2].setPoint(125, 91.2, 7);
        AQUARIUS[3].setPoint(142.6, 104.7, 7.7);
        AQUARIUS[4].setPoint(176.9, 93.6, 7);
        AQUARIUS[5].setPoint(259.2, 143.2, 8.3);
        AQUARIUS[6].setPoint(153.9, 162.1, 5.1);
        AQUARIUS[7].setPoint(167.7, 196.9, 4.6);
        AQUARIUS[8].setPoint(176.9, 218.1, 7);
        AQUARIUS[9].setPoint(37.2, 287.5, 7.9);
        AQUARIUS[10].setPoint(68.3, 235.9, 7.1);
        AQUARIUS[11].setPoint(79.9, 216.2, 6.2);
        AQUARIUS[12].setPoint(73.5, 160.7, 7.9);
        AQUARIUS[13].setPoint(20, 146.1, 8.3);
    }

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