#include "plugin.hpp"

struct PolyrhythmClock : Module {
    enum BPMModes {
        BPM_CV,
        BPM_P12,
        BPM_P24,
        NUM_BPM_MODES
    };
    enum ParamIds {
        CLOCK_TOGGLE_PARAM,
        BPM_PARAM,
        TUPLET1_RHYTHM_PARAM,
        TUPLET1_DUR_PARAM,
        TUPLET2_RHYTHM_PARAM,
        TUPLET2_DUR_PARAM,
        TUPLET3_RHYTHM_PARAM,
        TUPLET3_DUR_PARAM,
        TUPLETS_RAND_PARAM,
        NUM_PARAMS = TUPLETS_RAND_PARAM + 3
    };
    enum InputIds {
        RESET_INPUT,
        EXT_CLOCK_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        MASTER_PULSE_OUTPUT,
        TUPLET1_OUTPUT,
        TUPLET2_OUTPUT,
        TUPLET3_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        TOGGLE_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger toggleTrig, bpmInputTrig, resetTrig;
    dsp::PulseGenerator gatePulses[4];
    bool tupletGates[4] = {};
    bool clockOn = false;
    int bpmInputMode = BPM_CV;
    int extPulseIndex = 0;
    int extIntervalTime = 0; // keeps track of number of samples lapsed
    int ppqn = 0;
    float period = 0.0;
    int timeOut = 1; // seconds
    float currentBPM = 120.0;
    float clockFreq = 2.0; // Hz
    float phases[4] = {};
    float randoms[4];
    float currentRhythmFraction[3] = {};
    float phase = 0;
    float phaseTuplet1 = 0;
    float phaseTuplet2 = 0;
    float phaseTuplet3 = 0;

    PolyrhythmClock() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CLOCK_TOGGLE_PARAM, 0.0, 1.0, 0.0, "toggle clock");
        configParam(BPM_PARAM, -2.0, 6.0, 1.0, "Tempo", " bpm", 2.0, 60.0);
        configParam(TUPLET1_RHYTHM_PARAM, 1.0, 13.0, 1.0);
        configParam(TUPLET1_DUR_PARAM, 1.0, 13.0, 1.0);
        configParam(TUPLET2_RHYTHM_PARAM, 1.0, 13.0, 1.0);
        configParam(TUPLET2_DUR_PARAM, 1.0, 13.0, 1.0);
        configParam(TUPLET3_RHYTHM_PARAM, 1.0, 13.0, 1.0);
        configParam(TUPLET3_DUR_PARAM, 1.0, 13.0, 1.0);

        currentRhythmFraction[0] = params[TUPLET1_RHYTHM_PARAM].getValue() / params[TUPLET1_DUR_PARAM].getValue();
        currentRhythmFraction[1] = params[TUPLET2_RHYTHM_PARAM].getValue() / params[TUPLET2_DUR_PARAM].getValue();
        currentRhythmFraction[2] = params[TUPLET3_RHYTHM_PARAM].getValue() / params[TUPLET3_DUR_PARAM].getValue();

