#include "plugin.hpp"

#define SLIDER_WIDTH 15
#define SLIDER_TOP 4
#define NUM_OF_SLIDERS 32
// #define NUM_OF_SEQS 4

struct Sequencer {
    dsp::SchmittTrigger dimTrig;
    dsp::SchmittTrigger randomTrig;
    dsp::SchmittTrigger invertTrig;
    dsp::PulseGenerator gatePulse;
    int seqLength;
    int gateIndex;
    int currentPattern = 0;
    float pitchVoltage = 0.0;
    float gateProbabilities[NUM_OF_SLIDERS];

    Sequencer() {
        gateIndex = -1;
        seqLength = NUM_OF_SLIDERS;
        for (int i = 0; i < NUM_OF_SLIDERS; i++) {
            gateProbabilities[i] = random::uniform();
        }
    }

    void clockStep(int l, float spread) {
        seqLength = l;
        gateIndex = (gateIndex + 1) % seqLength;

        // gate
        float prob = gateProbabilities[gateIndex];
        if (random::uniform() < prob) {
            gatePulse.trigger(1e-3);
        }

        // pitch
        pitchVoltage = prob * (2 * spread) - spread;
    }
};

struct StochSeq4 : Module {
    enum SequencerIds {
        PURPLE_SEQ,
        BLUE_SEQ,
        AQUA_SEQ,
        RED_SEQ,
        NUM_SEQS
    };
	enum ParamIds {
		PATTERN_PARAM = NUM_SEQS,
		RANDOM_PARAM = PATTERN_PARAM + NUM_SEQS,
		INVERT_PARAM = RANDOM_PARAM + NUM_SEQS,
		DIMINUTION_PARAM = INVERT_PARAM + NUM_SEQS,
		LENGTH_PARAM = DIMINUTION_PARAM + NUM_SEQS,
		SPREAD_PARAM = LENGTH_PARAM + NUM_SEQS,
		NUM_PARAMS = SPREAD_PARAM + NUM_SEQS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT = NUM_SEQS,
		VOLTS_OUTPUT = GATES_OUTPUT + NUM_SEQS,
		NUM_OUTPUTS = VOLTS_OUTPUT + NUM_SEQS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    dsp::SchmittTrigger clockTrig;
    dsp::SchmittTrigger resetTrig;
    bool resetMode = false;
    Sequencer seqs[NUM_SEQS];

    StochSeq4() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(LENGTH_PARAM + PURPLE_SEQ, 1.0, 32.0, 32.0, "purple seq length");
        configParam(LENGTH_PARAM + BLUE_SEQ, 1.0, 32.0, 32.0, "blue seq length");
        configParam(LENGTH_PARAM + AQUA_SEQ, 1.0, 32.0, 32.0, "aqua seq length");
        configParam(LENGTH_PARAM + RED_SEQ, 1.0, 32.0, 32.0, "red seq length");
        configParam(PATTERN_PARAM + PURPLE_SEQ, 0.0, 7.0, 0.0, "purple pattern");
        configParam(PATTERN_PARAM + BLUE_SEQ, 0.0, 7.0, 0.0, "blue pattern");
        configParam(PATTERN_PARAM + AQUA_SEQ, 0.0, 7.0, 0.0, "aqua pattern");
        configParam(PATTERN_PARAM + RED_SEQ, 0.0, 7.0, 0.0, "red pattern");

        configParam(RANDOM_PARAM + PURPLE_SEQ, 0.0, 1.0, 0.0, "randomize purple pattern");
        configParam(RANDOM_PARAM + BLUE_SEQ, 0.0, 1.0, 0.0, "randomize blue pattern");
        configParam(RANDOM_PARAM + AQUA_SEQ, 0.0, 1.0, 0.0, "randomize aqua pattern");
        configParam(RANDOM_PARAM + RED_SEQ, 0.0, 1.0, 0.0, "randomize red pattern");
        configParam(INVERT_PARAM + PURPLE_SEQ, 0.0, 1.0, 0.0, "invert purple pattern");
        configParam(INVERT_PARAM + BLUE_SEQ, 0.0, 1.0, 0.0, "invert blue pattern");
        configParam(INVERT_PARAM + AQUA_SEQ, 0.0, 1.0, 0.0, "invert aqua pattern");
        configParam(INVERT_PARAM + RED_SEQ, 0.0, 1.0, 0.0, "invert red pattern");
        configParam(DIMINUTION_PARAM + PURPLE_SEQ, 0.0, 1.0, 0.0, "diminish purple pattern");
        configParam(DIMINUTION_PARAM + BLUE_SEQ, 0.0, 1.0, 0.0, "diminish blue pattern");
        configParam(DIMINUTION_PARAM + AQUA_SEQ, 0.0, 1.0, 0.0, "diminish aqua pattern");
        configParam(DIMINUTION_PARAM + RED_SEQ, 0.0, 1.0, 0.0, "diminish red pattern");

