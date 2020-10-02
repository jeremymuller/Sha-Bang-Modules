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

    dsp::BiquadFilter hip; 

    QubitCrusher() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override {
        // this works for the most part but I may try a different way
        int channels = std::max(inputs[MAIN_INPUT].getChannels(), 1);

        for (int c = 0; c < channels; c++) {
            float in = inputs[MAIN_INPUT].getVoltage(c);
            in = rescale(in, -5.0, 5.0, 0.0, 1.0);
            float scl = std::pow(2, 4.0) - 1;
            in = static_cast<int>(in * scl);
            in /= scl;
            in = rescale(in, 0.0, 1.0, -5.0, 5.0);

            // add DC offset
            hip.setParameters(dsp::BiquadFilter::HIGHPASS_1POLE, 5.0 / args.sampleRate, 0.0, 0.0);
            float out = hip.process(in);

            outputs[MAIN_OUTPUT].setVoltage(out, c);
        }
        outputs[MAIN_OUTPUT].setChannels(channels);
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