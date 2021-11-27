#include "plugin.hpp"

#define NUM_OF_CHANNELS 32

struct StochSeq4X : Module {
    enum SequencerIds {
        PURPLE_SEQ,
        BLUE_SEQ,
        AQUA_SEQ,
        RED_SEQ,
        NUM_SEQS
    };
    enum ParamIds {
        TOGGLE_ALL_PARAMS,
        TOGGLE_NOTGATE_PARAMS,
        NUM_PARAMS = TOGGLE_NOTGATE_PARAMS + NUM_SEQS
    };
    enum InputIds {
        FREQ_INPUT,
        TRIGGER_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        GATES_OUTPUT,
        NUM_OUTPUTS = GATES_OUTPUT + NUM_OF_CHANNELS
    };
    enum LightIds {
        TOGGLE_LIGHT,
        NUM_LIGHTS
    };

    SequencerIds currentSeq = PURPLE_SEQ;
    bool isGate[NUM_SEQS] = {true, true, true, true};
    bool allStrips = false;
    float outputValues[NUM_SEQS * NUM_OF_CHANNELS];
    // Expander
    float leftMessages[2][NUM_SEQS * NUM_OF_CHANNELS] = {}; // messages from StochSeq4: 2 Gate values

    StochSeq4X() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configSwitch(TOGGLE_ALL_PARAMS, 0.0, 4.0, 0.0, "Outputs", {"Purple", "Blue", "Aqua", "Red", "All"});

        configSwitch(TOGGLE_NOTGATE_PARAMS + PURPLE_SEQ, 0.0, 1.0, 0.0, "Column 1", {"gates", "not gates"});
        configSwitch(TOGGLE_NOTGATE_PARAMS + BLUE_SEQ, 0.0, 1.0, 0.0, "Column 2", {"gates", "not gates"});
        configSwitch(TOGGLE_NOTGATE_PARAMS + AQUA_SEQ, 0.0, 1.0, 0.0, "Column 3", {"gates", "not gates"});
        configSwitch(TOGGLE_NOTGATE_PARAMS + RED_SEQ, 0.0, 1.0, 0.0, "Column 4", {"gates", "not gates"});

        for (int i = 0; i < NUM_OF_CHANNELS; i++) {
			configOutput(GATES_OUTPUT + i, "Gate " + std::to_string(i+1));
		}

        leftExpander.producerMessage = leftMessages[0];
        leftExpander.consumerMessage = leftMessages[1];

        // for (int i = 0; i < 1 + NUM_OF_CHANNELS; i++) {
        //     outputValues[i] = 0.0;
        // }
    }

    void process(const ProcessArgs &args) override {
        // this expander code is modified from https://github.com/MarcBoule/ImpromptuModular/blob/v2/src/FourView.cpp

        currentSeq = (SequencerIds)params[TOGGLE_ALL_PARAMS].getValue();
        allStrips = currentSeq > RED_SEQ;

        for (int i = 0; i < NUM_SEQS; i++) {
            isGate[i] = (params[TOGGLE_NOTGATE_PARAMS + i].getValue() == 0);
        }

        bool isParent = (leftExpander.module && leftExpander.module->model == modelStochSeq4);

        if (isParent) {
            float *outputFromParent = (float *)(leftExpander.consumerMessage);
            memcpy(outputValues, outputFromParent, 4 * NUM_SEQS * NUM_OF_CHANNELS);
        } else {
            for (int i = 0; i < NUM_OF_CHANNELS * 4; i++) {
                outputValues[i] = 0.0;
            }
        }

        if (allStrips) {
            for (int i = 0, index = 0; i < NUM_SEQS; i++) {
                for (int j = 0; j < 8; j++, index++) {
                    int outputIndex = j + (i * NUM_OF_CHANNELS);
                    if (isGate[i])
                        outputs[GATES_OUTPUT + index].setVoltage(outputValues[outputIndex]);
                    else if (isParent)
                        outputs[GATES_OUTPUT + index].setVoltage(outputValues[outputIndex] > 0.0 ? 0.0 : 10.0);
                    else
                        outputs[GATES_OUTPUT + index].setVoltage(0.0);
                }
            }
        } else {
            int start = currentSeq * NUM_OF_CHANNELS;
            int end = start + NUM_OF_CHANNELS;
            for (int i = start, j = 0; i < end; i++, j++) {
                int isGateIndex = floor(j / 8);
                if (isGate[isGateIndex])
                    outputs[GATES_OUTPUT + j].setVoltage(outputValues[i]);
                else if (isParent)
                    outputs[GATES_OUTPUT + j].setVoltage(outputValues[i] > 0.0 ? 0.0 : 10.0);
                else
                    outputs[GATES_OUTPUT + j].setVoltage(0.0);
            }
        }

    }
};

