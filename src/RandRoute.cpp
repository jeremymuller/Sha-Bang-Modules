#include "plugin.hpp"

#define NUM_OF_OUTPUTS 4

struct RandRoute : Module {
    enum ParamIds {
        WEIGHTING_PARAM,
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
    int currentGate;

    RandRoute() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        // configParam(WEIGHTING_PARAM, 0.0, 4.0, 4.0, "weight");
        configSwitch(WEIGHTING_PARAM, 0.0, 4.0, 4.0, "Weight", {"Purple", "Blue", "Aqua", "Red", "Uniform"});

        configInput(TRIGGER_INPUT, "Trigger");
        configInput(GATE_INPUT, "Main");

        configOutput(GATES_OUTPUT, "Purple");
        configOutput(GATES_OUTPUT + 1, "Blue");
        configOutput(GATES_OUTPUT + 2, "Aqua");
        configOutput(GATES_OUTPUT + 3, "Red");

        configLight(PURPLE_LIGHT, "Output indicator");

        setCurrentGate();
    }

    void setCurrentGate() {
        if (params[WEIGHTING_PARAM].getValue() < 4) {
            int weight = (int)params[WEIGHTING_PARAM].getValue();
            int r = static_cast<int>(random::uniform() * (NUM_OF_OUTPUTS+1));
            currentGate = (r > 3) ? weight : r; // 40% chance of weighted choice
        } else {
            currentGate = static_cast<int>(random::uniform() * NUM_OF_OUTPUTS);
        }
    }

    void process(const ProcessArgs &args) override {
        // TODO: fix this routing shizzzz
        if (mainTrig.process(inputs[TRIGGER_INPUT].getVoltage())) {
            setCurrentGate();
        }
        for (int i = 0; i < NUM_LIGHTS; i++) {
            lights[i].setBrightness((i==currentGate) ? 1.0 : 0.0);
        }

        int channels = inputs[GATE_INPUT].getChannels();
        for (int c = 0; c < channels; c++) {
            float in = inputs[GATE_INPUT].getVoltage(c);
            outputs[GATES_OUTPUT + currentGate].setVoltage(in, c);
        }
        outputs[GATES_OUTPUT].setChannels(channels);
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
};

Model *modelRandRoute = createModel<RandRoute, RandRouteWidget>("RandRoute");