        configParam(SPREAD_PARAM + PURPLE_SEQ, -4.0, 4.0, 0.0, "purple spread");
        configParam(SPREAD_PARAM + BLUE_SEQ, -4.0, 4.0, 0.0, "blue spread");
        configParam(SPREAD_PARAM + AQUA_SEQ, -4.0, 4.0, 0.0, "aqua spread");
        configParam(SPREAD_PARAM + RED_SEQ, -4.0, 4.0, 0.0, "red spread");

        // for (int i = 0; i < NUM_SEQS; i++) {
        //     for (int j = 0; j < NUM_OF_SLIDERS; j++) {
        //         seqs[i].gateProbabilities[j] = random::uniform();
        //     }
        // }
    }

    void process(const ProcessArgs& args) override {
        // TODO
        if (resetTrig.process(inputs[RESET_INPUT].getVoltage())) {
            resetMode = true;
        }
        for (int i = 0; i < NUM_SEQS; i++) {
            if ((int)params[PATTERN_PARAM+i].getValue() != seqs[i].currentPattern) {
                int patt = (int)params[PATTERN_PARAM+i].getValue();
                seqs[i].currentPattern = patt;
                genPatterns(patt, i);
            }
            if (seqs[i].randomTrig.process(params[RANDOM_PARAM+i].getValue())) {
                genPatterns(100, i);
            }
            if (seqs[i].invertTrig.process(params[INVERT_PARAM+i].getValue())) {
                invert(i);
            }
            if (seqs[i].dimTrig.process(params[DIMINUTION_PARAM+i].getValue())) {
                diminish(i);
            }
        }
        if (clockTrig.process(inputs[CLOCK_INPUT].getVoltage())) {
            if (resetMode) {
                resetMode = false;
                resetSeqToEnd();
            }
            clockStep();
        }
        for (int i = 0; i < NUM_SEQS; i++) {
            bool pulse = seqs[i].gatePulse.process(1 / args.sampleRate);
            float gateVolt = pulse ? 10.0 : 0.0;
            outputs[GATES_OUTPUT + i].setVoltage(gateVolt);
            outputs[VOLTS_OUTPUT + i].setVoltage(seqs[i].pitchVoltage);
        }
    }

    void clockStep() {
        for (int i = 0; i < NUM_SEQS; i++) {
            int l = (int)params[LENGTH_PARAM+i].getValue();
            float spread = params[SPREAD_PARAM+i].getValue();
            seqs[i].clockStep(l, spread);
        }

    }

    void resetSeqToEnd() {
        // TODO
        for (int i = 0; i < NUM_SEQS; i++) {
            seqs[i].gateIndex = seqs[i].seqLength - 1;
        }
    }

    void invert(int id) {
        for (int i = 0; i < NUM_OF_SLIDERS; i++) {
            float currentProb = seqs[id].gateProbabilities[i];
            seqs[id].gateProbabilities[i] = 1.0 - currentProb;
        }
    }

    void diminish(int id) {
        float *tempArray = new float[16];
        int index = 0;
        for (int i = 0; i < NUM_OF_SLIDERS; i += 2) {
            tempArray[index] = seqs[id].gateProbabilities[i];
            index++;
        }
        for (int i = 0; i < NUM_OF_SLIDERS; i++) {
            seqs[id].gateProbabilities[i] = tempArray[i % 16];
        }
        delete[] tempArray;
    }

    void genPatterns(int c, int id) {
		switch (c) {
			case 0:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					seqs[id].gateProbabilities[i] = 0.0;
				}
				break;
			case 1:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
                    seqs[id].gateProbabilities[i] = 0.5;
                }
				break;
			case 2:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
                    seqs[id].gateProbabilities[i] = 1.0;
                }
				break;
			case 3:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
                    seqs[id].gateProbabilities[i] = (i % 8 == 0) ? 1.0 : 0.0;
                }
				break;
			case 4:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
                    seqs[id].gateProbabilities[i] = (i % 8 == 4) ? 1.0 : 0.0;
                }
				break;
			case 5:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
                    seqs[id].gateProbabilities[i] = (i % 4 == 0) ? 1.0 : 0.0;
                }
				break;
			case 6:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
                    seqs[id].gateProbabilities[i] = (i + 1.0) / NUM_OF_SLIDERS;
                }
				break;
			case 7:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
                    seqs[id].gateProbabilities[i] = std::sin(i / (NUM_OF_SLIDERS - 1.0) * M_PI);
                }
				break;
			default:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
                    seqs[id].gateProbabilities[i] = random::uniform();
                }
		}
	}
};

