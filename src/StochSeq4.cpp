#include "plugin.hpp"

#define SLIDER_WIDTH 15
#define SLIDER_TOP 4
#define NUM_OF_SLIDERS 32
// #define NUM_OF_SEQS 4

struct Sequencer {
    dsp::SchmittTrigger dimTrig;
    dsp::SchmittTrigger randomTrig;
    dsp::SchmittTrigger invertTrig;
    bool gatePulse = false;
    int seqLength;
    int gateIndex;
    int currentPattern = 0;
    float volts = 0.0;
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
        gatePulse = false;
        // gate
        float prob = gateProbabilities[gateIndex];
        if (random::uniform() < prob) {
            gatePulse = true;
        }

        // pitch
        volts = prob * (2 * spread) - spread;
    }
};

struct StochSeq4 : Module, Quantize {
    enum SequencerIds {
        PURPLE_SEQ,
        BLUE_SEQ,
        AQUA_SEQ,
        RED_SEQ,
        NUM_SEQS
    };
	enum ParamIds {
        ROOT_NOTE_PARAM,
        SCALE_PARAM,
		PATTERN_PARAM = NUM_SEQS,
		RANDOM_PARAM = PATTERN_PARAM + NUM_SEQS,
		INVERT_PARAM = RANDOM_PARAM + NUM_SEQS,
		DIMINUTION_PARAM = INVERT_PARAM + NUM_SEQS,
		LENGTH_PARAM = DIMINUTION_PARAM + NUM_SEQS,
		SPREAD_PARAM = LENGTH_PARAM + NUM_SEQS,
		NUM_PARAMS = SPREAD_PARAM + NUM_SEQS
	};
	enum InputIds {
		MASTER_CLOCK_INPUT,
        CLOCKS_INPUT = MASTER_CLOCK_INPUT + NUM_SEQS,
		RESET_INPUT = CLOCKS_INPUT + NUM_SEQS,
        RANDOM_INPUT = RESET_INPUT + NUM_SEQS,
        INVERT_INPUT = RANDOM_INPUT + NUM_SEQS,
        DIMINUTION_INPUT = INVERT_INPUT + NUM_SEQS,
		NUM_INPUTS = DIMINUTION_INPUT + NUM_SEQS
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
    dsp::SchmittTrigger clockTriggers[NUM_SEQS];
    dsp::SchmittTrigger resetTrig;
    bool resetMode = false;
    Sequencer seqs[NUM_SEQS];

    StochSeq4() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(ROOT_NOTE_PARAM, 0.0, Quantize::NUM_OF_NOTES - 1, 0.0, "Root note");
        configParam(SCALE_PARAM, 0.0, Quantize::NUM_OF_SCALES, 0.0, "Scale");
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

        configParam(SPREAD_PARAM + PURPLE_SEQ, -4.0, 4.0, 1.0, "purple spread");
        configParam(SPREAD_PARAM + BLUE_SEQ, -4.0, 4.0, 1.0, "blue spread");
        configParam(SPREAD_PARAM + AQUA_SEQ, -4.0, 4.0, 1.0, "aqua spread");
        configParam(SPREAD_PARAM + RED_SEQ, -4.0, 4.0, 1.0, "red spread");
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_t *currentPatternsJ = json_array();
        json_t *seqsProbsJ = json_array();
        for (int i = 0; i < NUM_SEQS; i++) {
            json_t *currentPatternJ = json_integer(seqs[i].currentPattern);
            json_array_append_new(currentPatternsJ, currentPatternJ);

            json_t *probsJ = json_array();
            for (int j = 0; j < NUM_OF_SLIDERS; j++) {
                json_t *probJ = json_real(seqs[i].gateProbabilities[j]);
                json_array_append_new(probsJ, probJ);
            }
            json_array_append_new(seqsProbsJ, probsJ);
        }
        json_object_set_new(rootJ, "currentPatterns", currentPatternsJ);
        json_object_set_new(rootJ, "seqsProbs", seqsProbsJ);

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *currentPatternsJ = json_object_get(rootJ, "currentPatterns");
        json_t *seqsProbsJ = json_object_get(rootJ, "seqsProbs");
        if (currentPatternsJ) {
            for (int i = 0; i < NUM_SEQS; i++) {
                json_t *currentPatternJ = json_array_get(currentPatternsJ, i);
                if (currentPatternJ) {
                    seqs[i].currentPattern = json_integer_value(currentPatternJ);
                }

                json_t *probsJ = json_array_get(seqsProbsJ, i);
                if (probsJ) {
                    for (int j = 0; j < NUM_OF_SLIDERS; j++) {
                        json_t *probJ = json_array_get(probsJ, j);
                        if (probJ) {
                            seqs[i].gateProbabilities[j] = json_real_value(probJ);
                        }
                    }
                }
            }
        }
    }

