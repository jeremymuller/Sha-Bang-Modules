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
    dsp::PulseGenerator notGatePulse;
    int seqLength;
    int gateIndex;
    int currentPattern = 0;
    float volts = 0.0;
    float invVolts = 0.0;
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
        } else {
            notGatePulse.trigger(1e-3);
        }

        // pitch
        volts = prob * (2 * spread) - spread;
        invVolts = (1.0 - prob) * (2 * spread) - spread;
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
        OR_OUTPUT,
        XOR_OUTPUT,
		GATES_OUTPUT = XOR_OUTPUT + NUM_SEQS,
        NOT_GATES_OUTPUT = GATES_OUTPUT + NUM_SEQS,
		VOLTS_OUTPUT = NOT_GATES_OUTPUT + NUM_SEQS,
        INV_VOLTS_OUTPUT = VOLTS_OUTPUT + NUM_SEQS,
		NUM_OUTPUTS = INV_VOLTS_OUTPUT + NUM_SEQS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    dsp::SchmittTrigger clockTrig;
    dsp::SchmittTrigger clockTriggers[NUM_SEQS];
    dsp::SchmittTrigger resetTrig;
    bool resetMode = false;
    bool showPercentages = true;
    bool enableKBShortcuts = true;
    int focusedSeq = PURPLE_SEQ;
    Sequencer clipBoard;
    Sequencer seqs[NUM_SEQS];

    StochSeq4() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(ROOT_NOTE_PARAM, 0.0, Quantize::NUM_OF_NOTES - 1, 0.0, "Root note");
        configParam(SCALE_PARAM, 0.0, Quantize::NUM_OF_SCALES, 0.0, "Scale");
        configParam(LENGTH_PARAM + PURPLE_SEQ, 1.0, 32.0, 32.0, "Purple seq length");
        configParam(LENGTH_PARAM + BLUE_SEQ, 1.0, 32.0, 32.0, "Blue seq length");
        configParam(LENGTH_PARAM + AQUA_SEQ, 1.0, 32.0, 32.0, "Aqua seq length");
        configParam(LENGTH_PARAM + RED_SEQ, 1.0, 32.0, 32.0, "Red seq length");
        configParam(PATTERN_PARAM + PURPLE_SEQ, 0.0, 7.0, 0.0, "Purple pattern");
        configParam(PATTERN_PARAM + BLUE_SEQ, 0.0, 7.0, 0.0, "Blue pattern");
        configParam(PATTERN_PARAM + AQUA_SEQ, 0.0, 7.0, 0.0, "Aqua pattern");
        configParam(PATTERN_PARAM + RED_SEQ, 0.0, 7.0, 0.0, "Red pattern");

        configButton(RANDOM_PARAM + PURPLE_SEQ, "Randomize purple pattern");
        configButton(RANDOM_PARAM + BLUE_SEQ, "Randomize blue pattern");
        configButton(RANDOM_PARAM + AQUA_SEQ, "Randomize aqua pattern");
        configButton(RANDOM_PARAM + RED_SEQ, "Randomize red pattern");
        configButton(INVERT_PARAM + PURPLE_SEQ, "Invert purple pattern");
        configButton(INVERT_PARAM + BLUE_SEQ, "Invert blue pattern");
        configButton(INVERT_PARAM + AQUA_SEQ, "Invert aqua pattern");
        configButton(INVERT_PARAM + RED_SEQ, "Invert red pattern");
        configButton(DIMINUTION_PARAM + PURPLE_SEQ, "Diminish purple pattern");
        configButton(DIMINUTION_PARAM + BLUE_SEQ, "Diminish blue pattern");
        configButton(DIMINUTION_PARAM + AQUA_SEQ, "Diminish aqua pattern");
        configButton(DIMINUTION_PARAM + RED_SEQ, "Diminish red pattern");

        configParam(SPREAD_PARAM + PURPLE_SEQ, -4.0, 4.0, 1.0, "Purple spread");
        configParam(SPREAD_PARAM + BLUE_SEQ, -4.0, 4.0, 1.0, "Blue spread");
        configParam(SPREAD_PARAM + AQUA_SEQ, -4.0, 4.0, 1.0, "Aqua spread");
        configParam(SPREAD_PARAM + RED_SEQ, -4.0, 4.0, 1.0, "Red spread");

        configInput(MASTER_CLOCK_INPUT, "Master clock");
        configInput(RESET_INPUT, "Reset");
        configInput(CLOCKS_INPUT + PURPLE_SEQ, "Purple clock");
        configInput(CLOCKS_INPUT + BLUE_SEQ, "Blue clock");
        configInput(CLOCKS_INPUT + AQUA_SEQ, "Aqua clock");
        configInput(CLOCKS_INPUT + RED_SEQ, "Red clock");
        configInput(RANDOM_INPUT + PURPLE_SEQ, "Randomize purple pattern");
        configInput(RANDOM_INPUT + BLUE_SEQ, "Randomize blue pattern");
        configInput(RANDOM_INPUT + AQUA_SEQ, "Randomize aqua pattern");
        configInput(RANDOM_INPUT + RED_SEQ, "Randomize red pattern");
        configInput(INVERT_INPUT + PURPLE_SEQ, "Invert purple pattern");
        configInput(INVERT_INPUT + BLUE_SEQ, "Invert blue pattern");
        configInput(INVERT_INPUT + AQUA_SEQ, "Invert aqua pattern");
        configInput(INVERT_INPUT + RED_SEQ, "Invert red pattern");
        configInput(DIMINUTION_INPUT + PURPLE_SEQ, "Diminish purple pattern");
        configInput(DIMINUTION_INPUT + BLUE_SEQ, "Diminish blue pattern");
        configInput(DIMINUTION_INPUT + AQUA_SEQ, "Diminish aqua pattern");
        configInput(DIMINUTION_INPUT + RED_SEQ, "Diminish red pattern");



        configOutput(OR_OUTPUT, "Or");
        configOutput(XOR_OUTPUT, "Xor");

        configOutput(GATES_OUTPUT + PURPLE_SEQ, "Purple Gates");
        configOutput(GATES_OUTPUT + BLUE_SEQ, "Blue Gates");
        configOutput(GATES_OUTPUT + AQUA_SEQ, "Aqua Gates");
        configOutput(GATES_OUTPUT + RED_SEQ, "Red Gates");
        configOutput(NOT_GATES_OUTPUT + PURPLE_SEQ, "Purple Not Gates");
        configOutput(NOT_GATES_OUTPUT + BLUE_SEQ, "Blue Not Gates");
        configOutput(NOT_GATES_OUTPUT + AQUA_SEQ, "Aqua Not Gates");
        configOutput(NOT_GATES_OUTPUT + RED_SEQ, "Red Not Gates");

        // configOutput(INV_VOLT_OUTPUT, "Inverted Pitch (V/OCT)");
        configOutput(VOLTS_OUTPUT + PURPLE_SEQ, "Purple Pitch (V/OCT)");
        configOutput(VOLTS_OUTPUT + BLUE_SEQ, "Blue Pitch (V/OCT)");
        configOutput(VOLTS_OUTPUT + AQUA_SEQ, "Aqua Pitch (V/OCT)");
        configOutput(VOLTS_OUTPUT + RED_SEQ, "Red Pitch (V/OCT)");
        configOutput(INV_VOLTS_OUTPUT + PURPLE_SEQ, "Purple Inverted Pitch (V/OCT)");
        configOutput(INV_VOLTS_OUTPUT + BLUE_SEQ, "Blue Inverted Pitch (V/OCT)");
        configOutput(INV_VOLTS_OUTPUT + AQUA_SEQ, "Aqua Inverted Pitch (V/OCT)");
        configOutput(INV_VOLTS_OUTPUT + RED_SEQ, "Red Inverted Pitch (V/OCT)");
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
        json_object_set_new(rootJ, "percentages", json_boolean(showPercentages));
        json_object_set_new(rootJ, "kbshortcuts", json_boolean(enableKBShortcuts));
        json_object_set_new(rootJ, "focusId", json_integer(focusedSeq));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *percentagesJ = json_object_get(rootJ, "percentages");
        if (percentagesJ) showPercentages = json_boolean_value(percentagesJ);

		json_t *kbshortcutsJ = json_object_get(rootJ, "kbshortcuts");
		if (kbshortcutsJ) enableKBShortcuts = json_boolean_value(kbshortcutsJ);

        json_t *focusIdJ = json_object_get(rootJ, "focusId");
        if (focusIdJ) focusedSeq = json_integer_value(focusIdJ);

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
        bool orGate = false;
        int xorGate = 0;
        for (int i = 0; i < NUM_SEQS; i++) {
            float pitchVoltage = Quantize::quantizeRawVoltage(seqs[i].volts, rootNote, scale);
            float invPitchVoltage = Quantize::quantizeRawVoltage(seqs[i].invVolts, rootNote, scale);
            bool pulse = seqs[i].gatePulse.process(1.0 / args.sampleRate);
            bool notPulse = seqs[i].notGatePulse.process(1.0 / args.sampleRate);
            if (pulse) {
                orGate = true;
                xorGate++;
            }
            
            outputs[GATES_OUTPUT + i].setVoltage(pulse ? 10.0 : 0.0);
            outputs[NOT_GATES_OUTPUT + i].setVoltage(notPulse ? 10.0 : 0.0);
            outputs[VOLTS_OUTPUT + i].setVoltage(pitchVoltage);
            outputs[INV_VOLTS_OUTPUT + i].setVoltage(invPitchVoltage);
        }

        outputs[OR_OUTPUT].setVoltage(orGate ? 10.0 : 0.0);
        outputs[XOR_OUTPUT].setVoltage((xorGate == 1) ? 10.0 : 0.0);
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

    void copyPatternToClipBoard() {
        for (int i = 0; i < NUM_OF_SLIDERS; i++) {
            clipBoard.gateProbabilities[i] = seqs[focusedSeq].gateProbabilities[i];
            
        }
        clipBoard.seqLength = params[LENGTH_PARAM + focusedSeq].getValue();
    }

    void pastePattern() {
        for (int i = 0; i < NUM_OF_SLIDERS; i++) {
            seqs[focusedSeq].gateProbabilities[i] = clipBoard.gateProbabilities[i];
        }
        params[LENGTH_PARAM + focusedSeq].setValue(clipBoard.seqLength);
    }

    void shiftFocus() {
        focusedSeq = (focusedSeq + 1) % NUM_SEQS;
    }

    void shiftPatternLeft(int id) {
		float temp = seqs[id].gateProbabilities[0]; // this goes at the end
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			if (i == NUM_OF_SLIDERS-1) {
				seqs[id].gateProbabilities[i] = temp;
			} else {
				seqs[id].gateProbabilities[i] = seqs[id].gateProbabilities[i+1];
			}
		}
	}

	void shiftPatternRight(int id) {
		float temp = seqs[id].gateProbabilities[NUM_OF_SLIDERS-1]; // this goes at the beginning
		for (int i = NUM_OF_SLIDERS-1; i >= 0; i--) {
			if (i == 0) {
				seqs[id].gateProbabilities[i] = temp;
			} else {
                seqs[id].gateProbabilities[i] = seqs[id].gateProbabilities[i - 1];
            }
		}
	}

	void shiftPatternUp(int id) {
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			seqs[id].gateProbabilities[i] += 0.05;
            seqs[id].gateProbabilities[i] = clamp(seqs[id].gateProbabilities[i], 0.0, 1.0);
        }
	}

	void shiftPatternDown(int id) {
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			seqs[id].gateProbabilities[i] -= 0.05;
            seqs[id].gateProbabilities[i] = clamp(seqs[id].gateProbabilities[i], 0.0, 1.0);
        }
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

