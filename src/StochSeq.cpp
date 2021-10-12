#include "plugin.hpp"

#define SLIDER_WIDTH 15
#define SLIDER_TOP 4
#define NUM_OF_SLIDERS 32
#define NUM_OF_LIGHTS 32

struct StochSeq : Module, Quantize {
	enum ModeIds {
		GATE_MODE,
		TRIG_MODE,
		VOLT_INDEPENDENT_MODE,
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
		LIGHTS = NUM_OF_LIGHTS,
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
	bool enableKBShortcuts = true;
	bool isCtrlClick = false;

	StochSeq() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configButton(RESET_PARAM, "Reset");
		configParam(PATTERN_PARAM, 0.0, 7.0, 0.0, "Pattern");
		configButton(INVERT_PARAM, "Invert pattern");
		configButton(RANDOM_PARAM, "Randomize pattern");
		configButton(DIMINUTION_PARAM, "Diminish pattern");
		configParam(LENGTH_PARAM, 1.0, 32.0, 32.0, "Seq length");
		configParam(SPREAD_PARAM, -4.0, 4.0, 1.0, "Spread");
		configParam(ROOT_NOTE_PARAM, 0.0, Quantize::NUM_OF_NOTES - 1, 0.0, "Root note");
		configParam(SCALE_PARAM, 0.0, Quantize::NUM_OF_SCALES, 0.0, "Scale");

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
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

		randLight = static_cast<int>(random::uniform() * NUM_OF_LIGHTS);
	}

	~StochSeq() {
		delete[] gateProbabilities;
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "currentPattern", json_integer(currentPattern));

		json_t *probsJ = json_array();
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			json_t *probJ = json_real(gateProbabilities[i]);
			json_array_append_new(probsJ, probJ);
		}
		json_object_set_new(rootJ, "probs", probsJ);
		json_object_set_new(rootJ, "percentages", json_boolean(showPercentages));
		json_object_set_new(rootJ, "kbshortcuts", json_boolean(enableKBShortcuts));
		json_object_set_new(rootJ, "gateMode", json_integer(gateMode));
		json_object_set_new(rootJ, "voltMode", json_integer(voltMode));

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

		json_t *currentPatternJ = json_object_get(rootJ, "currentPattern");
		if (currentPatternJ) currentPattern = json_integer_value(currentPatternJ);

		json_t *probsJ = json_object_get(rootJ, "probs");
		if (probsJ) {
			for (int i = 0; i < NUM_OF_SLIDERS; i++) {
				json_t *probJ = json_array_get(probsJ, i);
				if (probJ)
					gateProbabilities[i] = json_real_value(probJ);
			}
		}
	}

	void process(const ProcessArgs& args) override {
		if (resetTrig.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
			resetMode = true;
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
		}
		if (dimTrig.process(params[DIMINUTION_PARAM].getValue() + inputs[DIMINUTION_INPUT].getVoltage())) {
			diminish();
		}
		if (clockTrig.process(inputs[CLOCK_INPUT].getVoltage())) {
			if (resetMode) {
				resetMode = false;
				resetSeqToEnd();
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
		outputs[NOT_GATE_MAIN_OUTPUT].setVoltage(notGateVolt ? 10.0 : 0.0); // todo
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

		float spread = params[SPREAD_PARAM].getValue();
		float volts = prob * (2 * spread) - spread;
		float invVolts = (1.0 - prob) * (2 * spread) - spread;
		pitchVoltage = Quantize::quantizeRawVoltage(volts, rootNote, scale);
		invPitchVoltage = Quantize::quantizeRawVoltage(invVolts, rootNote, scale);

		// pitchVoltage = prob * (2 * spread) - spread;
	}

	void resetSeqToEnd() {
		gateIndex = seqLength-1;
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
	}

	void shiftPatternUp() {
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] += 0.05;
			gateProbabilities[i] = clamp(gateProbabilities[i], 0.0, 1.0);
		}
	}

	void shiftPatternDown() {
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] -= 0.05;
			gateProbabilities[i] = clamp(gateProbabilities[i], 0.0, 1.0);
		}
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
			default:
				for (int i = 0; i < NUM_OF_SLIDERS; i++) {
					gateProbabilities[i] = random::uniform();
				}
		}
	}
};

namespace StochSeqNS {
	struct GateModeValueItem : MenuItem {
		StochSeq *module;
		int gateMode;
		void onAction(const event::Action &e) override {
			module->gateMode = gateMode;
		}
	};