struct StochSeq4XDisplay : Widget {
	StochSeq4X *module;
	StochSeq4XDisplay() {}

	void draw(const DrawArgs& args) override {

        if (module == NULL) return;
		//background
		// nvgFillColor(args.vg, nvgRGB(40, 40, 40));
		// nvgBeginPath(args.vg);
		// nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		// nvgFill(args.vg);

        switch (module->currentSeq) {
            case StochSeq4X::PURPLE_SEQ:
                nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
                break;
            case StochSeq4X::BLUE_SEQ:
                nvgStrokeColor(args.vg, nvgRGB(38, 0, 255));
                break;
            case StochSeq4X::AQUA_SEQ:
                nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
                break;
            case StochSeq4X::RED_SEQ:
                nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
                break;
            default:
                nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
                break;
        }

        float ySpacing = 23.3;
        float xSpacing = 27.0;
        for (int x = 0; x < 4; x++) {
            if (module->allStrips) {
                switch (x) {
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
            }
            for (int y = 0; y < 8; y++) {
                nvgStrokeWidth(args.vg, 3.0);
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, 19.5 + (xSpacing * x), 146.7 + (ySpacing * y), 16 / 2.0);
                nvgStroke(args.vg);
            }
		}

	}

	// void drawLayer(const DrawArgs& args, int layer) override {

	// 	if (module == NULL) return;

	// 	if (layer == 1) {

	// 		// seq position
	// 		if (module->gateIndex >= 0) {
	// 			nvgStrokeWidth(args.vg, 2.0);
	// 			// nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
	// 			nvgStrokeColor(args.vg, nvgRGB(0, 238, 255));
	// 			nvgBeginPath(args.vg);
	// 			float x = clamp(module->gateIndex * sliderWidth, 0.0, box.size.x - sliderWidth);
	// 			nvgRect(args.vg, x, 1, sliderWidth, box.size.y - 1);
	// 			nvgStroke(args.vg);
	// 		}

	// 	}
	// 	Widget::drawLayer(args, layer);

	// }
};

struct StochSeq4XWidget : ModuleWidget {
    StochSeq4XWidget(StochSeq4X *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/StochSeq4X.svg")));

        StochSeq4XDisplay *display = new StochSeq4XDisplay();
        display->module = module;
        display->box.pos = Vec(0.0, 0.0);
        display->box.size = Vec(120, 379.6);
        addChild(display);

        addChild(createWidget<JeremyScrew>(Vec(12, 2)));
        addChild(createWidget<JeremyScrew>(Vec(12, box.size.y - 14)));
        addChild(createWidget<JeremyScrew>(Vec(box.size.x-12-12, 2)));
        addChild(createWidget<JeremyScrew>(Vec(box.size.x-12-12, box.size.y - 14)));

        // addParam(createParamCentered<Jeremy_4Switch>(Vec(45, 76.9), module, StochSeq4X::TOGGLE_PARAM));
        addParam(createParamCentered<BlueInvertKnob>(Vec(60, 83.1), module, StochSeq4X::TOGGLE_ALL_PARAMS));

        addParam(createParamCentered<NanoBlueButton>(Vec(19.5, 331), module, StochSeq4X::TOGGLE_NOTGATE_PARAMS + StochSeq4X::PURPLE_SEQ));
        addParam(createParamCentered<NanoBlueButton>(Vec(46.5, 331), module, StochSeq4X::TOGGLE_NOTGATE_PARAMS + StochSeq4X::BLUE_SEQ));
        addParam(createParamCentered<NanoBlueButton>(Vec(73.5, 331), module, StochSeq4X::TOGGLE_NOTGATE_PARAMS + StochSeq4X::AQUA_SEQ));
        addParam(createParamCentered<NanoBlueButton>(Vec(100.5, 331), module, StochSeq4X::TOGGLE_NOTGATE_PARAMS + StochSeq4X::RED_SEQ));

        // outputs
        // addOutput(createOutputCentered<TinyPJ301M>(Vec(19.9, 76.7), module, StochSeq4X::GATES_OUTPUT));
        // addOutput(createOutputCentered<TinyPJ301M>(Vec(70.1, 76.7), module, StochSeq4X::GATES_OUTPUT + 1));

        float ySpacing = 23.3;
        float xSpacing = 27.0;
        for (int x = 0, i = 0; x < 4; x++) {
            for (int y = 0; y < 8; y++, i++) {
                addOutput(createOutputCentered<TinyPJ301M>(Vec(19.5 + (xSpacing * x), 146.7 + (ySpacing * y)), module, StochSeq4X::GATES_OUTPUT + i));
                // i++;
            }
		}
    }
};

Model *modelStochSeq4X = createModel<StochSeq4X, StochSeq4XWidget>("StochSeq4X");