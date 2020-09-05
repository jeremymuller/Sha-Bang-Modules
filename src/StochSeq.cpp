#include "plugin.hpp"

#define SLIDER_WIDTH 15
#define SLIDER_TOP 4
#define NUM_OF_SLIDERS 32
#define NUM_OF_LIGHTS 32

struct StochSeq : Module {
	enum ParamIds {
		PATTERN_PARAM,
		RANDOM_PARAM,
		INVERT_PARAM,
		DIMINUTION_PARAM,
		LENGTH_PARAM,
		SPREAD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT = NUM_OF_SLIDERS,
		GATE_MAIN_OUTPUT = GATES_OUTPUT + NUM_OF_SLIDERS,
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
	int seqLength = NUM_OF_SLIDERS;
	int gateIndex = -1;
	int currentGateOut = gateIndex;
	int currentPattern = 0;
	bool resetMode = false;
	bool lightBlink = false;
	int randLight;
	float pitchVoltage = 0.0;
	float *gateProbabilities = new float[NUM_OF_SLIDERS];

	StochSeq() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PATTERN_PARAM, 0.0, 7.0, 0.0, "pattern");
		configParam(INVERT_PARAM, 0.0, 1.0, 0.0, "invert pattern");
		configParam(RANDOM_PARAM, 0.0, 1.0, 0.0, "randomize pattern");
		configParam(DIMINUTION_PARAM, 0.0, 1.0, 0.0, "diminish pattern");
		configParam(LENGTH_PARAM, 1.0, 32.0, 32.0, "seq length");
		configParam(SPREAD_PARAM, -4.0, 4.0, 0.0, "spread");

		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			gateProbabilities[i] = random::uniform();
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

		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *currentPatternJ = json_object_get(rootJ, "currentPattern");
		if (currentPatternJ) {
			currentPattern = json_integer_value(currentPatternJ);
		}

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
		if (resetTrig.process(inputs[RESET_INPUT].getVoltage())) {
			resetMode = true;
		}
		if ((int)params[PATTERN_PARAM].getValue() != currentPattern) {
			int patt = (int)params[PATTERN_PARAM].getValue();
			currentPattern = patt;
			genPatterns(patt);
		}
		if (randomTrig.process(params[RANDOM_PARAM].getValue())) {
			genPatterns(100);
		}
		if (invertTrig.process(params[INVERT_PARAM].getValue())) {
			invert();
		}
		if (dimTrig.process(params[DIMINUTION_PARAM].getValue())) {
			diminish();
		}
		if (clockTrig.process(inputs[CLOCK_INPUT].getVoltage())) {
			if (resetMode) {
				resetMode = false;
				resetSeqToEnd();
			}
			clockStep();
		}

