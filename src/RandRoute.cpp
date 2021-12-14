#include "plugin.hpp"

#define NUM_OF_OUTPUTS 4

struct RandRoute : Module {
    enum ParamIds {
        WEIGHTING_PARAM,
        PERCENTAGE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        TRIGGER_INPUT,
        GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
        GATES_OUTPUT = TRIGGER_INPUT + NUM_OF_OUTPUTS,
		NUM_OUTPUTS = GATES_OUTPUT + NUM_OF_OUTPUTS
	};
	enum LightIds {
        PURPLE_LIGHT,
        BLUE_LIGHT,
        AQUA_LIGHT,
        RED_LIGHT,
		NUM_LIGHTS
	};

    dsp::SchmittTrigger mainTrig;
    dsp::BooleanTrigger gateTriggers[16];
    int currentGate[16];
    float weightProb = 0.5;
    bool outcomes[NUM_OF_OUTPUTS][16] = {};
    bool toggle = false;

    RandRoute() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(WEIGHTING_PARAM, 0.0, 4.0, 4.0, "Weight", {"Purple", "Blue", "Aqua", "Red", "Uniform"});
        configParam(PERCENTAGE_PARAM, 0.0, 1.0, 0.5, "Weight probability", "%", 0, 100);

        configInput(TRIGGER_INPUT, "Trigger");
        configInput(GATE_INPUT, "Main");

        configOutput(GATES_OUTPUT, "Purple");
        configOutput(GATES_OUTPUT + 1, "Blue");
        configOutput(GATES_OUTPUT + 2, "Aqua");
        configOutput(GATES_OUTPUT + 3, "Red");

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
                int r = static_cast<int>(floor(random::uniform() * 4));
                while (r == weight) {
                    r = static_cast<int>(floor(random::uniform() * 4));
                }
                currentGate[channel] = r;
            }
        } else {
            currentGate[channel] = static_cast<int>(floor(random::uniform() * NUM_OF_OUTPUTS));
        }
    }

    void process(const ProcessArgs &args) override {
        int channels = std::max(inputs[GATE_INPUT].getChannels(), 1);
        if (inputs[TRIGGER_INPUT].isConnected()) {
            if (mainTrig.process(inputs[TRIGGER_INPUT].getVoltage())) {
                setCurrentGate(0);
            }
            for (int i = 0; i < NUM_LIGHTS; i++) {
                lights[i].setBrightness((i==currentGate[0]) ? 1.0 : 0.0);
            }

            for (int c = 0; c < channels; c++) {
                float in = inputs[GATE_INPUT].getVoltage(c);
                outputs[GATES_OUTPUT + currentGate[c]].setVoltage(in, c);
                for (int i = 0; i < 4; i++) {
                    if (i != currentGate[c]) outputs[GATES_OUTPUT + i].setVoltage(0.0, c);
                }
            }
        } else { // multinoulli gates (1 in -> 4 outs)
            for (int c = 0; c < channels; c++) {
                bool gate = inputs[GATE_INPUT].getVoltage(c) >= 2.0;
                if (gateTriggers[c].process(gate)) {
                    setCurrentGate(c);
                }

                for (int i = 0; i < 4; i++) {
                    bool rollDice = (i == currentGate[c]);
                    if (!toggle) {
                        outcomes[i][c] = rollDice;
                    } else {
                        if (rollDice) outcomes[i][c] = true;
                        else outcomes[i][c] = false;
                    }
                    bool gateOut = outcomes[i][c] && (toggle ? true : gate);

                    outputs[GATES_OUTPUT + i].setVoltage(gateOut ? 10.0 : 0.0, c);

                    lights[i].setBrightness((i == currentGate[c]) ? 1.0 : 0.0);
                }
            }
        }
        outputs[GATES_OUTPUT + 0].setChannels(channels);
        outputs[GATES_OUTPUT + 1].setChannels(channels);
        outputs[GATES_OUTPUT + 2].setChannels(channels);
        outputs[GATES_OUTPUT + 3].setChannels(channels);
    }

    json_t* dataToJson() override {
		json_t* rootJ = json_object();
        json_object_set_new(rootJ, "mode", json_boolean(toggle));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
        json_t *modeJ = json_object_get(rootJ, "mode");
        if (modeJ) toggle = json_boolean_value(modeJ);
	}
};

struct RandRouteWidget : ModuleWidget {
    RandRouteWidget(RandRoute *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RandRoute.svg")));

        addChild(createWidget<JeremyScrew>(Vec(16.5, 2)));
        addChild(createWidget<JeremyScrew>(Vec(16.5, box.size.y - 14)));

        addInput(createInputCentered<PJ301MPort>(Vec(22.5, 79.4), module, RandRoute::TRIGGER_INPUT));
        addParam(createParamCentered<BlueInvertKnob>(Vec(22.5, 156.1), module, RandRoute::WEIGHTING_PARAM));
        addParam(createParamCentered<NanoBlueKnob>(Vec(34, 139.7), module, RandRoute::PERCENTAGE_PARAM));

        for (int i = 0; i < NUM_OF_OUTPUTS; i++) {
            addOutput(createOutputCentered<PJ301MPort>(Vec(22.5, 200.7 + (i * 40.5)), module, RandRoute::GATES_OUTPUT+i));
        }
        addInput(createInputCentered<PJ301MPort>(Vec(22.5, 119.8), module, RandRoute::GATE_INPUT));

        // lights
        addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(22.5 - 3.21, 340.9 - 3.21), module, RandRoute::PURPLE_LIGHT));
        addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(22.5 - 3.21, 340.9 - 3.21), module, RandRoute::BLUE_LIGHT));
        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(22.5 - 3.21, 340.9 - 3.21), module, RandRoute::AQUA_LIGHT));
        addChild(createLight<SmallLight<JeremyRedLight>>(Vec(22.5 - 3.21, 340.9 - 3.21), module, RandRoute::RED_LIGHT));
    }

	void appendContextMenu(Menu* menu) override {
		RandRoute* module = dynamic_cast<RandRoute*>(this->module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createIndexPtrSubmenuItem("Mode", 
            {"Latch", "Toggle"}, 
            &module->toggle));
	}
};

Model *modelRandRoute = createModel<RandRoute, RandRouteWidget>("RandRoute");