struct StochSeq4Display : Widget {
    StochSeq4 *module;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;
    int seqId;
    StochSeq4Display() {}

    void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            e.consume(this);
            initX = e.pos.x;
            initY = e.pos.y;
            setProbabilities(initX, initY);
        }
    }

    void onDragStart(const event::DragStart &e) override {
        dragX = APP->scene->rack->mousePos.x;
        dragY = APP->scene->rack->mousePos.y;
    }

    void onDragMove(const event::DragMove &e) override {
        float newDragX = APP->scene->rack->mousePos.x;
        float newDragY = APP->scene->rack->mousePos.y;
        setProbabilities(initX + (newDragX - dragX), initY + (newDragY - dragY));
    }

    void setProbabilities(float currentX, float dragY) {
        int index = (int)(currentX / SLIDER_WIDTH);
        if (index >= NUM_OF_SLIDERS) index = NUM_OF_SLIDERS - 1;
        if (dragY < 0) dragY = 0;
        else if (dragY > box.size.y) dragY = box.size.y - SLIDER_TOP;
        module->seqs[seqId].gateProbabilities[index] = 1.0 - dragY / (box.size.y - SLIDER_TOP);
    }

    float getSliderHeight(int index) {
        float y = box.size.y - SLIDER_TOP;
        return y - (y * module->seqs[seqId].gateProbabilities[index]);
    }

    void draw(const DrawArgs& args) override {
        // TODO

        //background
        nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

        if (module == NULL) return;

        // sliders
        nvgStrokeColor(args.vg, nvgRGB(60, 70, 73));
        for (int i = 0; i < NUM_OF_SLIDERS; i++) {
            nvgStrokeWidth(args.vg, (i % 4 == 0 ? 2 : 0.5));
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, i * SLIDER_WIDTH, 0);
            nvgLineTo(args.vg, i * SLIDER_WIDTH, box.size.y);
            nvgStroke(args.vg);

            float sHeight = getSliderHeight(i);

            if (i < module->params[StochSeq4::LENGTH_PARAM+seqId].getValue()) {
                nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 191)); // bottoms
                nvgBeginPath(args.vg);
                nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, box.size.y - sHeight);
                nvgFill(args.vg);

                nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // tops
                nvgBeginPath(args.vg);
                nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, SLIDER_TOP);
                nvgFill(args.vg);
            } else {
                nvgFillColor(args.vg, nvgRGBA(150, 150, 150, 191)); // bottoms
                nvgBeginPath(args.vg);
                nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, box.size.y - sHeight);
                nvgFill(args.vg);

                nvgFillColor(args.vg, nvgRGB(150, 150, 150)); // tops
                nvgBeginPath(args.vg);
                nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, SLIDER_TOP);
                nvgFill(args.vg);
            }

        }

        // seq position
        if (module->seqs[seqId].gateIndex >= 0) {
            // TODO: set color for each sequencer
            nvgStrokeWidth(args.vg, 2.0);
            // nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
            switch (seqId) {
                case 0:
                    nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
                    break;
                case 1:
                    nvgStrokeColor(args.vg, nvgRGB(38, 0, 255));
                    break;
                case 2:
                    nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
                    break;
                case 3:
                    nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
                    break;
            }
            // nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
            nvgBeginPath(args.vg);
            nvgRect(args.vg, module->seqs[seqId].gateIndex * SLIDER_WIDTH, 1, SLIDER_WIDTH, box.size.y - 1);
            nvgStroke(args.vg);
        }

        // faded out non-pattern
        if (module->params[StochSeq4::LENGTH_PARAM+seqId].getValue() < NUM_OF_SLIDERS) {
            float x = module->params[StochSeq4::LENGTH_PARAM+seqId].getValue() * SLIDER_WIDTH;
            nvgStrokeWidth(args.vg, 2.0);
            switch (seqId) {
            case 0:
                nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
                break;
            case 1:
                nvgStrokeColor(args.vg, nvgRGB(38, 0, 255));
                break;
            case 2:
                nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
                break;
            case 3:
                nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
                break;
            }
            // nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, x, 0);
            nvgLineTo(args.vg, x, box.size.y);
            nvgStroke(args.vg);

            nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 130));
            nvgBeginPath(args.vg);
            nvgRect(args.vg, x, 0, box.size.x - x, box.size.y);
            nvgFill(args.vg);
        }
    }
};

