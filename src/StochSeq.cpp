#include "plugin.hpp"

#define SLIDER_WIDTH 15
#define SLIDER_TOP 4
#define NUM_OF_SLIDERS 32
#define NUM_OF_LIGHTS 32
#define NUM_OF_MEM_BANK 12

struct MemoryBank {
	bool isOn;
	int length;
	float *gateProbabilities = new float[NUM_OF_SLIDERS];

	MemoryBank() {
		isOn = false;
		length = NUM_OF_SLIDERS;

		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] = 0.0;
		}
	}

	~MemoryBank() {
		delete[] gateProbabilities;
	}

	void setGates(float *probs) {
		for (int i = 0; i < length; i++) {
			probs[i] = gateProbabilities[i];
		}
	}

	void setProbabilities(const float *probs, int size) {
		isOn = true;
		length = size;
		DEBUG("size: %d", size);
		DEBUG("length: %d", length);

		for (int i = 0; i < length; i++) {
			gateProbabilities[i] = probs[i];
		}
	}

	void clearBank() {
		isOn = false;
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] = 0.0;
		}
	}
};

struct StochSeq : Module, Quantize {
	enum ModeIds {
		GATE_MODE,
		TRIG_MODE,
		VOLT_INDEPENDENT_MODE = 0,
		VOLT_SAMPHOLD_MODE,
		NUM_MODES
	};
	enum ParamIds {
		RESET_PARAM,
		PATTERN_PARAM,
		RANDOM_PARAM,
		INVERT_PARAM,
		DIMINUTION_PARAM,
		LENGTH_PARAM,
		SPREAD_PARAM,
		ROOT_NOTE_PARAM,
        SCALE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		RANDOM_INPUT,
		INVERT_INPUT,
		DIMINUTION_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		MEM_BANK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT = NUM_OF_SLIDERS,
		GATE_MAIN_OUTPUT = GATES_OUTPUT + NUM_OF_SLIDERS,
		NOT_GATE_MAIN_OUTPUT,
		INV_VOLT_OUTPUT,
		VOLT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		BANG_LIGHTS = 4,
		LIGHTS = BANG_LIGHTS + NUM_OF_LIGHTS,
		NUM_LIGHTS = LIGHTS + NUM_OF_LIGHTS
	};

	dsp::SchmittTrigger clockTrig;
	dsp::SchmittTrigger resetTrig;
	dsp::SchmittTrigger dimTrig;
	dsp::SchmittTrigger randomTrig;
	dsp::SchmittTrigger invertTrig;
	dsp::PulseGenerator gatePulse;
	dsp::PulseGenerator notGatePulse;
	int gateMode = GATE_MODE;
	int voltMode = VOLT_INDEPENDENT_MODE;
	int voltRange = 1;
	int seqLength = NUM_OF_SLIDERS;
	int gateIndex = -1;
	int currentGateOut = gateIndex;
	int currentPattern = 0;
	bool gateOn = false;
	bool notGateOn = false;
	bool resetMode = false;
	bool lightBlink = false;
	bool showPercentages = true;
	int randLight;
	float pitchVoltage = 0.0;
	float invPitchVoltage = 0.0;
	float *gateProbabilities = new float[NUM_OF_SLIDERS];
	MemoryBank memBanks[NUM_OF_MEM_BANK];
	// float memBanks[NUM_OF_MEM_BANK][NUM_OF_SLIDERS] = {};
	int currentMemBank = 0;
	bool enableKBShortcuts = true;
	bool isCtrlClick = false;

	StochSeq() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configButton(RESET_PARAM, "Reset");
		configParam(PATTERN_PARAM, 0.0, 8.0, 0.0, "Pattern");
		configButton(INVERT_PARAM, "Invert pattern");
		configButton(RANDOM_PARAM, "Randomize pattern");
		configButton(DIMINUTION_PARAM, "Diminish pattern");
		configParam(LENGTH_PARAM, 1.0, 32.0, 32.0, "Seq length");
		configParam(SPREAD_PARAM, 0.0, 1.0, 0.1, "Volt scale", " %", 0, 100);
		configParam(ROOT_NOTE_PARAM, 0.0, Quantize::NUM_OF_NOTES - 1, 0.0, "Root note");
		configParam(SCALE_PARAM, 0.0, Quantize::NUM_OF_SCALES, 0.0, "Scale");

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(MEM_BANK_INPUT, "Memory Bank CV");
		configInput(INVERT_INPUT, "Invert pattern");
		configInput(RANDOM_INPUT, "Randomize pattern");
		configInput(DIMINUTION_INPUT, "Diminish pattern");

