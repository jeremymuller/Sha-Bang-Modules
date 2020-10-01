#include "plugin.hpp"

struct QubitCrusher : Module {
    enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
        MAIN_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
        MAIN_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    QubitCrusher() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override {

    }
};

struct QubitCrusherWidget : ModuleWidget {
    QubitCrusherWidget(QubitCrusher *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/QubitCrusher.svg")));

        addChild(createWidget<JeremyScrew>(Vec(16.5, 2)));
        addChild(createWidget<JeremyScrew>(Vec(16.5, box.size.y - 14)));

        addInput(createInputCentered<PJ301MPort>(Vec(22.5, 119.8), module, QubitCrusher::MAIN_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(Vec(22.5, 322.1), module, QubitCrusher::MAIN_OUTPUT));
    }
};

Model *modelQubitCrusher = createModel<QubitCrusher, QubitCrusherWidget>("QubitCrusher");