        for (int i = 0; i < 3; i++) {
            configParam(TUPLETS_RAND_PARAM+i, 0.0, 1.0, 1.0, "probability");
            randoms[i] = random::uniform();
        }
    }

    void checkPhases() {
        for (int i = 0; i < 4; i++) {
            if (phases[i] >= 1.0) {
                phases[i] = 0.0;
                if (i > 0) randoms[i] = random::uniform();
            }

            // TODO: this is awkward, i think it can be better
            if (i > 0) {
                if (randoms[i] < params[TUPLETS_RAND_PARAM + i-1].getValue())
                    if (phases[i] == 0.0) gatePulses[i].trigger(1e-3f);
            } else {
                if (phases[i] == 0.0) gatePulses[i].trigger(1e-3f);
            }
        }
    }

    void resetPhases() {
        for (int i = 0; i < 4; i++) {
            phases[i] = 0.0;
        }
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        json_object_set_new(rootJ, "clockOn", json_boolean(clockOn));
        json_object_set_new(rootJ, "extmode", json_integer(bpmInputMode));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *clockOnJ = json_object_get(rootJ, "clockOn");
        if (clockOnJ) clockOn = json_boolean_value(clockOnJ);

        json_t *extmodeJ = json_object_get(rootJ, "extmode");
        if (extmodeJ) bpmInputMode = json_integer_value(extmodeJ);
    }

    void process(const ProcessArgs& args) override {

        if (resetTrig.process(inputs[RESET_INPUT].getVoltage())) {
            resetPhases();
        }

        if (toggleTrig.process(params[CLOCK_TOGGLE_PARAM].getValue())) {
            clockOn = !clockOn;
        }

        lights[TOGGLE_LIGHT].setBrightness(clockOn ? 1.0 : 0.0);

        for (int i = 0; i < 4; i++) {
            tupletGates[i] = false;
        }

        // This could possibly be cleaned up in a later version
        bool bpmDetect = false;
        if (inputs[EXT_CLOCK_INPUT].isConnected()) {
            if (bpmInputMode == BPM_CV) {
                clockFreq = 2.0 * std::pow(2.0, inputs[EXT_CLOCK_INPUT].getVoltage());
            } else if (bpmInputMode == BPM_P12) {
                ppqn = 12;
                bpmDetect = bpmInputTrig.process(inputs[EXT_CLOCK_INPUT].getVoltage());
                if (bpmDetect)
                    clockOn = true;
            } else {
                ppqn = 24;
                bpmDetect = bpmInputTrig.process(inputs[EXT_CLOCK_INPUT].getVoltage());
                if (bpmDetect)
                    clockOn = true;
            }
        } else {
            float bpmParam = params[BPM_PARAM].getValue();
            clockFreq = std::pow(2.0, bpmParam);
        }
        currentBPM = clockFreq * 60;

        if (clockOn) {
            if (bpmInputMode != BPM_CV && inputs[EXT_CLOCK_INPUT].isConnected()) {
                period += args.sampleTime;
                if (period > timeOut) clockOn = false;
                if (bpmDetect) {
                    if (extPulseIndex > 1) {
                        clockFreq = (1.0 / period) / (float)ppqn;
                    }

                    extPulseIndex++;
                    if (extPulseIndex >= ppqn) extPulseIndex = 0;
                    period = 0.0;
                }
            }

            float frac1 = params[TUPLET1_RHYTHM_PARAM].getValue() / params[TUPLET1_DUR_PARAM].getValue();
            float frac2 = params[TUPLET2_RHYTHM_PARAM].getValue() / params[TUPLET2_DUR_PARAM].getValue();
            float frac3 = params[TUPLET3_RHYTHM_PARAM].getValue() / params[TUPLET3_DUR_PARAM].getValue();
            if (currentRhythmFraction[0] != frac1 && phases[0] == 0.0) {
                currentRhythmFraction[0] = frac1;
                phases[1] = 0.0;
            }
            if (currentRhythmFraction[1] != frac2 && phases[1] == 0.0) {
                currentRhythmFraction[1] = frac2;
                phases[2] = 0.0;
            }
            if (currentRhythmFraction[2] != frac3 && phases[2] == 0.0) {
                currentRhythmFraction[2] = frac3;
                phases[3] = 0.0;
            }

            // calculate embedded tuplets
            phases[0] += clockFreq * args.sampleTime;
            float accFreq = clockFreq * currentRhythmFraction[0];
            phases[1] += accFreq * args.sampleTime;
            accFreq *= params[TUPLET2_RHYTHM_PARAM].getValue() / params[TUPLET2_DUR_PARAM].getValue();
            phases[2] += accFreq * args.sampleTime;
            accFreq *= params[TUPLET3_RHYTHM_PARAM].getValue() / params[TUPLET3_DUR_PARAM].getValue();
            phases[3] += accFreq * args.sampleTime;

            checkPhases();

        } else {
            resetPhases();
        }

        for (int i = 0; i < 4; i++) {
            tupletGates[i] = gatePulses[i].process(1.0 / args.sampleRate);
        }

        outputs[MASTER_PULSE_OUTPUT].setVoltage(tupletGates[0] ? 10.0 : 0.0);
        outputs[TUPLET1_OUTPUT].setVoltage(tupletGates[1] ? 10.0 : 0.0);
        outputs[TUPLET2_OUTPUT].setVoltage(tupletGates[2] ? 10.0 : 0.0);
        outputs[TUPLET3_OUTPUT].setVoltage(tupletGates[3] ? 10.0 : 0.0);
    }
};

struct ExternalClockModeValueItem : MenuItem {
    PolyrhythmClock *module;
    PolyrhythmClock::BPMModes bpmMode;
    void onAction(const event::Action &e) override {
        module->bpmInputMode = bpmMode;
    }
};