		bool pulse = gatePulse.process(1 / args.sampleRate);
		float gateVolt = pulse ? 10.0 : 0.0;
		// float blink = lightBlink ? 1.0 : 0.0;
		outputs[GATES_OUTPUT + currentGateOut].setVoltage(gateVolt);
		outputs[GATE_MAIN_OUTPUT].setVoltage(gateVolt);
		outputs[VOLT_OUTPUT].setVoltage(pitchVoltage);
		// int randLight = int(random::uniform() * NUM_OF_LIGHTS);
		for (int i = 0; i < NUM_OF_LIGHTS; i++) {
			if (currentGateOut == i)
				lights[LIGHTS + i].setBrightness(lightBlink ? 1.0 : 0.0);
			else
				lights[LIGHTS + i].setBrightness(0.0);
		}
	}

	 void clockStep() {
		seqLength = (int)params[LENGTH_PARAM].getValue();
		gateIndex = (gateIndex + 1) % seqLength;
		lightBlink = false;

		float prob = gateProbabilities[gateIndex];
		if (random::uniform() < prob) {
			gatePulse.trigger(1e-3f);
			currentGateOut = gateIndex;
			lightBlink = true;
			randLight = static_cast<int>(random::uniform() * NUM_OF_LIGHTS);
		}

		float spread = params[SPREAD_PARAM].getValue();
		pitchVoltage = prob * (2 * spread) - spread;
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

struct StochSeqDisplay : Widget {
	StochSeq *module;
	float initX = 0;
	float initY = 0;
	float dragX = 0;
	float dragY = 0;
	StochSeqDisplay() {}

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
		module->gateProbabilities[index] = 1.0 - dragY / (box.size.y - SLIDER_TOP);
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

		if (module == NULL) return;

		// sliders
		nvgStrokeColor(args.vg, nvgRGB(60, 70, 73));
		for (int i = 0; i < NUM_OF_SLIDERS; i++) {
			nvgStrokeWidth(args.vg, (i % 4 == 0 ? 2 : 0.5));
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, i * SLIDER_WIDTH, 0);
			nvgLineTo(args.vg, i * SLIDER_WIDTH, box.size.y);
			nvgStroke(args.vg);

			// float sHeight = sliderHeights[i];
			float sHeight = getSliderHeight(i);

			if (i < module->params[StochSeq::LENGTH_PARAM].getValue()) {
				nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 191)); // bottoms
				// nvgFillColor(args.vg, nvgRGBA(0, 238, 255, 191)); // bottoms
				nvgBeginPath(args.vg);
				nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, box.size.y - sHeight);
				nvgFill(args.vg);

				nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // tops
				// nvgFillColor(args.vg, nvgRGB(0, 238, 255)); // tops
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
		if (module->gateIndex >= 0) {
			nvgStrokeWidth(args.vg, 2.0);
			// nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
			nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
			nvgBeginPath(args.vg);
			nvgRect(args.vg, module->gateIndex * SLIDER_WIDTH, 1, SLIDER_WIDTH, box.size.y - 1);
			nvgStroke(args.vg);
		}

		// faded out non-pattern
		if (module->params[StochSeq::LENGTH_PARAM].getValue() < NUM_OF_SLIDERS) {
			float x = module->params[StochSeq::LENGTH_PARAM].getValue() * SLIDER_WIDTH;
			nvgStrokeWidth(args.vg, 2.0);
			nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
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

		addParam(createParamCentered<BlueInvertKnob>(Vec(105.9, 237.3), module, StochSeq::LENGTH_PARAM));
		addParam(createParamCentered<BlueInvertKnob>(Vec(140.5, 237.3), module, StochSeq::PATTERN_PARAM));
		addParam(createParamCentered<DefaultButton>(Vec(175, 237.3), module, StochSeq::RANDOM_PARAM));
		addParam(createParamCentered<DefaultButton>(Vec(209.5, 237.3), module, StochSeq::INVERT_PARAM));
		addParam(createParamCentered<DefaultButton>(Vec(244.1, 237.3), module, StochSeq::DIMINUTION_PARAM));
		addParam(createParamCentered<BlueKnob>(Vec(437.2, 237.3), module, StochSeq::SPREAD_PARAM));


		addInput(createInputCentered<PJ301MPort>(Vec(36.9, 237.3), module, StochSeq::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(71.4, 237.3), module, StochSeq::RESET_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(345.5, 237.3), module, StochSeq::GATE_MAIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(399.5, 237.3), module, StochSeq::VOLT_OUTPUT));

		// addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(84, 50), module, StochSeq::BLUE_LIGHT));

		for (int i = 0; i < NUM_OF_LIGHTS; i++) {
			// TODO: a sine wave instead?
			float x = 196 * i / NUM_OF_LIGHTS + 5;
			
			float y = ((-std::sin(2.0 * M_PI * i / NUM_OF_LIGHTS) * 0.5 + 0.5) * 50 + 15);
			// float x = random::uniform() * 260 + 1;
			// float y = random::uniform() * 50 + 15;
			int light = int(random::uniform() * 4);
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
};


Model* modelStochSeq = createModel<StochSeq, StochSeqWidget>("StochSeq");