	struct GateModeItem : MenuItem {
		StochSeq *module;
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			std::vector<std::string> modes = {"Gates", "Triggers"};
			for (int i = 0; i < 2; i++) {
				GateModeValueItem *item = new GateModeValueItem;
				item->text = modes[i];
				item->rightText = CHECKMARK(module->gateMode == i);
				item->module = module;
				item->gateMode = i;
				menu->addChild(item);
			}
			return menu;
		}
	};

	struct VoltModeValueItem : MenuItem {
		StochSeq *module;
		int voltMode;
		void onAction(const event::Action &e) override {
			module->voltMode = voltMode;
		}
	};

	struct VoltModeItem : MenuItem {
		StochSeq *module;
		Menu *createChildMenu() override {
			Menu *menu = new Menu;
			std::vector<std::string> modes = {"Independent", "Sample and Hold"};
			for (int i = 0; i < 2; i++) {
				VoltModeValueItem *item = new VoltModeValueItem;
				item->text = modes[i];
				item->rightText = CHECKMARK(module->voltMode == i + StochSeq::VOLT_INDEPENDENT_MODE);
				item->module = module;
				item->voltMode = i + StochSeq::VOLT_INDEPENDENT_MODE;
				menu->addChild(item);
			}
			return menu;
		}
	};

	struct ShowTextValueItem : MenuItem {
        StochSeq *module;
        bool showPercentages;
        void onAction(const event::Action &e) override {
            module->showPercentages = showPercentages;
        }
    };

    struct ShowTextItem : MenuItem {
        StochSeq *module;
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
		StochSeq *module;
		bool enableKBShortcuts;
		void onAction(const event::Action &e) override {
			module->enableKBShortcuts = enableKBShortcuts;
		}
	};

	struct EnableShortcutsItem : MenuItem {
		StochSeq *module;
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
	}

	void toggleProbabilities(float currentX) {
        if (currentX < 0) currentX = 0;
        int index = (int)(currentX / sliderWidth);
        if (index >= NUM_OF_SLIDERS) index = NUM_OF_SLIDERS - 1;
        float p = module->gateProbabilities[index];
        module->gateProbabilities[index] = p < 0.5 ? 1.0 : 0.0;
    }

	float getSliderHeight(int index) {
		float y = box.size.y - SLIDER_TOP;
		return y - (y * module->gateProbabilities[index]);
	}

	void draw(const DrawArgs& args) override {
		//background
		nvgFillColor(args.vg, nvgRGB(40, 40, 40));
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgFill(args.vg);


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
		int visibleSliders = (int)module->params[StochSeq::LENGTH_PARAM].getValue();
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

			// if (i < module->params[StochSeq::LENGTH_PARAM].getValue()) {
			// 	nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 191)); // bottoms
			// 	// nvgFillColor(args.vg, nvgRGBA(0, 238, 255, 191)); // bottoms
			// 	nvgBeginPath(args.vg);
			// 	nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, box.size.y - sHeight);
			// 	nvgFill(args.vg);

			// 	nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // tops
			// 	// nvgFillColor(args.vg, nvgRGB(0, 238, 255)); // tops
			// 	nvgBeginPath(args.vg);
			// 	nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, SLIDER_TOP);
			// 	nvgFill(args.vg);
			// } else {
			// 	nvgFillColor(args.vg, nvgRGBA(150, 150, 150, 191)); // bottoms
			// 	nvgBeginPath(args.vg);
			// 	nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, box.size.y - sHeight);
			// 	nvgFill(args.vg);

			// 	nvgFillColor(args.vg, nvgRGB(150, 150, 150)); // tops
			// 	nvgBeginPath(args.vg);
			// 	nvgRect(args.vg, i * sliderWidth, sHeight, sliderWidth, SLIDER_TOP);
			// 	nvgFill(args.vg);
			// }

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

		nvgGlobalTint(args.vg, color::WHITE);

		// seq position
		if (module->gateIndex >= 0) {
			nvgStrokeWidth(args.vg, 2.0);
			// nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
			nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
			nvgBeginPath(args.vg);
			nvgRect(args.vg, module->gateIndex * sliderWidth, 1, sliderWidth, box.size.y - 1);
			nvgStroke(args.vg);
		}

		/***** OLD STUFF *****/
		// faded out non-pattern
		// if (module->params[StochSeq::LENGTH_PARAM].getValue() < NUM_OF_SLIDERS) {
		// 	float x = module->params[StochSeq::LENGTH_PARAM].getValue() * SLIDER_WIDTH;
		// 	nvgStrokeWidth(args.vg, 2.0);
		// 	nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
		// 	nvgBeginPath(args.vg);
		// 	nvgMoveTo(args.vg, x, 0);
		// 	nvgLineTo(args.vg, x, box.size.y);
		// 	nvgStroke(args.vg);

		// 	nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 130));
		// 	nvgBeginPath(args.vg);
		// 	nvgRect(args.vg, x, 0, box.size.x - x, box.size.y);
		// 	nvgFill(args.vg);
		// }
	}
};