namespace StochSeq4NS {
	struct ShowTextValueItem : MenuItem {
        StochSeq4 *module;
        bool showPercentages;
        void onAction(const event::Action &e) override {
            module->showPercentages = showPercentages;
        }
    };

    struct ShowTextItem : MenuItem {
        StochSeq4 *module;
        Menu *createChildMenu() override {
            Menu *menu = new Menu;
            std::vector<std::string> percentages = {"show", "hide"};
            for (int i = 0; i < 2; i++) {
                ShowTextValueItem *item = new ShowTextValueItem;
                item->text = percentages[i];
                bool isOn = (i == 0) ? true : false;
                item->rightText = CHECKMARK(module->showPercentages == isOn);
                item->module = module;
                item->showPercentages = isOn;
                menu->addChild(item);
            }
            return menu;
        }
    };

	struct EnableShortcutsValueItem : MenuItem {
		StochSeq4 *module;
		bool enableKBShortcuts;
		void onAction(const event::Action &e) override {
			module->enableKBShortcuts = enableKBShortcuts;
		}
	};

	struct EnableShortcutsItem : MenuItem {
		StochSeq4 *module;
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			std::vector<std::string> enabled = {"on", "off"};
            for (int i = 0; i < 2; i++) {
                EnableShortcutsValueItem *item = new EnableShortcutsValueItem;
                item->text = enabled[i];
                bool isOn = (i == 0) ? true : false;
                item->rightText = CHECKMARK(module->enableKBShortcuts == isOn);
                item->module = module;
				item->enableKBShortcuts = isOn;
				menu->addChild(item);
            }
            return menu;
		}
	};
}