		configOutput(GATE_MAIN_OUTPUT, "Gates");
		configOutput(NOT_GATE_MAIN_OUTPUT, "Not Gates");
		configOutput(VOLT_OUTPUT, "Pitch (V/OCT)");
		configOutput(INV_VOLT_OUTPUT, "Inverted Pitch (V/OCT)");

		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] = random::uniform();
			configOutput(GATES_OUTPUT + i, "Gate " + std::to_string(i+1));
		}

		memBanks[currentMemBank].setProbabilities(gateProbabilities, seqLength);

		randLight = static_cast<int>(random::uniform() * NUM_OF_LIGHTS);
	}

	~StochSeq() {
		delete[] gateProbabilities;
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "currentPattern", json_integer(currentPattern));

		// memory bank stuff
        json_t *memBankProbsJ = json_array();
		json_t *lengthsJ = json_array();
		json_t *onJ = json_array();
        for (int i = 0; i < NUM_OF_MEM_BANK; i++) {
			json_t *lengthJ = json_integer(memBanks[i].length);
			json_array_append_new(lengthsJ, lengthJ);
			json_t *isOnJ = json_boolean(memBanks[i].isOn);
			json_array_append_new(onJ, isOnJ);

            json_t *probsJ = json_array();
            for (int j = 0; j < NUM_OF_SLIDERS; j++) {
                json_t *probJ = json_real(memBanks[i].gateProbabilities[j]);
                json_array_append_new(probsJ, probJ);
            }
            json_array_append_new(memBankProbsJ, probsJ);
        }

		json_t *probsJ = json_array();
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			json_t *probJ = json_real(gateProbabilities[i]);
			json_array_append_new(probsJ, probJ);
		}
		json_object_set_new(rootJ, "probs", probsJ);
		json_object_set_new(rootJ, "memBankProbs", memBankProbsJ);
		json_object_set_new(rootJ, "isOn", onJ);
		json_object_set_new(rootJ, "lengths", lengthsJ);
		json_object_set_new(rootJ, "currentMemBank", json_integer(currentMemBank));
		json_object_set_new(rootJ, "percentages", json_boolean(showPercentages));
		json_object_set_new(rootJ, "kbshortcuts", json_boolean(enableKBShortcuts));
		json_object_set_new(rootJ, "gateMode", json_integer(gateMode));
		json_object_set_new(rootJ, "voltMode", json_integer(voltMode));
		json_object_set_new(rootJ, "voltRange", json_integer(voltRange));

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
        json_t *percentagesJ = json_object_get(rootJ, "percentages");
        if (percentagesJ) showPercentages = json_boolean_value(percentagesJ);

		json_t *kbshortcutsJ = json_object_get(rootJ, "kbshortcuts");
		if (kbshortcutsJ) enableKBShortcuts = json_boolean_value(kbshortcutsJ);

        json_t *gateModeJ = json_object_get(rootJ, "gateMode");
        if (gateModeJ) gateMode = json_integer_value(gateModeJ);

		json_t *voltModeJ = json_object_get(rootJ, "voltMode");
		if (voltModeJ) voltMode = json_integer_value(voltModeJ);

		json_t *voltRangeJ = json_object_get(rootJ, "voltRange");
		if (voltRangeJ) voltRange = json_integer_value(voltRangeJ);

		json_t *currentPatternJ = json_object_get(rootJ, "currentPattern");
		if (currentPatternJ) currentPattern = json_integer_value(currentPatternJ);

		json_t *currentBankJ = json_object_get(rootJ, "currentMemBank");
		if (currentBankJ) currentMemBank = json_integer_value(currentBankJ);

		json_t *probsJ = json_object_get(rootJ, "probs");
		if (probsJ) {
			for (int i = 0; i < NUM_OF_SLIDERS; i++) {
				json_t *probJ = json_array_get(probsJ, i);
				if (probJ)
					gateProbabilities[i] = json_real_value(probJ);
			}
		}

		json_t *memBankProbsJ = json_object_get(rootJ, "memBankProbs");
		json_t *onJ = json_object_get(rootJ, "isOn");
		json_t *lengthsJ = json_object_get(rootJ, "lengths");
		if (memBankProbsJ) {
            for (int i = 0; i < NUM_OF_MEM_BANK; i++) {
				json_t *isOnJ = json_array_get(onJ, i);
				if (isOnJ)
					memBanks[i].isOn = json_boolean_value(isOnJ);

				json_t *lengthJ = json_array_get(lengthsJ, i);
				if (lengthJ)
					memBanks[i].length = json_integer_value(lengthJ);

				json_t *probsJ = json_array_get(memBankProbsJ, i);
				if (probsJ) {
                    for (int j = 0; j < NUM_OF_SLIDERS; j++) {
                        json_t *probJ = json_array_get(probsJ, j);
                        if (probJ) {
                            memBanks[i].gateProbabilities[j] = json_real_value(probJ);
                        }
                    }
                }
            }
        }
	}

	void process(const ProcessArgs& args) override {
		if (resetTrig.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
			resetMode = true;
		}
		if (inputs[MEM_BANK_INPUT].isConnected()) {
			float cv = inputs[MEM_BANK_INPUT].getVoltage();
			float whole = floor(cv);
			float cvDecimal = cv - whole;
			int bankId = (int)rescale(cvDecimal, 0.0, 1.0, 0.0, 12.0);
			params[LENGTH_PARAM].setValue(memBanks[bankId].length);
			memBanks[bankId].setGates(gateProbabilities);
			currentMemBank = bankId;
		}

		if ((int)params[PATTERN_PARAM].getValue() != currentPattern) {
			int patt = (int)params[PATTERN_PARAM].getValue();
			currentPattern = patt;
			genPatterns(patt);
		}
		if (randomTrig.process(params[RANDOM_PARAM].getValue() + inputs[RANDOM_INPUT].getVoltage())) {
			genPatterns(100);
		}
		if (invertTrig.process(params[INVERT_PARAM].getValue() + inputs[INVERT_INPUT].getVoltage())) {
			invert();
			memBanks[currentMemBank].setProbabilities(gateProbabilities, seqLength);
		}
		if (dimTrig.process(params[DIMINUTION_PARAM].getValue() + inputs[DIMINUTION_INPUT].getVoltage())) {
			diminish();
			memBanks[currentMemBank].setProbabilities(gateProbabilities, seqLength);
		}
		if (clockTrig.process(inputs[CLOCK_INPUT].getVoltage())) {
			if (resetMode) {
				resetMode = false;
				resetSeq();
			}
			clockStep();
		}

		bool gateVolt;
		bool notGateVolt;
		if (gateMode == GATE_MODE) {
			gateVolt = gateOn;
			notGateVolt = notGateOn;
		} else {
			gateVolt = gatePulse.process(1.0 / args.sampleRate);
			notGateVolt = notGatePulse.process(1.0 / args.sampleRate);
		}
		// float blink = lightBlink ? 1.0 : 0.0;
		outputs[GATES_OUTPUT + currentGateOut].setVoltage(gateVolt ? 10.0 : 0.0);
		outputs[GATE_MAIN_OUTPUT].setVoltage(gateVolt ? 10.0 : 0.0);
		outputs[NOT_GATE_MAIN_OUTPUT].setVoltage(notGateVolt ? 10.0 : 0.0);
		if (voltMode == VOLT_SAMPHOLD_MODE && gateVolt) {
			outputs[INV_VOLT_OUTPUT].setVoltage(invPitchVoltage);
			outputs[VOLT_OUTPUT].setVoltage(pitchVoltage);
		} else if (voltMode == VOLT_INDEPENDENT_MODE) {
			outputs[INV_VOLT_OUTPUT].setVoltage(invPitchVoltage);
			outputs[VOLT_OUTPUT].setVoltage(pitchVoltage);
		}
		// int randLight = int(random::uniform() * NUM_OF_LIGHTS);
		for (int i = 0; i < NUM_OF_LIGHTS; i++) {
			if (currentGateOut == i)
				lights[LIGHTS + i].setSmoothBrightness((lightBlink ? 1.0 : 0.0), args.sampleTime * 30);
			else
				lights[LIGHTS + i].setBrightness(0.0);			
		}

		int bangGate = currentGateOut % 4;
		for (int i = 0; i < 4; i++) {
			if (bangGate == i)
				lights[BANG_LIGHTS + i].setSmoothBrightness((lightBlink ? 1.0 : 0.0), args.sampleTime * 30);
			else
				lights[BANG_LIGHTS + i].setBrightness(0.0);
		}
	}

	void clockStep() {
		int rootNote = params[ROOT_NOTE_PARAM].getValue();
		int scale = params[SCALE_PARAM].getValue();

		seqLength = (int)params[LENGTH_PARAM].getValue();
		gateIndex = (gateIndex + 1) % seqLength;
		lightBlink = false;

		float prob = gateProbabilities[gateIndex];
		if (random::uniform() < prob) {
			gatePulse.trigger(1e-3);
			gateOn = true;
			notGateOn = false;
			currentGateOut = gateIndex;
			lightBlink = true;
			randLight = static_cast<int>(random::uniform() * NUM_OF_LIGHTS);
		} else {
			notGatePulse.trigger(1e-3);
			gateOn = false;
			notGateOn = true;
		}

		float voltScaled = params[SPREAD_PARAM].getValue();
		float voltShift = voltRange == 0 ? -5.0 : 0.0;
		prob *= 10.0; // scale up to 10V
		float volts = (prob + voltShift) * voltScaled;
		float invVolts = ((10.0 - prob) + voltShift) * voltScaled;
		pitchVoltage = Quantize::quantizeRawVoltage(volts, rootNote, scale);
		invPitchVoltage = Quantize::quantizeRawVoltage(invVolts, rootNote, scale);

		// pitchVoltage = prob * (2 * spread) - spread;
	}

	void resetSeq() {
		// gateIndex = seqLength-1;
		gateIndex = -1;
	}

	void invert() {
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			float currentProb = gateProbabilities[i];
			gateProbabilities[i] = 1.0 - currentProb;
		}
	}

	void diminish() {
		float *tempArray = new float[16];
		int index = 0;
		for (int i = 0; i < NUM_OF_SLIDERS; i += 2) {
			tempArray[index] = gateProbabilities[i];
			index++;
		}
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] = tempArray[i % 16];
		}
		delete[] tempArray;
	}

	void shiftPatternLeft() {
		float temp = gateProbabilities[0]; // this goes at the end
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			if (i == NUM_OF_SLIDERS-1) {
				gateProbabilities[i] = temp;
			} else {
				gateProbabilities[i] = gateProbabilities[i+1];
			}
		}

		memBanks[currentMemBank].setProbabilities(gateProbabilities, seqLength);
	}

	void shiftPatternRight() {
		float temp = gateProbabilities[NUM_OF_SLIDERS-1]; // this goes at the beginning
		for (int i = NUM_OF_SLIDERS-1; i >= 0; i--) {
			if (i == 0) {
				gateProbabilities[i] = temp;
			} else {
				gateProbabilities[i] = gateProbabilities[i-1];
			}
		}

		memBanks[currentMemBank].setProbabilities(gateProbabilities, seqLength);
	}

	void shiftPatternUp() {
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] += 0.05;
			gateProbabilities[i] = clamp(gateProbabilities[i], 0.0, 1.0);
		}

		memBanks[currentMemBank].setProbabilities(gateProbabilities, seqLength);
	}

	void shiftPatternDown() {
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] -= 0.05;
			gateProbabilities[i] = clamp(gateProbabilities[i], 0.0, 1.0);
		}

		memBanks[currentMemBank].setProbabilities(gateProbabilities, seqLength);
	}

	void genPatterns(int c) {
		switch (c) {
			case 0:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = 0.0;
				}
				break;
			case 1:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = 0.5;
				}
				break;
			case 2:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = 1.0;
				}
				break;
			case 3:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = (i % 8 == 0) ? 1.0 : 0.0;
				}
				break;
			case 4:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = (i % 8 == 4) ? 1.0 : 0.0;
				}
				break;
			case 5:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = (i % 4 == 0) ? 1.0 : 0.0;
				}
				break;
			case 6:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = (i + 1.0) / NUM_OF_SLIDERS;
				}
				break;
			case 7:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = std::sin(i / (NUM_OF_SLIDERS - 1.0) * M_PI);
				}
				break;
			case 8:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = (std::sin(i / (float)NUM_OF_SLIDERS * M_PI * 2)) * 0.5 + 0.5;
				}
				break;
			default:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = random::uniform();
				}
		}

		memBanks[currentMemBank].setProbabilities(gateProbabilities, seqLength);
	}
};

