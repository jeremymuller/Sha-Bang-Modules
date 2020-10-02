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
    int constellationLength = 0;

    Point CONSTELLATION_ANDROMEDA[9];
    Point CONSTELLATION_AQUARIUS[14];
    Point CONSTELLATION_CENTAURUS[19];
    Point CONSTELLATION_DRACO[17];
    Point CONSTELLATION_GEMINI[10];
    Point CONSTELLATION_HERCULES[19];
    Point CONSTELLATION_LEO[9];
    Point CONSTELLATION_ORION[20];
    Point CONSTELLATION_PEGASUS[15];
    Point CONSTELLATION_SAGITTARIUS[23];

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

        CONSTELLATION_CENTAURUS[0].setPoint(113.4, 305.4, 12.0);
        CONSTELLATION_CENTAURUS[1].setPoint(151.6, 288.3, 8.5);
        CONSTELLATION_CENTAURUS[2].setPoint(313.9, 317.3, 6.1);
        CONSTELLATION_CENTAURUS[3].setPoint(309.6, 308.7, 5.0);
        CONSTELLATION_CENTAURUS[4].setPoint(308, 297, 6.1);
        CONSTELLATION_CENTAURUS[5].setPoint(357.2, 246.4, 6.7);
        CONSTELLATION_CENTAURUS[6].setPoint(301.5, 195.2, 5.0);
        CONSTELLATION_CENTAURUS[7].setPoint(295, 210.3, 6);
        CONSTELLATION_CENTAURUS[8].setPoint(272.6, 186.6, 5.0);
        CONSTELLATION_CENTAURUS[9].setPoint(255.3, 175.1, 7.7);
        CONSTELLATION_CENTAURUS[10].setPoint(172.9, 221.3, 7.6);
        CONSTELLATION_CENTAURUS[11].setPoint(21.7, 151.2, 6.4);
        CONSTELLATION_CENTAURUS[12].setPoint(63, 137.5, 7.7);
        CONSTELLATION_CENTAURUS[13].setPoint(139.3, 166.5, 7.7);
        CONSTELLATION_CENTAURUS[14].setPoint(139.3, 124.2, 5.0);
        CONSTELLATION_CENTAURUS[15].setPoint(138.9, 115, 6.5);
        CONSTELLATION_CENTAURUS[16].setPoint(168.9, 91.2, 5.1);
        CONSTELLATION_CENTAURUS[17].setPoint(187.8, 64.3, 8.3);
        CONSTELLATION_CENTAURUS[18].setPoint(101.2, 71.9, 7.7);

        CONSTELLATION_DRACO[0].setPoint(150.2, 109.3, 6.9);
        CONSTELLATION_DRACO[1].setPoint(320.3, 66.6, 6.4);
        CONSTELLATION_DRACO[2].setPoint(362.8, 72.5, 9);
        CONSTELLATION_DRACO[3].setPoint(333.3, 118.4, 5);
        CONSTELLATION_DRACO[4].setPoint(313.5, 92.9, 5);
        CONSTELLATION_DRACO[5].setPoint(194.3, 103.2, 7.8);
        CONSTELLATION_DRACO[6].setPoint(217, 132.5, 6.5);
        CONSTELLATION_DRACO[7].setPoint(256.6, 177.5, 8.2);
        CONSTELLATION_DRACO[8].setPoint(336.1, 197.9, 5);
        CONSTELLATION_DRACO[9].setPoint(344.7, 251.1, 9);
        CONSTELLATION_DRACO[10].setPoint(347.3, 292.2, 5.2);
        CONSTELLATION_DRACO[11].setPoint(310, 290.6, 6.1);
        CONSTELLATION_DRACO[12].setPoint(285.4, 261.9, 5.3);
        CONSTELLATION_DRACO[13].setPoint(264.4, 245.4, 5.5);
        CONSTELLATION_DRACO[14].setPoint(79.1, 183.5, 6.2);
        CONSTELLATION_DRACO[15].setPoint(46, 273.6, 7.8);
        CONSTELLATION_DRACO[16].setPoint(13.7, 314.2, 5.8);

        CONSTELLATION_GEMINI[0].setPoint(39.2, 254.6, 10);
        CONSTELLATION_GEMINI[1].setPoint(200, 356.2, 9.7);
        CONSTELLATION_GEMINI[2].setPoint(196.3, 325.5, 12);
        CONSTELLATION_GEMINI[3].setPoint(156.5, 305.3, 7.8);
        CONSTELLATION_GEMINI[4].setPoint(95.7, 279.9, 12);
        CONSTELLATION_GEMINI[5].setPoint(232.2, 245.6, 10.9);
        CONSTELLATION_GEMINI[6].setPoint(156.5, 174.2, 7.8);
        CONSTELLATION_GEMINI[7].setPoint(177.7, 115.9, 9.6);
        CONSTELLATION_GEMINI[8].setPoint(337.2, 65.4, 12);
        CONSTELLATION_GEMINI[9].setPoint(272, 27.1, 12);

        CONSTELLATION_HERCULES[0].setPoint(325.8, 38.9, 7.4);
        CONSTELLATION_HERCULES[1].setPoint(247.2, 174.1, 6);
        CONSTELLATION_HERCULES[2].setPoint(205.8, 185.8, 5.5);
        CONSTELLATION_HERCULES[3].setPoint(184.1, 364.8, 8.7);
        CONSTELLATION_HERCULES[4].setPoint(315.8, 300.9, 7.6);
        CONSTELLATION_HERCULES[5].setPoint(290.3, 278.6, 9.3);
        CONSTELLATION_HERCULES[6].setPoint(172.5, 252.1, 9.3);
        CONSTELLATION_HERCULES[7].setPoint(137.8, 240.5, 6);
        CONSTELLATION_HERCULES[8].setPoint(97.9, 222.2, 8.7);
        CONSTELLATION_HERCULES[9].setPoint(72.3, 206.3, 6.6);
        CONSTELLATION_HERCULES[10].setPoint(51.8, 209, 5.3);
        CONSTELLATION_HERCULES[11].setPoint(82.4, 120.2, 6.9);
        CONSTELLATION_HERCULES[12].setPoint(146, 122.9, 5.5);
        CONSTELLATION_HERCULES[13].setPoint(166.3, 126.2, 8.6);
        CONSTELLATION_HERCULES[14].setPoint(231.1, 98.5, 6.8);
        CONSTELLATION_HERCULES[15].setPoint(242.2, 58.1, 5.4);
        CONSTELLATION_HERCULES[16].setPoint(284.9, 22.2, 7.1);
        CONSTELLATION_HERCULES[17].setPoint(262.4, 13.3, 5.7);
        CONSTELLATION_HERCULES[18].setPoint(118.9, 29.1, 7.5);

        CONSTELLATION_LEO[0].setPoint(363, 131.1, 6.6);
        CONSTELLATION_LEO[1].setPoint(342.4, 106.3, 5.7);
        CONSTELLATION_LEO[2].setPoint(302.3, 271.8, 11.7);
        CONSTELLATION_LEO[3].setPoint(305, 215.5, 7.8);
        CONSTELLATION_LEO[4].setPoint(279.2, 136.6, 6.9);
        CONSTELLATION_LEO[5].setPoint(271.6, 178.1, 7.7);
        CONSTELLATION_LEO[6].setPoint(117.1, 221.6, 8.2);
        CONSTELLATION_LEO[7].setPoint(122.5, 163.1, 8.2);
        CONSTELLATION_LEO[8].setPoint(16.7, 223.4, 9.6);

        CONSTELLATION_ORION[0].setPoint(239.8, 287.2, 5.7);
        CONSTELLATION_ORION[1].setPoint(226.9, 296, 7.7);
        CONSTELLATION_ORION[2].setPoint(208.3, 301.2, 6.7);
        CONSTELLATION_ORION[3].setPoint(191.1, 296, 7.7);
        CONSTELLATION_ORION[4].setPoint(157.7, 287.2, 5.7);
        CONSTELLATION_ORION[5].setPoint(147.4, 275, 5.5);
        CONSTELLATION_ORION[6].setPoint(356.2, 126.7, 6.2);
        CONSTELLATION_ORION[7].setPoint(353.2, 102.1, 6);
        CONSTELLATION_ORION[8].setPoint(294.5, 92.3, 9.3);
        CONSTELLATION_ORION[9].setPoint(289.3, 78.3, 5.4);
        CONSTELLATION_ORION[10].setPoint(238.8, 104.5, 5.0);
        CONSTELLATION_ORION[11].setPoint(213.4, 122.1, 12.0);
        CONSTELLATION_ORION[12].setPoint(239.8, 177.7, 11.6);
        CONSTELLATION_ORION[13].setPoint(200.3, 205.2, 9.9);
        CONSTELLATION_ORION[14].setPoint(126.5, 185.9, 7.7);
        CONSTELLATION_ORION[15].setPoint(118, 173.6, 8.1);
        CONSTELLATION_ORION[16].setPoint(110.2, 160.6, 7.6);
        CONSTELLATION_ORION[17].setPoint(53.6, 225, 7.4);
        CONSTELLATION_ORION[18].setPoint(40.7, 233.2, 12.0);
        CONSTELLATION_ORION[19].setPoint(24.4, 139.7, 10.3);

        CONSTELLATION_PEGASUS[0].setPoint(15.6, 206.3, 5.7);
        CONSTELLATION_PEGASUS[1].setPoint(158.2, 216.3, 6.6);
        CONSTELLATION_PEGASUS[2].setPoint(195.2, 244, 5);
        CONSTELLATION_PEGASUS[3].setPoint(205.1, 254, 5.3);
        CONSTELLATION_PEGASUS[4].setPoint(272.2, 293.2, 5.8);
        CONSTELLATION_PEGASUS[5].setPoint(325.4, 259.8, 6.8);
        CONSTELLATION_PEGASUS[6].setPoint(320.9, 194.8, 6.1);
        CONSTELLATION_PEGASUS[7].setPoint(363.7, 170.1, 5);
        CONSTELLATION_PEGASUS[8].setPoint(315, 125.5, 5);
        CONSTELLATION_PEGASUS[9].setPoint(272.2, 130.1, 5.8);
        CONSTELLATION_PEGASUS[10].setPoint(195.9, 147.2, 6);
        CONSTELLATION_PEGASUS[11].setPoint(189, 138.1, 6.4);
        CONSTELLATION_PEGASUS[12].setPoint(204.3, 89.7, 7.5);
        CONSTELLATION_PEGASUS[13].setPoint(163.4, 108.5, 7.9);
        CONSTELLATION_PEGASUS[14].setPoint(45, 87.4, 7.6);

        CONSTELLATION_SAGITTARIUS[0].setPoint(359.2, 162.4, 5.2);
        CONSTELLATION_SAGITTARIUS[1].setPoint(301.6, 76.4, 10);
        CONSTELLATION_SAGITTARIUS[2].setPoint(166.2, 81.4, 7.3);
        CONSTELLATION_SAGITTARIUS[3].setPoint(152.8, 69.6, 6.3);
        CONSTELLATION_SAGITTARIUS[4].setPoint(128, 48.6, 6.1);
        CONSTELLATION_SAGITTARIUS[5].setPoint(116.3, 36.7, 7.5);
        CONSTELLATION_SAGITTARIUS[6].setPoint(273.3, 254.1, 7.4);
        CONSTELLATION_SAGITTARIUS[7].setPoint(260.6, 228.4, 9.2);
        CONSTELLATION_SAGITTARIUS[8].setPoint(308.8, 188.3, 7.3);
        CONSTELLATION_SAGITTARIUS[9].setPoint(273.3, 174.2, 7.5);
        CONSTELLATION_SAGITTARIUS[10].setPoint(260.6, 124.4, 9.3);
        CONSTELLATION_SAGITTARIUS[11].setPoint(213.6, 139.6, 7.8);
        CONSTELLATION_SAGITTARIUS[12].setPoint(189, 132.7, 10.6);
        CONSTELLATION_SAGITTARIUS[13].setPoint(171.6, 170.3, 10.9);
        CONSTELLATION_SAGITTARIUS[14].setPoint(158.2, 147.9, 8.7);
        CONSTELLATION_SAGITTARIUS[15].setPoint(84.4, 118.8, 7);
        CONSTELLATION_SAGITTARIUS[16].setPoint(125.5, 294.8, 7.7);
        CONSTELLATION_SAGITTARIUS[17].setPoint(128, 342, 9);
        CONSTELLATION_SAGITTARIUS[18].setPoint(56.8, 313.4, 9.4);
        CONSTELLATION_SAGITTARIUS[19].setPoint(38.2, 232.6, 5);
        CONSTELLATION_SAGITTARIUS[20].setPoint(36.4, 241.6, 7.9);
        CONSTELLATION_SAGITTARIUS[21].setPoint(33.8, 148.1, 5.3);
        CONSTELLATION_SAGITTARIUS[22].setPoint(20.3, 158, 7.3);
    }

    enum ConstellationNames {
        ANDROMEDA,
        AQUARIUS,
        CENTAURUS,
        DRACO,
        GEMINI,
        HERCULES,
        LEO,
        ORION,
        PEGASUS,
        SAGITTARIUS,
        NUM_OF_CONSTELLATIONS
    };

    Point *getConstellation(int c) {
        switch (c) {
            case ANDROMEDA:     constellationLength = LENGTHOF(CONSTELLATION_ANDROMEDA);    return CONSTELLATION_ANDROMEDA;
            case AQUARIUS:      constellationLength = LENGTHOF(CONSTELLATION_AQUARIUS);     return CONSTELLATION_AQUARIUS;
            case CENTAURUS:     constellationLength = LENGTHOF(CONSTELLATION_CENTAURUS);    return CONSTELLATION_CENTAURUS;
            case DRACO:         constellationLength = LENGTHOF(CONSTELLATION_DRACO);        return CONSTELLATION_DRACO;
            case GEMINI:        constellationLength = LENGTHOF(CONSTELLATION_GEMINI);       return CONSTELLATION_GEMINI;
            case HERCULES:      constellationLength = LENGTHOF(CONSTELLATION_HERCULES);     return CONSTELLATION_HERCULES;
            case LEO:           constellationLength = LENGTHOF(CONSTELLATION_LEO);          return CONSTELLATION_LEO;
            case ORION:         constellationLength = LENGTHOF(CONSTELLATION_ORION);        return CONSTELLATION_ORION;
            case PEGASUS:       constellationLength = LENGTHOF(CONSTELLATION_PEGASUS);      return CONSTELLATION_PEGASUS;
            case SAGITTARIUS:   constellationLength = LENGTHOF(CONSTELLATION_SAGITTARIUS);  return CONSTELLATION_SAGITTARIUS;
            default:                                                                        return NULL;
        }
    }

    std::string constellationName(int constellation) {
        switch(constellation) {
            case ANDROMEDA:     return "Andromeda";
            case AQUARIUS:      return "Aquarius";
            case CENTAURUS:     return "Centaurus";
            case DRACO:         return "Draco";
            case GEMINI:        return "Gemini";
            case HERCULES:      return "Hercules";
            case LEO:           return "Leo";
            case ORION:         return "Orion";
            case PEGASUS:       return "Pegasus";
            case SAGITTARIUS:   return "Sagittarius";
            default:            return "";
        }
    }
};