struct StochSeq4Display : Widget {
    StochSeq4 *module;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;
    float sliderWidth = SLIDER_WIDTH;
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
        dragX = APP->scene->rack->getMousePos().x;
        dragY = APP->scene->rack->getMousePos().y;
    }

    void onDragMove(const event::DragMove &e) override {
        float newDragX = APP->scene->rack->getMousePos().x;
        float newDragY = APP->scene->rack->getMousePos().y;
        setProbabilities(initX + (newDragX - dragX), initY + (newDragY - dragY));
    }

    void setProbabilities(float currentX, float dragY) {
        if (currentX < 0) currentX = 0;
        int index = (int)(currentX / sliderWidth);
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
        int visibleSliders = (int)module->params[StochSeq4::LENGTH_PARAM+seqId].getValue();
        sliderWidth = box.size.x / (float)visibleSliders;
        for (int i = 0; i < visibleSliders; i++) {
            nvgStrokeWidth(args.vg, (i % 4 == 0 ? 2 : 0.5));
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, i * sliderWidth, 0);
            nvgLineTo(args.vg, i * sliderWidth, box.size.y);
            nvgStroke(args.vg);

            float sHeight = getSliderHeight(i);

            nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 191)); // bottoms
            nvgBeginPath(args.vg);
            nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, box.size.y - sHeight);
            nvgFill(args.vg);

            nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // tops
            nvgBeginPath(args.vg);
            nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, SLIDER_TOP);
            nvgFill(args.vg);

            // if (i < module->params[StochSeq4::LENGTH_PARAM+seqId].getValue()) {
            //     nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 191)); // bottoms
            //     nvgBeginPath(args.vg);
            //     nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, box.size.y - sHeight);
            //     nvgFill(args.vg);

            //     nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // tops
            //     nvgBeginPath(args.vg);
            //     nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, SLIDER_TOP);
            //     nvgFill(args.vg);
            // } else {
            //     nvgFillColor(args.vg, nvgRGBA(150, 150, 150, 191)); // bottoms
            //     nvgBeginPath(args.vg);
            //     nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, box.size.y - sHeight);
            //     nvgFill(args.vg);

            //     nvgFillColor(args.vg, nvgRGB(150, 150, 150)); // tops
            //     nvgBeginPath(args.vg);
            //     nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, SLIDER_TOP);
            //     nvgFill(args.vg);
            // }

            // text
            if (module->showPercentages) {
                nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
                nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                nvgFontSize(args.vg, 9 + (sliderWidth / SLIDER_WIDTH));
                float w = i * sliderWidth;
                float yText = sHeight;
                if (sHeight < SLIDER_TOP + 3) {
                    yText = (SLIDER_TOP * 2) + sHeight + 3;
                    nvgFillColor(args.vg, nvgRGB(0, 0, 0));
                }
                std::string probText = std::to_string(static_cast<int>(module->seqs[seqId].gateProbabilities[i] * 100));
                nvgText(args.vg, w + sliderWidth/2.0, yText, probText.c_str(), NULL);
            }

        }

        // seq position
        if (module->seqs[seqId].gateIndex >= 0) {
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
            nvgRect(args.vg, module->seqs[seqId].gateIndex * sliderWidth, 1, sliderWidth, box.size.y - 1);
            nvgStroke(args.vg);
        }

        // focus rect
        if (module->enableKBShortcuts) {
            if (module->focusedSeq == seqId) {
                nvgStrokeWidth(args.vg, 4.0);
                // nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
                switch (seqId) {
                    case 0:
                        nvgStrokeColor(args.vg, nvgRGBA(128, 0, 219, 100));
                        break;
                    case 1:
                        nvgStrokeColor(args.vg, nvgRGBA(38, 0, 255, 100));
                        break;
                    case 2:
                        nvgStrokeColor(args.vg, nvgRGBA(0, 238, 255, 100));
                        break;
                    case 3:
                        nvgStrokeColor(args.vg, nvgRGBA(255, 0, 0, 100));
                        break;
                }
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
                nvgStroke(args.vg);
            }
        }

        /***** OLD STUFF *****/
        // faded out non-pattern
        // if (module->params[StochSeq4::LENGTH_PARAM+seqId].getValue() < NUM_OF_SLIDERS) {
        //     float x = module->params[StochSeq4::LENGTH_PARAM+seqId].getValue() * SLIDER_WIDTH;
        //     nvgStrokeWidth(args.vg, 2.0);
        //     switch (seqId) {
        //     case 0:
        //         nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
        //         break;
        //     case 1:
        //         nvgStrokeColor(args.vg, nvgRGB(38, 0, 255));
        //         break;
        //     case 2:
        //         nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
        //         break;
        //     case 3:
        //         nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
        //         break;
        //     }
        //     // nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
        //     nvgBeginPath(args.vg);
        //     nvgMoveTo(args.vg, x, 0);
        //     nvgLineTo(args.vg, x, box.size.y);
        //     nvgStroke(args.vg);

        //     nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 130));
        //     nvgBeginPath(args.vg);
        //     nvgRect(args.vg, x, 0, box.size.x - x, box.size.y);
        //     nvgFill(args.vg);
        // }
    }
};