struct StochSeqDisplay : Widget {
	StochSeq *module;
	float initX = 0;
	float initY = 0;
	float dragX = 0;
	float dragY = 0;
	float sliderWidth = SLIDER_WIDTH;
	StochSeqDisplay() {}

	void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if ((e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
                module->isCtrlClick = true;
                e.consume(this);
                initX = e.pos.x;
                initY = e.pos.y;
                toggleProbabilities(initX);
            }
            else {
                module->isCtrlClick = false;
                e.consume(this);
                initX = e.pos.x;
                initY = e.pos.y;
                setProbabilities(initX, initY);
            }
        }
	}

	void onDragStart(const event::DragStart &e) override {
		dragX = APP->scene->rack->getMousePos().x;
		dragY = APP->scene->rack->getMousePos().y;
	}

	void onDragMove(const event::DragMove &e) override {
		if (!module->isCtrlClick) {
			float newDragX = APP->scene->rack->getMousePos().x;
			float newDragY = APP->scene->rack->getMousePos().y;
			setProbabilities(initX + (newDragX - dragX), initY + (newDragY - dragY));
		}
	}

	void setProbabilities(float currentX, float dragY) {
		int index = (int)(currentX / sliderWidth);
		if (index >= NUM_OF_SLIDERS) index = NUM_OF_SLIDERS - 1;
		if (dragY < 0) dragY = 0;
		else if (dragY > box.size.y) dragY = box.size.y - SLIDER_TOP;
		float prob = 1.0 - dragY / (box.size.y - SLIDER_TOP);
		module->gateProbabilities[index] = clamp(prob, 0.0, 1.0);
		int visibleSliders = (int)module->params[StochSeq::LENGTH_PARAM].getValue();
		module->memBanks[module->currentMemBank].setProbabilities(module->gateProbabilities, visibleSliders);
	}

	void toggleProbabilities(float currentX) {
        if (currentX < 0) currentX = 0;
        int index = (int)(currentX / sliderWidth);
        if (index >= NUM_OF_SLIDERS) index = NUM_OF_SLIDERS - 1;
        float p = module->gateProbabilities[index];
        module->gateProbabilities[index] = p < 0.5 ? 1.0 : 0.0;
		int visibleSliders = (int)module->params[StochSeq::LENGTH_PARAM].getValue();
		module->memBanks[module->currentMemBank].setProbabilities(module->gateProbabilities, visibleSliders);
	}

	float getSliderHeight(int index) {
		float y = box.size.y - SLIDER_TOP;
		return y - (y * module->gateProbabilities[index]);
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

				// sine wave sliders
				float sinHeight = (std::sin(i / (float)NUM_OF_SLIDERS * M_PI * 2)) * 0.5 + 0.5;
				float rHeight = (box.size.y-SLIDER_TOP) * (1 - sinHeight);
				// float rHeight = (box.size.y-SLIDER_TOP) * random::uniform();

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

		//background
		nvgFillColor(args.vg, nvgRGB(40, 40, 40));
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFill(args.vg);

		// sliders
		nvgStrokeColor(args.vg, nvgRGB(60, 70, 73));
		int visibleSliders = (int)module->params[StochSeq::LENGTH_PARAM].getValue();
		int mLength = module->memBanks[module->currentMemBank].length;
		if (mLength != visibleSliders)
			module->memBanks[module->currentMemBank].setProbabilities(module->gateProbabilities, visibleSliders);

		module->memBanks[module->currentMemBank].length = visibleSliders;
		sliderWidth = box.size.x / (float)visibleSliders;
		for (int i = 0; i < visibleSliders; i++) {
			nvgStrokeWidth(args.vg, (i % 4 == 0 ? 2 : 0.5));
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, i * sliderWidth, 0);
			nvgLineTo(args.vg, i * sliderWidth, box.size.y);
			nvgStroke(args.vg);
			float sHeight = getSliderHeight(i);

			// float sHeight = sliderHeights[i];

			nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 191)); // bottoms
			nvgBeginPath(args.vg);
			nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, box.size.y - sHeight);
			nvgFill(args.vg);

			nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // tops
			nvgBeginPath(args.vg);
			nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, SLIDER_TOP);
			nvgFill(args.vg);

			// percentage texts for each slider
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
				std::string probText = std::to_string(static_cast<int>(module->gateProbabilities[i] * 100));
				nvgText(args.vg, w + sliderWidth/2.0, yText, probText.c_str(), NULL);
			}
		}

	}

	void drawLayer(const DrawArgs& args, int layer) override {

		if (module == NULL) return;

		if (layer == 1) {

			// seq position
			if (module->gateIndex >= -1) {
				nvgStrokeWidth(args.vg, 2.0);
				// nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
				nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
				nvgBeginPath(args.vg);
				int pos = module->resetMode ? 0 : clamp(module->gateIndex, 0, NUM_OF_SLIDERS);
				float x = clamp(pos * sliderWidth, 0.0, box.size.x - sliderWidth);
				nvgRect(args.vg, x, 1, sliderWidth, box.size.y - 1);
				nvgStroke(args.vg);
			}

		}
		Widget::drawLayer(args, layer);

	}
};

