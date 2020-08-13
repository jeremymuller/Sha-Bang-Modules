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
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    dsp::PulseGenerator masterPulse;
    bool clockOn = false;
    float phase = 0;

    PolyrhythmClock() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CLOCK_TOGGLE_PARAM, 0.0, 1.0, 0.0, "toggle clock");
        configParam(BPM_PARAM, 30, 3000, 120, "bpm");
    }

    void process(const ProcessArgs& args) override {
        clockOn = (params[CLOCK_TOGGLE_PARAM].getValue() == 1) ? true : false;
        if (clockOn) {
            float time = (int)params[BPM_PARAM].getValue() / 60.0;
            phase += time / args.sampleRate;
            if (phase >= 1.0) {
                phase -= 1.0;
                masterPulse.trigger(1e-3);
            }
        }

        bool pulse = clockOn && masterPulse.process(1.0 / args.sampleRate);
        outputs[MASTER_PULSE_OUTPUT].setVoltage(pulse ? 10.0 : 0.0);
    }
};

struct PolyrhythmClockWidget : ModuleWidget {
    PolyrhythmClockWidget(PolyrhythmClock *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyrhythmClock.svg")));

        addParam(createParamCentered<ToggleButton>(Vec(45, 50.6), module, PolyrhythmClock::CLOCK_TOGGLE_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(45, 80.8), module, PolyrhythmClock::BPM_PARAM));

        addOutput(createOutputCentered<PJ301MPort>(Vec(45, 119.8), module, PolyrhythmClock::MASTER_PULSE_OUTPUT));
    }
};

Model *modelPolyrhythmClock = createModel<PolyrhythmClock, PolyrhythmClockWidget>("PolyrhythmClock");