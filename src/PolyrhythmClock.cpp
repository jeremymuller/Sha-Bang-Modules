#include "plugin.hpp"

struct PolyrhythmClock : Module {
    enum ParamIds {
        CLOCK_TOGGLE_PARAM,
        BPM_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        MASTER_PULSE_OUTPUT,
        EMBED1_OUTPUT,
        EMBED2_OUTPUT,
        EMBED3_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    dsp::PulseGenerator masterPulse;
    dsp::PulseGenerator embed1Pulse, embed2Pulse, embed3Pulse;
    bool clockOn = false;
    float phase = 0;
    float phaseEmbed1 = 0;
    float phaseEmbed2 = 0;
    float phaseEmbed3 = 0;

    PolyrhythmClock() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CLOCK_TOGGLE_PARAM, 0.0, 1.0, 0.0, "toggle clock");
        configParam(BPM_PARAM, -2.0, 6.0, 1.0, "bpm");
    }

    void process(const ProcessArgs& args) override {
        clockOn = (params[CLOCK_TOGGLE_PARAM].getValue() == 1) ? true : false;
        if (clockOn) {
            float time = std::pow(2.0, params[BPM_PARAM].getValue());
            phase += time / args.sampleRate;
            if (phase >= 1.0) {
                phase -= 1.0;
                masterPulse.trigger(1e-3);
            }

            time *= 1.5;
            phaseEmbed1 += time / args.sampleRate;
            if (phaseEmbed1 >= 1.0) {
                phaseEmbed1 -= 1.0;
                embed1Pulse.trigger(1e-3);
            }
            time *= 3;
            phaseEmbed2 += time / args.sampleRate;
            if (phaseEmbed2 >= 1.0) {
                phaseEmbed2 -= 1.0;
                embed2Pulse.trigger(1e-3);
            }
        }

        bool pulse = clockOn && masterPulse.process(1.0 / args.sampleRate);
        bool embed1 = clockOn && embed1Pulse.process(1.0 / args.sampleRate);
        bool embed2 = clockOn && embed2Pulse.process(1.0 / args.sampleRate);
        outputs[MASTER_PULSE_OUTPUT].setVoltage(pulse ? 10.0 : 0.0);
        outputs[EMBED1_OUTPUT].setVoltage(embed1 ? 10.0 : 0.0);
        outputs[EMBED2_OUTPUT].setVoltage(embed2 ? 10.0 : 0.0);
    }
};

struct PolyrhythmClockWidget : ModuleWidget {
    PolyrhythmClockWidget(PolyrhythmClock *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyrhythmClock.svg")));

        addParam(createParamCentered<ToggleButton>(Vec(45, 50.6), module, PolyrhythmClock::CLOCK_TOGGLE_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(45, 80.8), module, PolyrhythmClock::BPM_PARAM));

        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 119.8), module, PolyrhythmClock::MASTER_PULSE_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(45, 201.3), module, PolyrhythmClock::EMBED1_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(45, 255.4), module, PolyrhythmClock::EMBED2_OUTPUT));
    }
};

Model *modelPolyrhythmClock = createModel<PolyrhythmClock, PolyrhythmClockWidget>("PolyrhythmClock");