struct MemoryBankDisplay : Widget {
	StochSeq *module;
	int bankId;
	float sliderWidth = 1.25;

	MemoryBankDisplay() {}

	void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.consume(this);
			module->params[StochSeq::LENGTH_PARAM].setValue(module->memBanks[bankId].length);
			module->memBanks[bankId].setGates(module->gateProbabilities);
			module->currentMemBank = bankId;

			// int visibleSliders = (int)module->params[StochSeq::LENGTH_PARAM].getValue();
			// module->memBanks[bankId].setProbabilities(module->gateProbabilities, visibleSliders);

			// select from bank
			// highlight too
        }
	}

	float getSliderHeight(int index) {
		float y = box.size.y;
		return y - (y * module->memBanks[bankId].gateProbabilities[index]);
	}

	void draw(const DrawArgs& args) override {

		if (module == NULL) {
			// draw for preview

			// border lines
			if (bankId < 11) {
				nvgStrokeColor(args.vg, nvgRGB(60, 70, 73));
				nvgStrokeWidth(args.vg, 1.5);
				nvgBeginPath(args.vg);
				nvgMoveTo(args.vg, box.size.x, 0);
				nvgLineTo(args.vg, box.size.x, box.size.y);
				nvgStroke(args.vg);
			}

			if (bankId == 0) {
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					float sinHeight = (std::sin(i / (float)NUM_OF_SLIDERS * M_PI * 2)) * 0.5 + 0.5;
					float rHeight = (box.size.y - SLIDER_TOP) * (1 - sinHeight);
					if (sinHeight > 0.0) {
						nvgBeginPath(args.vg);
						nvgRect(args.vg, i * sliderWidth, rHeight, sliderWidth, box.size.y - rHeight);
						nvgFill(args.vg);
					}
				}
			}


			return;
		}

		// border lines
		if (bankId < 11) {
			nvgStrokeColor(args.vg, nvgRGB(60, 70, 73));
			nvgStrokeWidth(args.vg, 1.5);
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, box.size.x, 0);
			nvgLineTo(args.vg, box.size.x, box.size.y);
			nvgStroke(args.vg);
		}

		if (module->memBanks[bankId].isOn) {
			// sliders
			nvgStrokeColor(args.vg, nvgRGB(60, 70, 73));
			sliderWidth = box.size.x / (float)module->memBanks[bankId].length;

			if (module->currentMemBank == bankId)
				nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // bars
			else
				nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 220)); // bars
			
			for (int i = 0; i < module->memBanks[bankId].length; i++) {
				if (module->memBanks[bankId].gateProbabilities[i] > 0.0) {
					float sHeight = getSliderHeight(i);
					nvgBeginPath(args.vg);
					nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, box.size.y - sHeight);
					nvgFill(args.vg);
				}
			}

		}
	}

	void drawLayer(const DrawArgs& args, int layer) override {

		if (module == NULL) return;

		if (layer == 1) {
			if (bankId != module->currentMemBank) {
				nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 120));
				nvgBeginPath(args.vg);
				nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
				nvgFill(args.vg);
			}
		}

		Widget::drawLayer(args, layer);
	}

};