struct StochSeq4Widget : ModuleWidget {
    StochSeq4Widget(StochSeq4 *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/StochSeq4.svg")));

        StochSeq4Display *displayPurple = new StochSeq4Display();
        displayPurple->module = module;
        displayPurple->seqId = StochSeq4::PURPLE_SEQ;
        displayPurple->box.pos = Vec(189.5, 18.2);
        displayPurple->box.size = Vec(480, 80.7);
        addChild(displayPurple);

        StochSeq4Display *displayBlue = new StochSeq4Display();
        displayBlue->module = module;
        displayBlue->seqId = StochSeq4::BLUE_SEQ;
        displayBlue->box.pos = Vec(189.5, 105.7);
        displayBlue->box.size = Vec(480, 80.7);
        addChild(displayBlue);

        StochSeq4Display *displayAqua = new StochSeq4Display();
        displayAqua->module = module;
        displayAqua->seqId = StochSeq4::AQUA_SEQ;
        displayAqua->box.pos = Vec(189.5, 193.2);
        displayAqua->box.size = Vec(480, 80.7);
        addChild(displayAqua);

        StochSeq4Display *displayRed = new StochSeq4Display();
        displayRed->module = module;
        displayRed->seqId = StochSeq4::RED_SEQ;
        displayRed->box.pos = Vec(189.5, 280.6);
        displayRed->box.size = Vec(480, 80.7);
        addChild(displayRed);

        // lengths
        addParam(createParamCentered<PurpleInvertKnob>(Vec(24.3, 104.8), module, StochSeq4::LENGTH_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<BlueInvertKnob>(Vec(24.3, 176.1), module, StochSeq4::LENGTH_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<AquaInvertKnob>(Vec(24.3, 247.3), module, StochSeq4::LENGTH_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<RedInvertKnob>(Vec(24.3, 317.9), module, StochSeq4::LENGTH_PARAM + StochSeq4::RED_SEQ));

        // patterns
        addParam(createParamCentered<PurpleInvertKnob>(Vec(58.8, 104.8), module, StochSeq4::PATTERN_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<BlueInvertKnob>(Vec(58.8, 176.1), module, StochSeq4::PATTERN_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<AquaInvertKnob>(Vec(58.8, 247.3), module, StochSeq4::PATTERN_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<RedInvertKnob>(Vec(58.8, 317.9), module, StochSeq4::PATTERN_PARAM + StochSeq4::RED_SEQ));

        // randoms
        addParam(createParamCentered<TinyPurpleButton>(Vec(93.4, 104.8), module, StochSeq4::RANDOM_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueButton>(Vec(93.4, 176.1), module, StochSeq4::RANDOM_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaButton>(Vec(93.4, 247.3), module, StochSeq4::RANDOM_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedButton>(Vec(93.4, 317.9), module, StochSeq4::RANDOM_PARAM + StochSeq4::RED_SEQ));
        // inverts
        addParam(createParamCentered<TinyPurpleButton>(Vec(127.9, 104.8), module, StochSeq4::INVERT_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueButton>(Vec(127.9, 176.1), module, StochSeq4::INVERT_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaButton>(Vec(127.9, 247.3), module, StochSeq4::INVERT_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedButton>(Vec(127.9, 317.9), module, StochSeq4::INVERT_PARAM + StochSeq4::RED_SEQ));
        // diminish
        addParam(createParamCentered<TinyPurpleButton>(Vec(162.4, 104.8), module, StochSeq4::DIMINUTION_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueButton>(Vec(162.4, 176.1), module, StochSeq4::DIMINUTION_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaButton>(Vec(162.4, 247.3), module, StochSeq4::DIMINUTION_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedButton>(Vec(162.4, 317.9), module, StochSeq4::DIMINUTION_PARAM + StochSeq4::RED_SEQ));
        // spreads
        addParam(createParamCentered<TinyPurpleKnob>(Vec(705.6, 72.5), module, StochSeq4::SPREAD_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueKnob>(Vec(705.6, 159.9), module, StochSeq4::SPREAD_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaKnob>(Vec(705.6, 247.4), module, StochSeq4::SPREAD_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedKnob>(Vec(705.6, 334.9), module, StochSeq4::SPREAD_PARAM + StochSeq4::RED_SEQ));

        addInput(createInputCentered<PJ301MPort>(Vec(126.9, 63.8), module, StochSeq4::CLOCK_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(161.5, 63.8), module, StochSeq4::RESET_INPUT));

        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(683.7, 91.3), module, StochSeq4::GATES_OUTPUT + StochSeq4::PURPLE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(705.6, 91.3), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::PURPLE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(683.7, 178.7), module, StochSeq4::GATES_OUTPUT + StochSeq4::BLUE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(705.6, 178.7), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::BLUE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(683.7, 266.2), module, StochSeq4::GATES_OUTPUT + StochSeq4::AQUA_SEQ));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(705.6, 266.2), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::AQUA_SEQ));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(683.7, 353.7), module, StochSeq4::GATES_OUTPUT + StochSeq4::RED_SEQ));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(705.6, 353.7), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::RED_SEQ));
    }
};

Model *modelStochSeq4 = createModel<StochSeq4, StochSeq4Widget>("StochSeq4");
