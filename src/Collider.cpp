#include "plugin.hpp"

#define MIN_SHAKE_ENERGY 0.001

struct Collider : Module {
    enum ParamIds {
        SHAKE_PARAM,
        PARTICLES_PARAM,
        CENTER_FREQ_PARAM,
        FREQ_RANGE_PARAM,
        RANDOMIZE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        SHAKE_INPUT,
        CENTER_FREQ_INPUT,
        FREQ_RANGE_INPUT,
        VEL_INPUT,
        PARTICLES_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
        VEL_OUTPUT,
        GATE_OUTPUT,
        VOLT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
        PURPLE_LIGHT,
        BLUE_LIGHT,
        AQUA_LIGHT,
        RED_LIGHT,
		NUM_LIGHTS
	};

    dsp::SchmittTrigger shakeTrig;
    dsp::PulseGenerator pulses[16];
    bool gates[16];
    bool isShaking = false;
    float ampLevel = 0.0;
    float centerFreq = 2800; // 2800
    float freqSweep = 1.001;
    float centerVoltage = 0.0; 
    float freqRange = 0.2;
    float notes[3];
    float freqs[3];
    float freqRandomize = 0.0;
    int noteIndex = 0;
    float shakeEnergy = 0.0;
    float velocity = 1.0;
    float systemDecay = 0.9999;
    float soundDecay = 0.85;
    int numOfParticles = 50;
    float percentageObj = numOfParticles * 0.027;
    int currentChannel = 0;
    int channels = 1;
    int checkParams = 0;

    Collider() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configButton(SHAKE_PARAM, "Shake");
        configParam(PARTICLES_PARAM, 1.0, 150.0, 50.0, "Number of Particles");
        configParam(CENTER_FREQ_PARAM, 100.0, 10000.0, 2000.0, "Frequency", " Hz");
        configParam(FREQ_RANGE_PARAM, 0.0, 1.0, 0.2, "Frequency Range", " x");
        configParam(RANDOMIZE_PARAM, 0.0, 1.0, 0.0, "Frequency randomization", " x");

        configInput(SHAKE_INPUT, "Shake");
        configInput(CENTER_FREQ_INPUT, "Center frequency");
        configInput(FREQ_RANGE_INPUT, "Frequency range");
        configInput(VEL_INPUT, "Velocity");
        configInput(PARTICLES_INPUT, "Number of Particles");

        configOutput(VOLT_OUTPUT, "Pitch (V/OCT)");
        configOutput(GATE_OUTPUT, "Gate");
        configOutput(VEL_OUTPUT, "Velocity");

        initNotes(centerFreq);
    }

    void initNotes(float center) {
        centerFreq = center;
        notes[0] = freqToMidi(centerFreq);
        notes[1] = freqToMidi(centerFreq * (1.0 - freqRange));
        notes[2] = freqToMidi(centerFreq * (1.0 + freqRange));

        freqs[0] = centerFreq;
        freqs[1] = centerFreq * (1.0 - freqRange);
        freqs[2] = centerFreq * (1.0 + freqRange);
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        json_object_set_new(rootJ, "channels", json_integer(channels));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *channelsJ = json_object_get(rootJ, "channels");
        if (channelsJ) channels = json_integer_value(channelsJ);
    }

    void process(const ProcessArgs &args) override {
        if (checkParams == 0) {
            if (params[SHAKE_PARAM].getValue() + inputs[SHAKE_INPUT].getVoltage()) {
                shakeEnergy = velocity;
            }

            if (inputs[VEL_INPUT].isConnected()) {
                float volts = inputs[VEL_INPUT].getVoltage() / 10.0;
                if (volts != velocity) {
                    velocity = volts;
                }
            } else {
                velocity = 1.0;
            }

            freqRandomize = params[RANDOMIZE_PARAM].getValue();

            if (inputs[PARTICLES_INPUT].isConnected()) {
                float cv = inputs[PARTICLES_INPUT].getVoltage() / 10.0;
                cv = cv * cv;
                numOfParticles = (int)rescale(cv, 0.0, 1.0, 1.0, 150.0);
                percentageObj = numOfParticles * 0.027;
            } else if (params[PARTICLES_PARAM].getValue() != numOfParticles) {
                numOfParticles = (int)params[PARTICLES_PARAM].getValue();
                percentageObj = numOfParticles * 0.027;
            }

            if (inputs[CENTER_FREQ_INPUT].isConnected()) {
                if (inputs[CENTER_FREQ_INPUT].getVoltage() != centerVoltage) {
                    centerVoltage = inputs[CENTER_FREQ_INPUT].getVoltage();
                    centerFreq = dsp::FREQ_C4 * pow(2, centerVoltage);
                    initNotes(centerFreq);
                }
            } else if (params[CENTER_FREQ_PARAM].getValue() != centerFreq) {
                initNotes(params[CENTER_FREQ_PARAM].getValue());
            }

            if (inputs[FREQ_RANGE_INPUT].isConnected()) {
                float cv = clamp(inputs[FREQ_RANGE_INPUT].getVoltage() / 10.f, 0.0, 0.95);
                cv = cv * cv; // square it so it's easy to get low values
                if (cv != freqRange) {
                    freqRange = cv;
                    initNotes(centerFreq);
                }
            } else if (params[FREQ_RANGE_PARAM].getValue() != freqRange) {
                freqRange = clamp(params[FREQ_RANGE_PARAM].getValue(), 0.0, 0.95);
                initNotes(centerFreq);
            }
        }
        checkParams = (checkParams + 1) % 4;

        // this algorithm inspired by Perry Cook's Phisem

        if (shakeEnergy > MIN_SHAKE_ENERGY) {
            shakeEnergy *= systemDecay;
            if (randRange(1024.f) < percentageObj) {
                currentChannel = (currentChannel + 1) % channels;
                ampLevel += shakeEnergy;

                pulses[currentChannel].trigger(1e-3f);

                float freq = freqs[noteIndex] * (1.0 + (freqRandomize * randRange(-1.0, 1.0)));
                float volts = freqToVolts(freq);
                noteIndex = (noteIndex + 1) % 3;
                outputs[VOLT_OUTPUT].setVoltage(volts, currentChannel);

                outputs[VEL_OUTPUT].setVoltage(ampLevel * 10.0, currentChannel);
            }
            ampLevel *= soundDecay;

            bool pulse = pulses[currentChannel].process(args.sampleTime);
            outputs[GATE_OUTPUT].setVoltage(pulse ? 10.0 : 0.0, currentChannel);

        } else {
            for (int i = 0; i < channels; i++) {
                bool pulse = pulses[i].process(args.sampleTime);
                outputs[GATE_OUTPUT].setVoltage(pulse ? 10.0 : 0.0, i);
            }
        }

        outputs[VEL_OUTPUT].setChannels(channels);
        outputs[GATE_OUTPUT].setChannels(channels);
        outputs[VOLT_OUTPUT].setChannels(channels);

    }
};