struct StochSeqWidget : ModuleWidget {
	StochSeqWidget(StochSeq* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/StochSeq.svg")));

		StochSeqDisplay *display = new StochSeqDisplay();
		display->module = module;
		display->box.pos = Vec(7.4, 47.7);
		display->box.size = Vec(480, 102.9); // Vec(480, 141.9)
		addChild(display);

		for (int i = 0; i < NUM_OF_MEM_BANK; i++) {
			MemoryBankDisplay *memDisplay = new MemoryBankDisplay();
			memDisplay->module = module;
			memDisplay->bankId = i;
			memDisplay->box.pos = Vec(7.6 + (i * 40), 160.8);
			memDisplay->box.size = Vec(40, 28.9);
			addChild(memDisplay);
		}

		addChild(createWidget<JeremyScrew>(Vec(25.9, 2)));
		addChild(createWidget<JeremyScrew>(Vec(25.9, box.size.y-14)));
		addChild(createWidget<JeremyScrew>(Vec(457.1, 2)));
		addChild(createWidget<JeremyScrew>(Vec(457.1, box.size.y-14)));

		addParam(createParamCentered<TinyBlueButton>(Vec(71.4, 256.8), module, StochSeq::RESET_PARAM));
		addParam(createParamCentered<BlueInvertKnob>(Vec(105.9, 228.7), module, StochSeq::LENGTH_PARAM));
		addParam(createParamCentered<BlueInvertKnob>(Vec(140.5, 228.7), module, StochSeq::PATTERN_PARAM));
		addParam(createParamCentered<DefaultButton>(Vec(175, 228.7), module, StochSeq::RANDOM_PARAM));
		addParam(createParamCentered<DefaultButton>(Vec(209.5, 228.7), module, StochSeq::INVERT_PARAM));
		addParam(createParamCentered<DefaultButton>(Vec(244.1, 228.7), module, StochSeq::DIMINUTION_PARAM));
		addParam(createParamCentered<BlueKnob>(Vec(451.7, 256.8), module, StochSeq::SPREAD_PARAM));

		// note and scale knobs
		BlueNoteKnob *noteKnob = dynamic_cast<BlueNoteKnob *>(createParamCentered<BlueNoteKnob>(Vec(282, 228.7), module, StochSeq::ROOT_NOTE_PARAM));
		LeftAlignedLabel *const noteLabel = new LeftAlignedLabel;
		noteLabel->box.pos = Vec(297.9, 232.3);
		noteLabel->text = "";
		noteKnob->connectLabel(noteLabel, module);
		addChild(noteLabel);
		addParam(noteKnob);

		BlueScaleKnob *scaleKnob = dynamic_cast<BlueScaleKnob *>(createParamCentered<BlueScaleKnob>(Vec(282, 256.8), module, StochSeq::SCALE_PARAM));
		LeftAlignedLabel *const scaleLabel = new LeftAlignedLabel;
		scaleLabel->box.pos = Vec(297.9, 260.4);
		scaleLabel->text = "";
		scaleKnob->connectLabel(scaleLabel, module);
		addChild(scaleLabel);
		addParam(scaleKnob);

		addInput(createInputCentered<PJ301MPort>(Vec(175, 256.8), module, StochSeq::RANDOM_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(209.5, 256.8), module, StochSeq::INVERT_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(244.1, 256.8), module, StochSeq::DIMINUTION_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(36.9, 228.7), module, StochSeq::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(36.9, 256.8), module, StochSeq::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(71.4, 228.7), module, StochSeq::MEM_BANK_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(360.7, 228.7), module, StochSeq::GATE_MAIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(397.1, 228.7), module, StochSeq::NOT_GATE_MAIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(433.5, 228.7), module, StochSeq::INV_VOLT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(469.9, 228.7), module, StochSeq::VOLT_OUTPUT));

		// addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(84, 50), module, StochSeq::BLUE_LIGHT));

		// for (int i = 0; i < NUM_OF_LIGHTS; i++) {
		// 	float x = 196 * i / NUM_OF_LIGHTS + 5;
			
		// 	float y = ((-std::sin(2.0 * M_PI * i / NUM_OF_LIGHTS) * 0.5 + 0.5) * 50 + 15);
		// 	// float x = random::uniform() * 260 + 1;
		// 	// float y = random::uniform() * 50 + 15;
		// 	// int light = int(random::uniform() * 4);

		// 	int light = int(i / (NUM_OF_LIGHTS/4.0));

		// 	switch(light) {
		// 		case 0:
		// 			addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));
		// 			break;
		// 		case 1:
		// 			addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));
		// 			break;
		// 		case 2:
		// 			addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));
		// 			break;
		// 		case 3:
		// 			addChild(createLight<SmallLight<JeremyRedLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));
		// 			break;
		// 	}

		// 	// if (random::uniform() < 0.5)
		// 		// addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));

		// 	// addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(84, 50), module, StochSeq::BLUE_LIGHT));

		// }

		addChild(createLight<SmallLight<JeremyPurpleLight> >(Vec(241.1 - 3, 343.2 - 3), module, StochSeq::BANG_LIGHTS + 0));
		addChild(createLight<SmallLight<JeremyBlueLight> >(Vec(253.7 - 3, 343.2 - 3), module, StochSeq::BANG_LIGHTS + 1));
		addChild(createLight<SmallLight<JeremyAquaLight> >(Vec(241.1 - 3, 355.3 - 3), module, StochSeq::BANG_LIGHTS + 2));
		addChild(createLight<SmallLight<JeremyRedLight> >(Vec(253.7 - 3, 355.3 - 3), module, StochSeq::BANG_LIGHTS + 3));

		float spacing = 27.0;
		// addOutput(createOutputCentered<TinyPJ301M>(Vec(44.2, 224.7), module, StochSeq::GATES_OUTPUT+1));

		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			// int paramId = NUM_OF_SLIDERS - 1 - i;
			if (i < 16)
				addOutput(createOutputCentered<TinyPJ301M>(Vec(44.2 + (spacing * i), 295.2), module, StochSeq::GATES_OUTPUT + i));
			else {
				int j = i;
				j %= 16;
				addOutput(createOutputCentered<TinyPJ301M>(Vec(44.2 + (spacing * j), 319.2), module, StochSeq::GATES_OUTPUT + i));
			}
		}


	}

	void appendContextMenu(Menu *menu) override {
		StochSeq *module = dynamic_cast<StochSeq*>(this->module);
		menu->addChild(new MenuEntry);

		menu->addChild(createIndexPtrSubmenuItem("Gate mode", {"Gates", "Triggers"}, &module->gateMode));
		menu->addChild(createIndexPtrSubmenuItem("V/OCT mode", {"Independent", "Sample and Hold"}, &module->voltMode));
		menu->addChild(createIndexPtrSubmenuItem("Volt Offset", {"±5V", "+10V"}, &module->voltRange));

		menu->addChild(new MenuEntry);

		menu->addChild(createBoolPtrMenuItem("Slider Percentages", "", &module->showPercentages));
		menu->addChild(createBoolPtrMenuItem("Keyboard Shortcuts", "", &module->enableKBShortcuts));
	}

	void onSelectKey(const event::SelectKey &e) override {
		StochSeq *module = dynamic_cast<StochSeq *>(this->module);
		if (!module->enableKBShortcuts) {
			ModuleWidget::onSelectKey(e);
			return;
		}

		if (e.key == GLFW_KEY_LEFT && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			e.consume(this);
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
				module->shiftPatternLeft();
			}
		} else if (e.key == GLFW_KEY_RIGHT && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			e.consume(this);
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
				module->shiftPatternRight();
			}
		} else if (e.key == GLFW_KEY_UP && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			e.consume(this);
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
				module->shiftPatternUp();
			}
		} else if (e.key == GLFW_KEY_DOWN && (e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			e.consume(this);
			if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
				module->shiftPatternDown();
			}
		} else {
			// ModuleWidget::onSelectKey(e);
			OpaqueWidget::onSelectKey(e);
		}
	}

};


Model* modelStochSeq = createModel<StochSeq, StochSeqWidget>("StochSeq");