struct ExternalClockModeItem : MenuItem {
    PolyrhythmClock *module;
    Menu *createChildMenu() override {
        Menu *menu = new Menu;
        std::vector<std::string> bpmModeNames = {"CV", "12 PPQN", "24 PPQN"};
        for (int i = 0; i < PolyrhythmClock::NUM_BPM_MODES; i++) {
            PolyrhythmClock::BPMModes bpmMode = (PolyrhythmClock::BPMModes) i;
            ExternalClockModeValueItem *item = new ExternalClockModeValueItem;
            item->text = bpmModeNames[i];
            item->rightText = CHECKMARK(module->bpmInputMode == bpmMode);
            item->module = module;
            item->bpmMode = bpmMode;
            menu->addChild(item);
        }
        return menu;
    }
};

struct BPMDisplay : Widget {
    std::string text;
    int fontSize;
    PolyrhythmClock *module;
    BPMDisplay(int _fontSize = 15) {
        fontSize = _fontSize;
        box.size.y = BND_WIDGET_HEIGHT;
    }
    void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        // int bpm = int(std::pow(2.0, module->params[PolyrhythmClock::BPM_PARAM].getValue()) * 60);
        int bpm = static_cast<int>(round(module->currentBPM));
        text = std::to_string(bpm) + " bpm";
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER + NVG_ALIGN_TOP);
        nvgFillColor(args.vg, nvgRGB(128, 0, 219));
        nvgFontSize(args.vg, fontSize);
        nvgText(args.vg, 0, 0, text.c_str(), NULL);
    }
};

struct RatioDisplay : Widget {
    std::string text1, text2, text3;
	int fontSize;
    PolyrhythmClock *module;
	RatioDisplay(int _fontSize = 13) {
		fontSize = _fontSize;
		box.size.y = BND_WIDGET_HEIGHT;
	}
	void draw(const DrawArgs &args) override {
        //background
        // nvgFillColor(args.vg, nvgRGB(230, 230, 230));
        // nvgBeginPath(args.vg);
        // nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        // nvgFill(args.vg);
        if (module == NULL) return;

        int num1 = (int)module->params[PolyrhythmClock::TUPLET1_RHYTHM_PARAM].getValue();
        int den1 = (int)module->params[PolyrhythmClock::TUPLET1_DUR_PARAM].getValue();
        text1 = std::to_string(num1) + ":" + std::to_string(den1);
        float xPos1 = num1 < 10 ? 7.6 : 0.0;
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT + NVG_ALIGN_TOP);
        // nvgTextAlign(args.vg, NVG_ALIGN_CENTER + NVG_ALIGN_TOP);
        // nvgFillColor(args.vg, nvgRGB(38, 0, 255));
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFontSize(args.vg, fontSize);
		nvgText(args.vg, xPos1, 0, text1.c_str(), NULL);

        int num2 = (int)module->params[PolyrhythmClock::TUPLET2_RHYTHM_PARAM].getValue();
        int den2 = (int)module->params[PolyrhythmClock::TUPLET2_DUR_PARAM].getValue();
        text2 = std::to_string(num2) + ":" + std::to_string(den2);
        float xPos2 = num2 < 10 ? 7.6 : 0.0;
        // nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
        // nvgTextAlign(args.vg, NVG_ALIGN_TOP);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFontSize(args.vg, fontSize);
        // nvgText(args.vg, 5, 5, text2.c_str(), NULL);
        nvgText(args.vg, xPos2, 75.4, text2.c_str(), NULL);

        int num3 = (int)module->params[PolyrhythmClock::TUPLET3_RHYTHM_PARAM].getValue();
        int den3 = (int)module->params[PolyrhythmClock::TUPLET3_DUR_PARAM].getValue();
        text3 = std::to_string(num3) + ":" + std::to_string(den3);
        float xPos3 = num3 < 10 ? 7.6 : 0.0;
        // nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
        // nvgTextAlign(args.vg, NVG_ALIGN_TOP);
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFontSize(args.vg, fontSize);
        nvgText(args.vg, xPos3, 148.5, text3.c_str(), NULL);
    }
};