struct StochSeqWidget : ModuleWidget {
	StochSeqWidget(StochSeq* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/StochSeq.svg")));

		StochSeqDisplay *display = new StochSeqDisplay();
		display->module = module;
		display->box.pos = Vec(7.4, 86.7);
		display->box.size = Vec(480, 102.9);
		addChild(display);

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
		addInput(createInputCentered<PJ301MPort>(Vec(71.4, 228.7), module, StochSeq::RESET_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(360.7, 228.7), module, StochSeq::GATE_MAIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(397.1, 228.7), module, StochSeq::NOT_GATE_MAIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(433.5, 228.7), module, StochSeq::INV_VOLT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(469.9, 228.7), module, StochSeq::VOLT_OUTPUT));

		// addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(84, 50), module, StochSeq::BLUE_LIGHT));

		for (int i = 0; i < NUM_OF_LIGHTS; i++) {
			float x = 196 * i / NUM_OF_LIGHTS + 5;
			
			float y = ((-std::sin(2.0 * M_PI * i / NUM_OF_LIGHTS) * 0.5 + 0.5) * 50 + 15);
			// float x = random::uniform() * 260 + 1;
			// float y = random::uniform() * 50 + 15;
			// int light = int(random::uniform() * 4);

			int light = int(i / (NUM_OF_LIGHTS/4.0));

			switch(light) {
				case 0:
					addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));
					break;
				case 1:
					addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));
					break;
				case 2:
					addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));
					break;
				case 3:
					addChild(createLight<SmallLight<JeremyRedLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));
					break;
			}

			// if (random::uniform() < 0.5)
				// addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(x, y), module, StochSeq::LIGHTS + i));

			// addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(84, 50), module, StochSeq::BLUE_LIGHT));

		}

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
		
		StochSeqNS::GateModeItem *gateModeItem = new StochSeqNS::GateModeItem;
		gateModeItem->text = "Gate mode";
		if (module->gateMode == StochSeq::GATE_MODE) gateModeItem->rightText = std::string("Gates") + " " + RIGHT_ARROW;
		else gateModeItem->rightText = std::string("Triggers") + " " + RIGHT_ARROW;
		gateModeItem->module = module;
		menu->addChild(gateModeItem);
		
		StochSeqNS::VoltModeItem *voltModeItem = new StochSeqNS::VoltModeItem;
		voltModeItem->text = "V/OCT mode";
		if (module->voltMode == StochSeq::VOLT_INDEPENDENT_MODE) voltModeItem->rightText = std::string("Independent") + " " + RIGHT_ARROW;
		else voltModeItem->rightText = std::string("Sample and Hold") + " " + RIGHT_ARROW;
		voltModeItem->module = module;
		menu->addChild(voltModeItem);

		menu->addChild(new MenuEntry);

		StochSeqNS::ShowTextItem *showTextItem = new StochSeqNS::ShowTextItem;
		showTextItem->text = "Slider Percentages";
		if (module->showPercentages) showTextItem->rightText = std::string("show") + " " + RIGHT_ARROW;
		else showTextItem->rightText = std::string("hide") + " " + RIGHT_ARROW;
		showTextItem->module = module;
		menu->addChild(showTextItem);

		StochSeqNS::EnableShortcutsItem *enableShortcutItem = new StochSeqNS::EnableShortcutsItem;
		enableShortcutItem->text = "Keyboard Shortcuts";
		if (module->enableKBShortcuts) enableShortcutItem->rightText = std::string("on") + " " + RIGHT_ARROW;
		else enableShortcutItem->rightText = std::string("off") + " " + RIGHT_ARROW;
		enableShortcutItem->module = module;
		menu->addChild(enableShortcutItem);
	}

	void onHoverKey(const event::HoverKey &e) override {
		StochSeq *module = dynamic_cast<StochSeq *>(this->module);
		if (!module->enableKBShortcuts) {
			ModuleWidget::onHoverKey(e);
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
			ModuleWidget::onHoverKey(e);
			// OpaqueWidget::onHoverKey(e);
		}
	}
};


Model* modelStochSeq = createModel<StochSeq, StochSeqWidget>("StochSeq");