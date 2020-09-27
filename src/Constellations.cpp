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
    Point CONSTELLATION_ANDROMEDA[9];
    Point CONSTELLATION_AQUARIUS[14];
    
    Constellations() {
        CONSTELLATION_ANDROMEDA[0].setPoint(353.3, 332.6, 12);
        CONSTELLATION_ANDROMEDA[1].setPoint(107.1, 44.6, 10);
        CONSTELLATION_ANDROMEDA[2].setPoint(123.4, 68.8, 7.5);
        CONSTELLATION_ANDROMEDA[3].setPoint(255.8, 309.3, 11);
        CONSTELLATION_ANDROMEDA[4].setPoint(259.5, 268.2, 10);
        CONSTELLATION_ANDROMEDA[5].setPoint(164.1, 240.1, 12);
        CONSTELLATION_ANDROMEDA[6].setPoint(202, 199.7, 7.2);
        CONSTELLATION_ANDROMEDA[7].setPoint(141.4, 91.4, 7.4);
        CONSTELLATION_ANDROMEDA[8].setPoint(25, 122.1, 12);

        CONSTELLATION_AQUARIUS[0].setPoint(111.3, 92, 7.2);
        CONSTELLATION_AQUARIUS[1].setPoint(360.3, 186.6, 5.4);
        CONSTELLATION_AQUARIUS[2].setPoint(125, 91.2, 7);
        CONSTELLATION_AQUARIUS[3].setPoint(142.6, 104.7, 7.7);
        CONSTELLATION_AQUARIUS[4].setPoint(176.9, 93.6, 7);
        CONSTELLATION_AQUARIUS[5].setPoint(259.2, 143.2, 8.3);
        CONSTELLATION_AQUARIUS[6].setPoint(153.9, 162.1, 5.1);
        CONSTELLATION_AQUARIUS[7].setPoint(167.7, 196.9, 4.6);
        CONSTELLATION_AQUARIUS[8].setPoint(176.9, 218.1, 7);
        CONSTELLATION_AQUARIUS[9].setPoint(37.2, 287.5, 7.9);
        CONSTELLATION_AQUARIUS[10].setPoint(68.3, 235.9, 7.1);
        CONSTELLATION_AQUARIUS[11].setPoint(79.9, 216.2, 6.2);
        CONSTELLATION_AQUARIUS[12].setPoint(73.5, 160.7, 7.9);
        CONSTELLATION_AQUARIUS[13].setPoint(20, 146.1, 8.3);
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

    enum ConstellationNames {
        ANDROMEDA,
        AQUARIUS,
        NUM_OF_CONSTELLATIONS
    };

    std::string constellationName(int constellation) {
        switch(constellation) {
            case ANDROMEDA:             return "Andromeda";
            case AQUARIUS:              return "Aquarius";
            default:                    return "";
        }
    }
};