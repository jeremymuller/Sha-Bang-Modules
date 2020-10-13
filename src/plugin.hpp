#pragma once
#include <rack.hpp>
#include "Quantize.cpp"
#include "Constellations.cpp"


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model *modelStochSeq;
extern Model *modelStochSeq4;
extern Model *modelPolyrhythmClock;
extern Model *modelRandGates;
extern Model *modelNeutrinode;
extern Model *modelCosmosis;
extern Model *modelJeremyBlankPanel;
extern Model *modelQubitCrusher;

/************************** INLINE FUNCTIONS **************************/

inline float dist(Vec a, Vec b) { // returns distance between two points
    return std::sqrt(std::pow((a.x-b.x), 2) + std::pow((a.y-b.y), 2));
}

inline float mag(Vec a) { // returns magnitude of vector
    return std::sqrt(a.x * a.x + a.y * a.y);
}

inline float randRange(float max) { // returns random float up to max
    return random::uniform() * max;
}

inline float randRange(float min, float max) { // returns random float within min/max range
    return random::uniform() * fabs(max-min) + min;
}

/************************** LABEL **************************/

struct LeftAlignedLabel : Widget {
    std::string text;
	int fontSize;
    NVGcolor color;
	LeftAlignedLabel(int _fontSize = 13) {
		fontSize = _fontSize;
		box.size.y = BND_WIDGET_HEIGHT;
	}
	void draw(const DrawArgs &args) override {
		nvgTextAlign(args.vg, NVG_ALIGN_LEFT);
        nvgFillColor(args.vg, color);
        // nvgFillColor(args.vg, nvgRGB(128, 0, 219));
        nvgFontSize(args.vg, fontSize);
		nvgText(args.vg, 0, 0, text.c_str(), NULL);
	}
};

struct CenterAlignedLabel : Widget {
    std::string text;
    int fontSize;
    NVGcolor color;
    CenterAlignedLabel(int _fontSize = 13) {
        fontSize = _fontSize;
        box.size.y = BND_WIDGET_HEIGHT;
    }
    void draw(const DrawArgs &args) override {
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
        nvgFillColor(args.vg, color);
        nvgFontSize(args.vg, fontSize);
        nvgText(args.vg, 0, 0, text.c_str(), NULL);
    }
};

/************************** PORTS **************************/

struct TinyPortPurple : SvgPort {
    TinyPortPurple() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPortPurple.svg")));
    }
};

struct TinyPortBlue : SvgPort {
    TinyPortBlue() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPortBlue.svg")));
    }
};

struct TinyPortAqua : SvgPort {
    TinyPortAqua() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPortAqua.svg")));
    }
};

struct TinyPortRed : SvgPort {
    TinyPortRed() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPortRed.svg")));
    }
};

struct TinyPJ301M : SvgPort {
    TinyPJ301M() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPJ301M.svg")));
    }
};

struct TinyPJ301MPurple : SvgPort {
    TinyPJ301MPurple() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPJ301MPurple.svg")));
    }
};

struct TinyPJ301MBlue : SvgPort {
    TinyPJ301MBlue() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPJ301MBlue.svg")));
    }
};

struct TinyPJ301MAqua : SvgPort {
    TinyPJ301MAqua() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPJ301MAqua.svg")));
    }
};

struct TinyPJ301MRed : SvgPort {
    TinyPJ301MRed() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPJ301MRed.svg")));
    }
};

struct PJ301MPurple : SvgPort {
    PJ301MPurple() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PJ301MPurple.svg")));
    }
};

struct PJ301MBlue : SvgPort {
    PJ301MBlue() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PJ301MBlue.svg")));
    }
};

struct PJ301MAqua : SvgPort {
    PJ301MAqua() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PJ301MAqua.svg")));
    }
};

struct PJ301MRed : SvgPort {
    PJ301MRed() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PJ301MRed.svg")));
    }
};

struct OrbitPort : SvgPort {
    OrbitPort() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/OrbitPort.svg")));
    }
};

/************************** SWITCHES **************************/

struct Jeremy_HSwitch : SvgSwitch {
    Jeremy_HSwitch() {
        shadow->opacity = 0;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Switch_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Switch_1.svg")));
    }
};

struct Jeremy_HSwitchBlue : SvgSwitch {
    Jeremy_HSwitchBlue() {
        shadow->opacity = 0;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueSwitch_0.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueSwitch_1.svg")));
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

struct PauseButton : SvgSwitch {
    PauseButton() {
        // momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PauseButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PauseButtonDown.svg")));
    }
};

struct ToggleButton : SvgSwitch {
    ToggleButton() {
        // momentary = true;
        // shadow->opacity = 0;
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

struct TinyPurpleButton : SvgSwitch {
    TinyPurpleButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPurpleButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPurpleButtonDown.svg")));
    }
};

struct TinyBlueButton : SvgSwitch {
    TinyBlueButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyBlueButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyBlueButtonDown.svg")));
    }
};

struct TinyAquaButton : SvgSwitch {
    TinyAquaButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyAquaButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyAquaButtonDown.svg")));
    }
};

struct TinyRedButton : SvgSwitch {
    TinyRedButton() {
        momentary = true;
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyRedButtonUp.svg")));
        addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyRedButtonDown.svg")));
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

struct PurpleInvertKnob : RoundKnob {
    PurpleInvertKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PurpleInvertKnob.svg")));
        snap = true;
    }
};

struct BlueInvertKnob : RoundKnob {
    BlueInvertKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueInvertKnob.svg")));
        snap = true;
    }
};

struct AquaInvertKnob : RoundKnob {
    AquaInvertKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AquaInvertKnob.svg")));
        snap = true;
    }
};

