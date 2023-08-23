#include "plugin.hpp"

#define NUM_OF_INPUTS 4

struct RandGates : Module {
    	enum ParamIds {
        WEIGHTING_PARAM,
        PERCENTAGE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        TRIGGER_INPUT,
        GATES_INPUT = TRIGGER_INPUT + NUM_OF_INPUTS,
		NUM_INPUTS = GATES_INPUT + NUM_OF_INPUTS
	};
	enum OutputIds {
        GATE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
        PURPLE_LIGHT,
        BLUE_LIGHT,
        AQUA_LIGHT,
        RED_LIGHT,
		NUM_LIGHTS
	};

    dsp::SchmittTrigger polyTrig[16];
    dsp::SchmittTrigger monoTrig;
    int currentGate[16];
    float weightProb = 0.5;

    RandGates() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(WEIGHTING_PARAM, 0.0, 4.0, 4.0, "Weight", {"Purple", "Blue", "Aqua", "Red", "Uniform"});
        configParam(PERCENTAGE_PARAM, 0.0, 1.0, 0.5, "Weight probability", "%", 0, 100);

        configInput(TRIGGER_INPUT, "Trigger");
        configInput(GATES_INPUT, "Purple");
        configInput(GATES_INPUT + 1, "Blue");
        configInput(GATES_INPUT + 2, "Aqua");
        configInput(GATES_INPUT + 3, "Red");

        configOutput(GATE_OUTPUT, "Main");

        configLight(PURPLE_LIGHT, "Output indicator");

        for (int i = 0; i < 16; i++) {
            setCurrentGate(i);
        }
    }

    void setCurrentGate(int channel) {
        int weight = (int)params[WEIGHTING_PARAM].getValue();
        weightProb = params[PERCENTAGE_PARAM].getValue();
        if (weight < 4) {
            if (random::uniform() < weightProb) {
                currentGate[channel] = weight;
            } else {
                int r = static_cast<int>(random::uniform() * 4);
                while (r == weight) {
                    r = static_cast<int>(random::uniform() * 4);
                }
                currentGate[channel] = r;
            }
        } else {
            currentGate[channel] = static_cast<int>(random::uniform() * NUM_OF_INPUTS);
        }

        /******* OLD *******/
        // if (params[WEIGHTING_PARAM].getValue() < 4) {
        //     int weight = (int)params[WEIGHTING_PARAM].getValue();
        //     int r = static_cast<int>(random::uniform() * (NUM_OF_INPUTS+1));
        //     currentGate = (r > 3) ? weight : r; // 40% chance of weighted choice
        // } else {
        //     currentGate = static_cast<int>(random::uniform() * NUM_OF_INPUTS);
        // }
    }

    void process(const ProcessArgs &args) override {

        // get number of channels first
        int channels = 1;

        if (inputs[TRIGGER_INPUT].isPolyphonic()) {
            channels = std::max(inputs[TRIGGER_INPUT].getChannels(), channels);
            for (int c = 0; c < channels; c++) {
                if (polyTrig[c].process(inputs[TRIGGER_INPUT].getVoltage(c))) {
                    setCurrentGate(c);
                }
                float in = inputs[GATES_INPUT + currentGate[c]].getVoltage();
                outputs[GATE_OUTPUT].setVoltage(in, c);
            }
        } else {
            if (monoTrig.process(inputs[TRIGGER_INPUT].getVoltage())) {
                setCurrentGate(0);
            }
            for (int i = 0; i < NUM_OF_INPUTS; i++) {
                channels = std::max(inputs[GATES_INPUT+i].getChannels(), channels);
            }

            for (int c = 0; c < channels; c++) {
                float in = inputs[GATES_INPUT + currentGate[0]].getVoltage(c);
                outputs[GATE_OUTPUT].setVoltage(in, c);
            }
        }


        for (int i = 0; i < NUM_LIGHTS; i++) {
            lights[i].setBrightness((i==currentGate[0]) ? 1.0 : 0.0);
        }
        outputs[GATE_OUTPUT].setChannels(channels);
        // outputs[GATE_OUTPUT].setVoltage(inputs[GATES_INPUT + currentGate].getVoltage());
    }
};

struct RandGatesWidget : ModuleWidget {
    RandGatesWidget(RandGates *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/RandGates.svg"), asset::plugin(pluginInstance, "res/RandGates-dark.svg")));

        addChild(createWidget<JeremyScrew>(Vec(16.5, 2)));
        addChild(createWidget<JeremyScrew>(Vec(16.5, box.size.y - 14)));

        addInput(createInputCentered<PJ301MPort>(Vec(22.5, 79.4), module, RandGates::TRIGGER_INPUT));
        addParam(createParamCentered<BlueInvertKnob>(Vec(22.5, 281.6), module, RandGates::WEIGHTING_PARAM));
        addParam(createParamCentered<NanoBlueKnob>(Vec(34, 265.2), module, RandGates::PERCENTAGE_PARAM));

        for (int i = 0; i < NUM_OF_INPUTS; i++) {
            addInput(createInputCentered<PJ301MPort>(Vec(22.5, 119.8 + (i * 40.5)), module, RandGates::GATES_INPUT+i));
        }
        // addInput(createInputCentered<PJ301MPort>(Vec(36.9, 237.3), module, StochSeq::CLOCK_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(22.5, 322.1), module, RandGates::GATE_OUTPUT));

        // lights
        addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(22.5 - 3.21, 340.9 - 3.21), module, RandGates::PURPLE_LIGHT));
        addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(22.5 - 3.21, 340.9 - 3.21), module, RandGates::BLUE_LIGHT));
        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(22.5 - 3.21, 340.9 - 3.21), module, RandGates::AQUA_LIGHT));
        addChild(createLight<SmallLight<JeremyRedLight>>(Vec(22.5 - 3.21, 340.9 - 3.21), module, RandGates::RED_LIGHT));
    }
};

Model *modelRandGates = createModel<RandGates, RandGatesWidget>("RandGates");