struct PolyrhythmClockWidget : ModuleWidget {
    PolyrhythmClockWidget(PolyrhythmClock *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyrhythmClock.svg")));

        // CenteredLabel *const ratioLabel = new CenteredLabel();
        // ratioLabel->box.pos = Vec(60.9, 203.5);
        // ratioLabel->text = "5:4";
        // addChild(ratioLabel);
        BPMDisplay *bpmLabel = new BPMDisplay();
        bpmLabel->module = module;
        bpmLabel->box.pos = Vec(45, 92.8);
        addChild(bpmLabel);

        RatioDisplay *ratioLabel1 = new RatioDisplay();
        ratioLabel1->module = module;
        // ratioLabel1->box.pos = Vec(36, 175.5);
        ratioLabel1->box.pos = Vec(29, 151.6);
        // ratioLabel1->box.pos = Vec(45, 175.5);
        ratioLabel1->box.size.x = 30; // 10
        ratioLabel1->text2 = "5:4";
        addChild(ratioLabel1);
        // RatioDisplay *ratioLabel2 = new RatioDisplay();
        // ratioLabel2->module = module;
        // ratioLabel2->box.pos = Vec(36.7, 229.6);
        // ratioLabel2->box.size.x = 10;
        // ratioLabel2->text = "5:4";
        // addChild(ratioLabel2);

        addChild(createWidget<JeremyScrew>(Vec(12, 2)));
        addChild(createWidget<JeremyScrew>(Vec(12, box.size.y - 14)));
        addChild(createWidget<JeremyScrew>(Vec(box.size.x-12-12, 2)));
        addChild(createWidget<JeremyScrew>(Vec(box.size.x-12-12, box.size.y - 14)));

        // addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(29.8 - 3.21, 54 - 3.21), module, PolyrhythmClock::TOGGLE_LIGHT));

        addParam(createParamCentered<PurpleKnob>(Vec(45, 76.7), module, PolyrhythmClock::BPM_PARAM));
        addParam(createParamCentered<TinyPurpleButton>(Vec(45, 54), module, PolyrhythmClock::CLOCK_TOGGLE_PARAM));
        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(45 - 3.21, 54 - 3.21), module, PolyrhythmClock::TOGGLE_LIGHT));
        // inputs
        addInput(createInputCentered<TinyPJ301M>(Vec(19.9, 76.7), module, PolyrhythmClock::RESET_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(70.1, 76.7), module, PolyrhythmClock::EXT_CLOCK_INPUT));

        // tuplet 1
        addParam(createParamCentered<BlueInvertKnob>(Vec(19.9, 173.6), module, PolyrhythmClock::TUPLET1_RHYTHM_PARAM));
        addParam(createParamCentered<BlueInvertKnob>(Vec(70.1, 173.6), module, PolyrhythmClock::TUPLET1_DUR_PARAM));
        addParam(createParamCentered<TinyBlueKnob>(Vec(45, 174.2), module, PolyrhythmClock::TUPLETS_RAND_PARAM));
        // tuplet 2
        addParam(createParamCentered<AquaInvertKnob>(Vec(19.9, 249), module, PolyrhythmClock::TUPLET2_RHYTHM_PARAM));
        addParam(createParamCentered<AquaInvertKnob>(Vec(70.1, 249), module, PolyrhythmClock::TUPLET2_DUR_PARAM));
        addParam(createParamCentered<TinyAquaKnob>(Vec(45, 249.6), module, PolyrhythmClock::TUPLETS_RAND_PARAM+1));
        // tuplet 3
        addParam(createParamCentered<RedInvertKnob>(Vec(19.9, 322.1), module, PolyrhythmClock::TUPLET3_RHYTHM_PARAM));
        addParam(createParamCentered<RedInvertKnob>(Vec(70.1, 322.1), module, PolyrhythmClock::TUPLET3_DUR_PARAM));
        addParam(createParamCentered<TinyRedKnob>(Vec(45, 322.7), module, PolyrhythmClock::TUPLETS_RAND_PARAM+2));

        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 119.8), module, PolyrhythmClock::MASTER_PULSE_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(45, 195.8), module, PolyrhythmClock::TUPLET1_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(45, 271.1), module, PolyrhythmClock::TUPLET2_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(45, 344.3), module, PolyrhythmClock::TUPLET3_OUTPUT));
    }

    void appendContextMenu(Menu *menu) override {
        PolyrhythmClock *module = dynamic_cast<PolyrhythmClock*>(this->module);
        menu->addChild(new MenuEntry);

        ExternalClockModeItem *extClockModeItem = new ExternalClockModeItem;
        extClockModeItem->text = "External Clock Mode";
        extClockModeItem->rightText = RIGHT_ARROW;
        extClockModeItem->module = module;
        menu->addChild(extClockModeItem);
    }
};

Model *modelPolyrhythmClock = createModel<PolyrhythmClock, PolyrhythmClockWidget>("PolyrhythmClock");