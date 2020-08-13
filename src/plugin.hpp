#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model *modelStochSeq;
extern Model *modelPolyrhythmClock;

/************************** PORTS **************************/

struct TinyPJ301M : SvgPort {
    TinyPJ301M() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPJ301M.svg")));
    }
};

struct OrbitPort : SvgPort {
    OrbitPort() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/OrbitPort.svg")));
    }
};

/************************** BUTTONS **************************/

struct DefaultButton : SvgSwitch {
    DefaultButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DefaultButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DefaultButtonDown.svg")));
    }
};

struct ToggleButton : SvgSwitch {
    ToggleButton() {
        // momentary = true;
        shadow->opacity = 0;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ToggleButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ToggleButtonDown.svg")));
    }
};

struct PurpleButton : SvgSwitch {
    PurpleButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PurpleButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PurpleButtonDown.svg")));
    }
};

struct BlueButton : SvgSwitch {
    BlueButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueButtonDown.svg")));
    }
};

struct AquaButton : SvgSwitch {
    AquaButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AquaButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AquaButtonDown.svg")));
    }
};

struct RedButton : SvgSwitch {
    RedButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RedButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RedButtonDown.svg")));
    }
};

/************************** KNOBS **************************/

struct PurpleKnob : RoundKnob {
    PurpleKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PurpleKnob.svg")));
    }
};

struct BlueKnob : RoundKnob {
    BlueKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueKnob.svg")));
    }
};

struct AquaKnob : RoundKnob {
    AquaKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AquaKnob.svg")));
    }
};

struct RedKnob : RoundKnob {
    RedKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RedKnob.svg")));
    }
};

struct PurpleSnapKnob : RoundKnob {
    PurpleSnapKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PurpleKnob.svg")));
        snap = true;
    }
};

struct BlueInvertKnob : RoundKnob {
    BlueInvertKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueInvertKnob.svg")));
        snap = true;
    }
};

struct AquaSnapKnob : RoundKnob {
    AquaSnapKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AquaKnob.svg")));
        snap = true;
    }
};

struct RedSnapKnob : RoundKnob {
    RedSnapKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RedKnob.svg")));
    }
};

/************************** LIGHTS **************************/

struct JeremyPurpleLight : ModuleLightWidget {
    JeremyPurpleLight() {
        firstLightId = 1;
        addBaseColor(nvgRGB(128, 0, 219));
    }
};

struct JeremyBlueLight : ModuleLightWidget {
    JeremyBlueLight() {
        firstLightId = 1;
        addBaseColor(nvgRGB(0, 0, 255));
    }
};

struct JeremyAquaLight : ModuleLightWidget {
    JeremyAquaLight() {
        firstLightId = 1;
        addBaseColor(nvgRGB(0, 238, 255));
    }
};

struct JeremyRedLight : ModuleLightWidget {
    JeremyRedLight() {
        firstLightId = 1;
        addBaseColor(nvgRGB(255, 0, 0));
    }
};

/************************** SCREWS **************************/

struct JeremyScrew : SvgScrew {
	JeremyScrew() {
		sw->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/screw.svg")));
		box.size = sw->box.size;
	}
};