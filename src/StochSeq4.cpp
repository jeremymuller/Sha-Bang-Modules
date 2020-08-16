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
    int seqLength = NUM_OF_SLIDERS;
    int gateIndex = -1;
    int currentPattern = 0;
    float pitchVoltage = 0.0;
    float gateProbabilities[NUM_OF_SLIDERS];
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
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATES_OUTPUT = NUM_SEQS,
		VOLTS_OUTPUT = GATES_OUTPUT + NUM_SEQS,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    dsp::SchmittTrigger clockTrig;
    dsp::SchmittTrigger resetTrig;
    bool resetMode = true;
    Sequencer seqs[NUM_SEQS];
    SequencerIds id;

    StochSeq4() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(LENGTH_PARAM, 1.0, 32.0, 32.0, "seq length");
        configParam(RANDOM_PARAM+PURPLE_SEQ, 0.0, 1.0, 0.0, "randomize purple pattern");

        for (int i = 0; i < NUM_SEQS; i++) {
            for (int j = 0; j < NUM_OF_SLIDERS; j++) {
                seqs[i].gateProbabilities[j] = random::uniform();
            }
        }
    }

    void process(const ProcessArgs& args) override {
        // TODO
        if (resetTrig.process(inputs[RESET_INPUT].getVoltage())) {
            resetMode = true;
        }
        if (seqs[id].randomTrig.process(params[RANDOM_PARAM+id].getValue())) {
            genPatterns(100);
        }
        if (clockTrig.process(inputs[CLOCK_INPUT].getVoltage())) {
            if (resetMode) {
                resetMode = false;
                resetSeqToEnd();
            }
            clockStep();
        }


    }

    void clockStep() {

    }

    void resetSeqToEnd() {
        // TODO
    }

    void genPatterns(int c) {
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

            nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 191)); // bottoms
            nvgBeginPath(args.vg);
            nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, box.size.y - sHeight);
            nvgFill(args.vg);

            nvgFillColor(args.vg, nvgRGB(255, 255, 255)); // tops
            nvgBeginPath(args.vg);
            nvgRect(args.vg, i * SLIDER_WIDTH, sHeight, SLIDER_WIDTH, SLIDER_TOP);
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

        addParam(createParamCentered<TinyPurpleButton>(Vec(93.4, 104.8), module, StochSeq4::RANDOM_PARAM + StochSeq4::PURPLE_SEQ));
        // addParam(createParamCentered<TinyPurpleButton>(Vec(93.4, 104.8), module, StochSeq4::RANDOM_PARAM + StochSeq4::BLUE_SEQ));
        // addParam(createParamCentered<TinyPurpleButton>(Vec(93.4, 104.8), module, StochSeq4::RANDOM_PARAM + StochSeq4::AQUA_SEQ));
        // addParam(createParamCentered<TinyPurpleButton>(Vec(93.4, 104.8), module, StochSeq4::RANDOM_PARAM + StochSeq4::RED_SEQ));

        addInput(createInputCentered<PJ301MPort>(Vec(126.9, 63.8), module, StochSeq4::CLOCK_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(161.5, 63.8), module, StochSeq4::RESET_INPUT));

        addParam(createParamCentered<PurpleInvertKnob>(Vec(24.3, 104.8), module, StochSeq4::LENGTH_PARAM));
    }
};

Model *modelStochSeq4 = createModel<StochSeq4, StochSeq4Widget>("StochSeq4");