    void process(const ProcessArgs& args) override {
        if (resetTrig.process(inputs[RESET_INPUT].getVoltage())) {
            resetMode = true;
        }
        for (int i = 0; i < NUM_SEQS; i++) {
            if ((int)params[PATTERN_PARAM+i].getValue() != seqs[i].currentPattern) {
                int patt = (int)params[PATTERN_PARAM+i].getValue();
                seqs[i].currentPattern = patt;
                genPatterns(patt, i);
            }
            if (seqs[i].randomTrig.process(params[RANDOM_PARAM+i].getValue() + inputs[RANDOM_INPUT+i].getVoltage())) {
                genPatterns(100, i);
            }
            if (seqs[i].invertTrig.process(params[INVERT_PARAM+i].getValue() + inputs[INVERT_INPUT+i].getVoltage())) {
                invert(i);
            }
            if (seqs[i].dimTrig.process(params[DIMINUTION_PARAM+i].getValue() + inputs[DIMINUTION_INPUT+i].getVoltage())) {
                diminish(i);
            }
        }
        if (clockTrig.process(inputs[MASTER_CLOCK_INPUT].getVoltage())) {
            if (resetMode) {
                resetMode = false;
                resetSeqToEnd();
            }
            clockStep();
        } else {
            for (int i = 0; i < NUM_SEQS; i++) {
                if (clockTriggers[i].process(inputs[CLOCKS_INPUT+i].getVoltage())) {
                    if (resetMode) {
                        resetMode = false;
                        resetSeqToEnd();
                    }
                    clockStep(i);
                    // int l = (int)params[LENGTH_PARAM + i].getValue();
                    // float spread = params[SPREAD_PARAM + i].getValue();
                    // seqs[i].clockStep(l, spread);
                }
            }
        }
        int rootNote = params[ROOT_NOTE_PARAM].getValue();
        int scale = params[SCALE_PARAM].getValue();
        for (int i = 0; i < NUM_SEQS; i++) {
            float pitchVoltage = Quantize::quantizeRawVoltage(seqs[i].volts, rootNote, scale);

            outputs[GATES_OUTPUT + i].setVoltage(seqs[i].gatePulse ? 10.0 : 0.0);
            outputs[VOLTS_OUTPUT + i].setVoltage(pitchVoltage);
        }
    }

    void clockStep() {
        // MASTER clock step (all)
        for (int i = 0; i < NUM_SEQS; i++) {
            int l = (int)params[LENGTH_PARAM+i].getValue();
            float spread = params[SPREAD_PARAM+i].getValue();
            seqs[i].clockStep(l, spread);
        }
    }

    void clockStep(int i) {
        // individual clock steps
        int l = (int)params[LENGTH_PARAM + i].getValue();
        float spread = params[SPREAD_PARAM + i].getValue();
        seqs[i].clockStep(l, spread);
    }

    void resetSeqToEnd() {
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
        // nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        // nvgBeginPath(args.vg);
        // nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        // nvgFill(args.vg);

        if (module == NULL) {
            // draw stuff for preview
			nvgStrokeColor(args.vg, nvgRGB(60, 70, 73));
			for (int i = 0; i < NUM_OF_SLIDERS; i++) {
				// grid
				nvgStrokeWidth(args.vg, (i % 4 == 0 ? 2 : 0.5));
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, i * SLIDER_WIDTH, 0);
				nvgLineTo(args.vg, i * SLIDER_WIDTH, box.size.y);
				nvgStroke(args.vg);

				// random sliders
				float rHeight = (box.size.y-SLIDER_TOP) * random::uniform();
				nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 191)); // bottoms
				nvgBeginPath(args.vg);
				nvgRect(args.vg, i * SLIDER_WIDTH, rHeight, SLIDER_WIDTH, box.size.y - rHeight);
				nvgFill(args.vg);

				nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // tops
				nvgBeginPath(args.vg);
				nvgRect(args.vg, i * SLIDER_WIDTH, rHeight, SLIDER_WIDTH, SLIDER_TOP);
				nvgFill(args.vg);
			}
            return;
        }

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
        displayPurple->box.pos = Vec(211.1, 18.2);
        displayPurple->box.size = Vec(480, 80.7);
        addChild(displayPurple);

        StochSeq4Display *displayBlue = new StochSeq4Display();
        displayBlue->module = module;
        displayBlue->seqId = StochSeq4::BLUE_SEQ;
        displayBlue->box.pos = Vec(211.1, 105.7);
        displayBlue->box.size = Vec(480, 80.7);
        addChild(displayBlue);

        StochSeq4Display *displayAqua = new StochSeq4Display();
        displayAqua->module = module;
        displayAqua->seqId = StochSeq4::AQUA_SEQ;
        displayAqua->box.pos = Vec(211.1, 193.2);
        displayAqua->box.size = Vec(480, 80.7);
        addChild(displayAqua);

        StochSeq4Display *displayRed = new StochSeq4Display();
        displayRed->module = module;
        displayRed->seqId = StochSeq4::RED_SEQ;
        displayRed->box.pos = Vec(211.1, 280.6);
        displayRed->box.size = Vec(480, 80.7);
        addChild(displayRed);

        // addChild(createWidget<JeremyScrew>(Vec(47.7, 2)));
        addChild(createWidget<JeremyScrew>(Vec(47.7, box.size.y - 14)));
        addChild(createWidget<JeremyScrew>(Vec(660.3, 2)));
        addChild(createWidget<JeremyScrew>(Vec(660.3, box.size.y - 14)));

        // clock inputs
        addInput(createInputCentered<TinyPJ301M>(Vec(26.3, 104.8), module, StochSeq4::CLOCKS_INPUT + StochSeq4::PURPLE_SEQ));
        addInput(createInputCentered<TinyPJ301M>(Vec(26.3, 162.3), module, StochSeq4::CLOCKS_INPUT + StochSeq4::BLUE_SEQ));
        addInput(createInputCentered<TinyPJ301M>(Vec(26.3, 219.8), module, StochSeq4::CLOCKS_INPUT + StochSeq4::AQUA_SEQ));
        addInput(createInputCentered<TinyPJ301M>(Vec(26.3, 278), module, StochSeq4::CLOCKS_INPUT + StochSeq4::RED_SEQ));
        // lengths
        addParam(createParamCentered<PurpleInvertKnob>(Vec(54.8, 104.8), module, StochSeq4::LENGTH_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<BlueInvertKnob>(Vec(54.8, 162.3), module, StochSeq4::LENGTH_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<AquaInvertKnob>(Vec(54.8, 219.8), module, StochSeq4::LENGTH_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<RedInvertKnob>(Vec(54.8, 278), module, StochSeq4::LENGTH_PARAM + StochSeq4::RED_SEQ));
        // patterns
        addParam(createParamCentered<PurpleInvertKnob>(Vec(87.1, 104.8), module, StochSeq4::PATTERN_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<BlueInvertKnob>(Vec(87.1, 162.3), module, StochSeq4::PATTERN_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<AquaInvertKnob>(Vec(87.1, 219.8), module, StochSeq4::PATTERN_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<RedInvertKnob>(Vec(87.1, 278), module, StochSeq4::PATTERN_PARAM + StochSeq4::RED_SEQ));
        // randoms
        addParam(createParamCentered<TinyPurpleButton>(Vec(119.4, 104.8), module, StochSeq4::RANDOM_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueButton>(Vec(119.4, 162.3), module, StochSeq4::RANDOM_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaButton>(Vec(119.4, 219.8), module, StochSeq4::RANDOM_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedButton>(Vec(119.4, 278), module, StochSeq4::RANDOM_PARAM + StochSeq4::RED_SEQ));
        // inverts
        addParam(createParamCentered<TinyPurpleButton>(Vec(151.7, 104.8), module, StochSeq4::INVERT_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueButton>(Vec(151.7, 162.3), module, StochSeq4::INVERT_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaButton>(Vec(151.7, 219.8), module, StochSeq4::INVERT_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedButton>(Vec(151.7, 278), module, StochSeq4::INVERT_PARAM + StochSeq4::RED_SEQ));
        // diminish
        addParam(createParamCentered<TinyPurpleButton>(Vec(184, 104.8), module, StochSeq4::DIMINUTION_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueButton>(Vec(184, 162.3), module, StochSeq4::DIMINUTION_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaButton>(Vec(184, 219.8), module, StochSeq4::DIMINUTION_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedButton>(Vec(184, 278), module, StochSeq4::DIMINUTION_PARAM + StochSeq4::RED_SEQ));
        // spreads
        addParam(createParamCentered<TinyPurpleKnob>(Vec(750.6, 66.2), module, StochSeq4::SPREAD_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueKnob>(Vec(750.6, 153.7), module, StochSeq4::SPREAD_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaKnob>(Vec(750.6, 241.2), module, StochSeq4::SPREAD_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedKnob>(Vec(750.6, 328.6), module, StochSeq4::SPREAD_PARAM + StochSeq4::RED_SEQ));

        // note and scale knobs
        PurpleNoteKnob *noteKnob = dynamic_cast<PurpleNoteKnob *>(createParamCentered<PurpleNoteKnob>(Vec(26.3, 321), module, StochSeq4::ROOT_NOTE_PARAM));
        LeftAlignedLabel *const noteLabel = new LeftAlignedLabel;
        noteLabel->box.pos = Vec(46.2, 324.6);
        noteLabel->text = "";
        noteKnob->connectLabel(noteLabel, module);
        addChild(noteLabel);
        addParam(noteKnob);

        PurpleScaleKnob *scaleKnob = dynamic_cast<PurpleScaleKnob *>(createParamCentered<PurpleScaleKnob>(Vec(26.3, 349.6), module, StochSeq4::SCALE_PARAM));
        LeftAlignedLabel *const scaleLabel = new LeftAlignedLabel;
        scaleLabel->box.pos = Vec(46.2, 353.2);
        scaleLabel->text = "";
        scaleKnob->connectLabel(scaleLabel, module);
        addChild(scaleLabel);
        addParam(scaleKnob);

        addInput(createInputCentered<PJ301MPort>(Vec(148.8, 63.8), module, StochSeq4::MASTER_CLOCK_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(180.1, 63.8), module, StochSeq4::RESET_INPUT));
        for (int i = 0; i < 4; i++) {
            addInput(createInputCentered<TinyPJ301M>(Vec(119.4, 124.8 + (i * 57.3)), module, StochSeq4::RANDOM_INPUT + i));
            addInput(createInputCentered<TinyPJ301M>(Vec(151.7, 124.8 + (i * 57.3)), module, StochSeq4::INVERT_INPUT + i));
            addInput(createInputCentered<TinyPJ301M>(Vec(184, 124.8 + (i * 57.3)), module, StochSeq4::DIMINUTION_INPUT + i));
        }

        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(705.3, 66.2), module, StochSeq4::GATES_OUTPUT + StochSeq4::PURPLE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(728.7, 66.2), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::PURPLE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(705.3, 153.7), module, StochSeq4::GATES_OUTPUT + StochSeq4::BLUE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(728.7, 153.7), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::BLUE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(705.3, 241.2), module, StochSeq4::GATES_OUTPUT + StochSeq4::AQUA_SEQ));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(728.7, 241.2), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::AQUA_SEQ));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(705.3, 328.6), module, StochSeq4::GATES_OUTPUT + StochSeq4::RED_SEQ));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(728.7, 328.6), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::RED_SEQ));
    }
};

Model *modelStochSeq4 = createModel<StochSeq4, StochSeq4Widget>("StochSeq4");