struct RedInvertKnob : RoundKnob {
    RedInvertKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RedInvertKnob.svg")));
        snap = true;
    }
};

struct TinyPurpleKnob : RoundKnob {
    TinyPurpleKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyPurpleKnob.svg")));
    }
};

struct TinyBlueKnob : RoundKnob {
    TinyBlueKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyBlueKnob.svg")));
    }
};

struct TinyAquaKnob : RoundKnob {
    TinyAquaKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyAquaKnob.svg")));
    }
};

struct TinyRedKnob : RoundKnob {
    TinyRedKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyRedKnob.svg")));
    }
};

struct TinyBlueInvertKnob : RoundKnob {
    TinyBlueInvertKnob() {
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TinyBlueInvertKnob.svg")));
        snap = true;
    }
};

/************************** KNOBS with LABELS **************************/
// code help from https://github.com/jeremywen/JW-Modules/blob/master/src/JWModules.hpp

struct PurpleInvertKnobLabel : PurpleInvertKnob {
    LeftAlignedLabel *linkedLabel = NULL;
    Module *linkedModule = NULL;

    PurpleInvertKnobLabel() {}

    void connectLabel(LeftAlignedLabel *label, Module *module) {
        linkedLabel = label;
        linkedModule = module;
        if (linkedModule && linkedLabel) {
            linkedLabel->text = formatCurrentValue();
            linkedLabel->color = nvgRGB(128, 0, 219);
        }
    }

    void onChange(const event::Change &e) override {
        RoundKnob::onChange(e);
        if (linkedModule && linkedLabel) {
            linkedLabel->text = formatCurrentValue();
        }
    }

    virtual std::string formatCurrentValue() {
        if (paramQuantity != NULL) {
            return std::to_string(static_cast<unsigned int>(paramQuantity->getValue()));
        }
        return "";
    }
};

struct BlueInvertKnobLabel : BlueInvertKnob {
    LeftAlignedLabel *linkedLabel = NULL;
    Module *linkedModule = NULL;

    BlueInvertKnobLabel() {}

    void connectLabel(LeftAlignedLabel *label, Module *module) {
        linkedLabel = label;
        linkedModule = module;
        if (linkedModule && linkedLabel) {
            linkedLabel->text = formatCurrentValue();
            linkedLabel->color = nvgRGB(38, 0, 255);
        }
    }

    void onChange(const event::Change &e) override {
        RoundKnob::onChange(e);
        if (linkedModule && linkedLabel) {
            linkedLabel->text = formatCurrentValue();
        }
    }

    virtual std::string formatCurrentValue() {
        if (paramQuantity != NULL) {
            return std::to_string(static_cast<unsigned int>(paramQuantity->getValue()));
        }
        return "";
    }
};

struct BlueInvertKnobLabelCentered : BlueInvertKnob {
    CenterAlignedLabel *linkedLabel = NULL;
    Module *linkedModule = NULL;

    BlueInvertKnobLabelCentered() {}

    void connectLabel(CenterAlignedLabel *label, Module *module) {
        linkedLabel = label;
        linkedModule = module;
        if (linkedModule && linkedLabel) {
            linkedLabel->text = formatCurrentValue();
            linkedLabel->color = nvgRGB(0, 0, 0);
            // linkedLabel->color = nvgRGB(38, 0, 255);
        }
    }

    void onChange(const event::Change &e) override {
        RoundKnob::onChange(e);
        if (linkedModule && linkedLabel) {
            linkedLabel->text = formatCurrentValue();
        }
    }

    virtual std::string formatCurrentValue() {
        if (paramQuantity != NULL) {
            return std::to_string(static_cast<unsigned int>(paramQuantity->getValue()));
        }
        return "";
    }
};

struct PurpleNoteKnob : PurpleInvertKnobLabel {
    Quantize *quantize;
    PurpleNoteKnob() {}

    std::string formatCurrentValue() override {
        if (paramQuantity != NULL) {
            return quantize->noteName(static_cast<unsigned int>(paramQuantity->getValue()));
        }
        return "";
    }
};

struct PurpleScaleKnob : PurpleInvertKnobLabel {
    Quantize *quantize;
    PurpleScaleKnob() {}  
    
    std::string formatCurrentValue() override {
        if (paramQuantity != NULL) {
            return quantize->scaleName(static_cast<unsigned int>(paramQuantity->getValue()));
        }
        return "";
    }
};

struct BlueNoteKnob : BlueInvertKnobLabel {
    Quantize *quantize;
    BlueNoteKnob() {}

    std::string formatCurrentValue() override {
        if (paramQuantity != NULL) {
            return quantize->noteName(static_cast<unsigned int>(paramQuantity->getValue()));
        }
        return "";
    }
};

struct BlueScaleKnob : BlueInvertKnobLabel {
    Quantize *quantize;
    BlueScaleKnob() {}  
    
    std::string formatCurrentValue() override {
        if (paramQuantity != NULL) {
            return quantize->scaleName(static_cast<unsigned int>(paramQuantity->getValue()));
        }
        return "";
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