namespace ColliderNS {
    struct ChannelValueItem : MenuItem {
        Collider *module;
        int channels;
        void onAction(const event::Action &e) override {
            module->channels = channels;
        }
    };

    struct ChannelItem : MenuItem {
        Collider *module;
        Menu *createChildMenu() override {
            Menu *menu = new Menu;
            for (int channels = 1; channels <= 16; channels++) {
                ChannelValueItem *item = new ChannelValueItem;
                if (channels == 1)
                    item->text = "Monophonic";
                else
                    item->text = string::f("%d", channels);
                item->rightText = CHECKMARK(module->channels == channels);
                item->module = module;
                item->channels = channels;
                menu->addChild(item);
            }
            return menu;
        }
    };
}

struct ColliderWidget : ModuleWidget {
    ColliderWidget(Collider *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Collider.svg"), asset::plugin(pluginInstance, "res/Collider-dark.svg")));


        // screws
        addChild(createWidget<JeremyScrew>(Vec(17.3, 2)));
        addChild(createWidget<JeremyScrew>(Vec(17.3, box.size.y - 14)));
        addChild(createWidget<JeremyScrew>(Vec(90.7, 2)));
        addChild(createWidget<JeremyScrew>(Vec(90.7, box.size.y - 14)));

        // button
        addParam(createParamCentered<BigButton>(Vec(60, 77.4), module, Collider::SHAKE_PARAM));

        // knobs
        addParam(createParamCentered<BlueKnob>(Vec(29.4, 184.3), module, Collider::CENTER_FREQ_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(61.5, 184.3), module, Collider::FREQ_RANGE_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(93.6, 184.3), module, Collider::RANDOMIZE_PARAM));
        addParam(createParamCentered<BlueInvertKnob>(Vec(77.6, 245.8), module, Collider::PARTICLES_PARAM));

        // inputs
        addInput(createInputCentered<PJ301MPort>(Vec(29.4, 106), module, Collider::SHAKE_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(29.4, 155.9), module, Collider::CENTER_FREQ_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(61.5, 155.9), module, Collider::FREQ_RANGE_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(93.6, 155.9), module, Collider::VEL_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(45.4, 245.8), module, Collider::PARTICLES_INPUT));

        // outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(29.4, 300.8), module, Collider::VOLT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(61.5, 300.8), module, Collider::GATE_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(93.6, 300.8), module, Collider::VEL_OUTPUT));
    }

    void appendContextMenu(Menu *menu) override {
        Collider *module = dynamic_cast<Collider*>(this->module);
        menu->addChild(new MenuEntry);

        ColliderNS::ChannelItem *channelItem = new ColliderNS::ChannelItem;
        channelItem->text = "Polyphony channels";
        channelItem->rightText = string::f("%d", module->channels) + " " + RIGHT_ARROW;
        channelItem->module = module;
        menu->addChild(channelItem);
    }
};

Model *modelCollider = createModel<Collider, ColliderWidget>("Collider");