struct StochSeq4Widget : ModuleWidget {
    StochSeq4Widget(StochSeq4 *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/StochSeq4.svg")));

        StochSeq4Display *displayPurple = new StochSeq4Display();
        displayPurple->module = module;
        displayPurple->seqId = StochSeq4::PURPLE_SEQ;
        displayPurple->box.pos = Vec(277.6, 18.2);
        displayPurple->box.size = Vec(480, 80.7);
        addChild(displayPurple);

        StochSeq4Display *displayBlue = new StochSeq4Display();
        displayBlue->module = module;
        displayBlue->seqId = StochSeq4::BLUE_SEQ;
        displayBlue->box.pos = Vec(277.6, 105.7);
        displayBlue->box.size = Vec(480, 80.7);
        addChild(displayBlue);

        StochSeq4Display *displayAqua = new StochSeq4Display();
        displayAqua->module = module;
        displayAqua->seqId = StochSeq4::AQUA_SEQ;
        displayAqua->box.pos = Vec(277.6, 193.2);
        displayAqua->box.size = Vec(480, 80.7);
        addChild(displayAqua);

        StochSeq4Display *displayRed = new StochSeq4Display();
        displayRed->module = module;
        displayRed->seqId = StochSeq4::RED_SEQ;
        displayRed->box.pos = Vec(277.6, 280.6);
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
        addParam(createParamCentered<TinyPurpleKnob>(Vec(264.6, 114.8), module, StochSeq4::SPREAD_PARAM + StochSeq4::PURPLE_SEQ));
        addParam(createParamCentered<TinyBlueKnob>(Vec(264.6, 172.2), module, StochSeq4::SPREAD_PARAM + StochSeq4::BLUE_SEQ));
        addParam(createParamCentered<TinyAquaKnob>(Vec(264.6, 229.5), module, StochSeq4::SPREAD_PARAM + StochSeq4::AQUA_SEQ));
        addParam(createParamCentered<TinyRedKnob>(Vec(264.6, 286.9), module, StochSeq4::SPREAD_PARAM + StochSeq4::RED_SEQ));

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

        addInput(createInputCentered<PJ301MPort>(Vec(151.7, 63.8), module, StochSeq4::MASTER_CLOCK_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(184, 63.8), module, StochSeq4::RESET_INPUT));
        for (int i = 0; i < 4; i++) {
            addInput(createInputCentered<TinyPJ301M>(Vec(119.4, 124.8 + (i * 57.3)), module, StochSeq4::RANDOM_INPUT + i));
            addInput(createInputCentered<TinyPJ301M>(Vec(151.7, 124.8 + (i * 57.3)), module, StochSeq4::INVERT_INPUT + i));
            addInput(createInputCentered<TinyPJ301M>(Vec(184, 124.8 + (i * 57.3)), module, StochSeq4::DIMINUTION_INPUT + i));
        }

        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(219.3, 104.8), module, StochSeq4::GATES_OUTPUT + StochSeq4::PURPLE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(219.3, 124.8), module, StochSeq4::NOT_GATES_OUTPUT + StochSeq4::PURPLE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(242.7, 104.8), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::PURPLE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(242.7, 124.8), module, StochSeq4::INV_VOLTS_OUTPUT + StochSeq4::PURPLE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(219.3, 162.2), module, StochSeq4::GATES_OUTPUT + StochSeq4::BLUE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(219.3, 182.1), module, StochSeq4::NOT_GATES_OUTPUT + StochSeq4::BLUE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(242.7, 162.2), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::BLUE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(242.7, 182.1), module, StochSeq4::INV_VOLTS_OUTPUT + StochSeq4::BLUE_SEQ));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(219.3, 219.6), module, StochSeq4::GATES_OUTPUT + StochSeq4::AQUA_SEQ));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(219.3, 239.5), module, StochSeq4::NOT_GATES_OUTPUT + StochSeq4::AQUA_SEQ));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(242.7, 219.6), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::AQUA_SEQ));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(242.7, 239.5), module, StochSeq4::INV_VOLTS_OUTPUT + StochSeq4::AQUA_SEQ));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(219.3, 276.9), module, StochSeq4::GATES_OUTPUT + StochSeq4::RED_SEQ));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(219.3, 296.9), module, StochSeq4::NOT_GATES_OUTPUT + StochSeq4::RED_SEQ));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(242.7, 276.9), module, StochSeq4::VOLTS_OUTPUT + StochSeq4::RED_SEQ));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(242.7, 296.9), module, StochSeq4::INV_VOLTS_OUTPUT + StochSeq4::RED_SEQ));

        // or, xor
        addOutput(createOutputCentered<PJ301MPort>(Vec(226.4, 345.4), module, StochSeq4::OR_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(254.9, 345.4), module, StochSeq4::XOR_OUTPUT));
    }

	void appendContextMenu(Menu *menu) override {
		StochSeq4 *module = dynamic_cast<StochSeq4*>(this->module);
		menu->addChild(new MenuEntry);

		StochSeq4NS::ShowTextItem *showTextItem = new StochSeq4NS::ShowTextItem;
		showTextItem->text = "Slider Percentages";
		if (module->showPercentages) showTextItem->rightText = std::string("show") + " " + RIGHT_ARROW;
		else showTextItem->rightText = std::string("hide") + " " + RIGHT_ARROW;
		showTextItem->module = module;
		menu->addChild(showTextItem);

        StochSeq4NS::EnableShortcutsItem *enableShortcutItem = new StochSeq4NS::EnableShortcutsItem;
		enableShortcutItem->text = "Keyboard Shortcuts";
		if (module->enableKBShortcuts) enableShortcutItem->rightText = std::string("on") + " " + RIGHT_ARROW;
		else enableShortcutItem->rightText = std::string("off") + " " + RIGHT_ARROW;
		enableShortcutItem->module = module;
		menu->addChild(enableShortcutItem);
	}

    void onHoverKey(const event::HoverKey &e) override {
        StochSeq4 *module = dynamic_cast<StochSeq4 *>(this->module);
		if (!module->enableKBShortcuts) {
			ModuleWidget::onHoverKey(e);
			return;
		}

        if (e.key == GLFW_KEY_ENTER && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
            e.consume(this);
            if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
                module->shiftFocus();
            }
        } else if (e.key == GLFW_KEY_LEFT && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			e.consume(this);
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
				module->shiftPatternLeft(module->focusedSeq);
			}
		} else if (e.key == GLFW_KEY_RIGHT && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			e.consume(this);
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
                module->shiftPatternRight(module->focusedSeq);
            }
		} else if (e.key == GLFW_KEY_UP && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			e.consume(this);
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
                module->shiftPatternUp(module->focusedSeq);
            }
		} else if (e.key == GLFW_KEY_DOWN && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			e.consume(this);
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
                module->shiftPatternDown(module->focusedSeq);
            }
		} else if (e.key == GLFW_KEY_C && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
            e.consume(this);
            if (e.action == GLFW_PRESS) {
                module->copyPatternToClipBoard();
            }
        } else if (e.key == GLFW_KEY_V && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
            e.consume(this);
            if (e.action == GLFW_PRESS) {
                module->pastePattern();
            }
        } else {
			ModuleWidget::onHoverKey(e);
			// OpaqueWidget::onHoverKey(e);
		}
    } 

};

Model *modelStochSeq4 = createModel<StochSeq4, StochSeq4Widget>("StochSeq4");
