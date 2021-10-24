#include "plugin.hpp"

struct QubitCrusher : Module {
    enum ParamIds {
        BITS_PARAM,
        BITS_MOD_PARAM,
        SAMP_HOLD_PARAM,
        SAMP_HOLD_MOD_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        BITS_MOD_INPUT,
        RAND_BITS_INPUT,
        SAMP_HOLD_MOD_INPUT,
        RAND_SAMP_INPUT,
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
    float out = 0.0;
    float bitRate = 8.0;
    float sampHold = 0.0;
    float sr = 1.0;

    QubitCrusher() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BITS_PARAM, 1.0, 16.0, 8.0, "Bit rate");
        configParam(BITS_MOD_PARAM, 0.0, 1.0, 0.0, "Bit rate modulation", "%", 0.0, 100.0);
        configParam(SAMP_HOLD_PARAM, 0.00001, 1.0, 1.0, "Downsampling", " Hz", 0, 44100);
        configParam(SAMP_HOLD_MOD_PARAM, 0.0, 1.0, 0.0, "Sample rate modulation", "%", 0.0, 100.0);

        configInput(BITS_MOD_INPUT, "Bit rate modulation");
        configInput(RAND_BITS_INPUT, "Randomize bit rate");

        configInput(SAMP_HOLD_MOD_INPUT, "Sample rate modulation");
        configInput(RAND_SAMP_INPUT, "Randomize sample rate");

        configInput(MAIN_INPUT, "Audio");
        configOutput(MAIN_OUTPUT, "Audio");

        configBypass(MAIN_INPUT, MAIN_OUTPUT);
    }

    void process(const ProcessArgs &args) override {
        if (!inputs[MAIN_INPUT].isConnected()) {
            return;
        }

        int channels = std::max(inputs[MAIN_INPUT].getChannels(), 1);
        if (inputs[RAND_BITS_INPUT].isConnected()) {
            if (inputs[RAND_BITS_INPUT].getVoltage()) bitRate = randRange(1, 8);
        } else {
            bitRate = params[BITS_PARAM].getValue();
        }

        if (inputs[RAND_SAMP_INPUT].isConnected()) {
            if (inputs[RAND_SAMP_INPUT].getVoltage()) sr = randRange(0.01, 0.5);
        } else {
            sr = params[SAMP_HOLD_PARAM].getValue();
        }

        // TODO: which one of these?
        // float bitModParam = dsp::quadraticBipolar(params[BITS_MOD_PARAM].getValue());
        float bitModParam = params[BITS_MOD_PARAM].getValue();
        float sampModParam = params[SAMP_HOLD_MOD_PARAM].getValue();
        // float sampH = params[SAMP_HOLD_PARAM].getValue();

        if (sampHold >= 1.0) {
            sampHold = 0.0;
        }

        for (int c = 0; c < channels; c++) {

            if (sampHold == 0) {
                if (inputs[BITS_MOD_INPUT].isConnected()) {
                    bitRate += bitModParam * inputs[BITS_MOD_INPUT].getVoltage(c);
                    bitRate = clamp(bitRate, 1.0, 16.0);
                }
                if (inputs[SAMP_HOLD_MOD_INPUT].isConnected()) {
                    sr += sampModParam * inputs[SAMP_HOLD_MOD_INPUT].getVoltage(c);
                    sr = clamp(sr, 0.01, 1.0);
                }
                float in = inputs[MAIN_INPUT].getVoltage(c);
                float numBits = std::pow(2, bitRate) - 1;
                in = in * 0.1 + 0.5;
                out = round(in * numBits) / numBits;
                out = out * 10 - 5; // scale it back up
            }


            // in = rescale(in, -5.0, 5.0, 0.0, 1.0);
            // float scl = std::pow(2, bitRate); // subtract 1?
            // in = static_cast<int>(in * scl);
            // in /= scl;
            // in = rescale(in, 0.0, 1.0, -5.0, 5.0);

            // add DC offset
            // hip.setParameters(dsp::BiquadFilter::HIGHPASS_1POLE, 5.0 / args.sampleRate, 0.0, 0.0);
            // out = hip.process(out);

            outputs[MAIN_OUTPUT].setVoltage(out, c);
        }
        outputs[MAIN_OUTPUT].setChannels(channels);

        sampHold += sr;
    }
};

struct QubitCrusherWidget : ModuleWidget {
    QubitCrusherWidget(QubitCrusher *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/QubitCrusher.svg")));

        addChild(createWidget<JeremyScrew>(Vec(16.5, 2)));
        addChild(createWidget<JeremyScrew>(Vec(16.5, box.size.y - 14)));

        // params
        addParam(createParamCentered<BlueKnob>(Vec(22.5, 79.4), module, QubitCrusher::BITS_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(22.5, 115.1), module, QubitCrusher::BITS_MOD_PARAM));
        addParam(createParamCentered<PurpleKnob>(Vec(22.5, 181.5), module, QubitCrusher::SAMP_HOLD_PARAM));
        addParam(createParamCentered<PurpleKnob>(Vec(22.5, 217.2), module, QubitCrusher::SAMP_HOLD_MOD_PARAM));

        // inputs
        addInput(createInputCentered<PJ301MPort>(Vec(22.5, 281.6), module, QubitCrusher::MAIN_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(11, 143.2), module, QubitCrusher::BITS_MOD_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(34, 245.3), module, QubitCrusher::SAMP_HOLD_MOD_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(34, 143.2), module, QubitCrusher::RAND_BITS_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(11, 245.3), module, QubitCrusher::RAND_SAMP_INPUT));

        // outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(22.5, 322.1), module, QubitCrusher::MAIN_OUTPUT));
    }
};

Model *modelQubitCrusher = createModel<QubitCrusher, QubitCrusherWidget>("QubitCrusher");