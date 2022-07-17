#include "plugin.hpp"

#define MAX_CHANNELS 16

// {step: ratio}
// {1 : 16:15, 2 : 9:8, 3 : 6:5, 4 : 5:4, 5 : 4:3, 6 : 7:5, 7 : 3:2, 8 : 8:5, 9 : 5:3, 10 : 9:5, 11 : 15:8, 12 : 2:1}

struct PitchSet {
    struct Note {
        float pitch;
        int channel;
        int polyOctave;
        Note() {
            // ?
            pitch = -5.0;
            channel = -1;
            polyOctave = 0;
        }

        Note(float _pitch, int _channel) {
            pitch = _pitch;
            channel = _channel;
            polyOctave = 0;
        }
    };

    int noteCount = 0;
    int currentOctave = 0;
    int transposition = 0;
    int playIndeces[MAX_CHANNELS];
    Note *sortedNotes = new Note[MAX_CHANNELS];
    Note *notesAsPlayed = new Note[MAX_CHANNELS];
    std::vector<Note> notesQueue;

    PitchSet() {
        for (int i = 0; i < MAX_CHANNELS; i++) {
            playIndeces[i] = 0;
        }
    }

    ~PitchSet() {
        delete[] sortedNotes;
        delete[] notesAsPlayed;
    }

    void resetSortedNotes(int index) {
        sortedNotes[index].pitch = -5.0;
        sortedNotes[index].channel = -1;
        sortedNotes[index].polyOctave = 0;
    }

    void resetNotesAsPlayed(int index) {
        notesAsPlayed[index].pitch = -5.0;
        notesAsPlayed[index].channel = -1;
    }

    void addNote(float _pitch, int _channel) {
        noteCount++;
        Note n(_pitch, _channel);
        if (noteCount == 1) {
            sortedNotes[0] = n;
            notesAsPlayed[0] = n;
            transposition = static_cast<int>(std::round(n.pitch * 12.0));
        } else {
            // find insertion index
            int insertIndex = 0;
            while (_pitch >= sortedNotes[insertIndex].pitch && insertIndex < noteCount - 1) {
                insertIndex++;
            }
            for (int i = noteCount; i > insertIndex; i--) {
                sortedNotes[i] = sortedNotes[i - 1];
            }
            sortedNotes[insertIndex] = n;
            notesAsPlayed[noteCount - 1] = n;
        }
    }

    void removeNote(int _channel) {
        // TODO: clean this up since a lot of this is redundant

        if (noteCount > 0) noteCount--;
        // sorted notes
        int removeIndex = 0;
        while (sortedNotes[removeIndex].channel != _channel && removeIndex < noteCount) {
            removeIndex++;
        }
        for (int i = noteCount + 1; removeIndex < i; removeIndex++) {
            sortedNotes[removeIndex] = sortedNotes[removeIndex + 1];
        }
        for (int i = noteCount + 1; i < MAX_CHANNELS; i++) {
            resetSortedNotes(i);
        }

        // as played notes
        removeIndex = 0;
        while (notesAsPlayed[removeIndex].channel != _channel && removeIndex < noteCount) {
            removeIndex++;
        }
        for (int i = noteCount + 1; removeIndex < i; removeIndex++) {
            notesAsPlayed[removeIndex] = notesAsPlayed[removeIndex + 1];
        }
        for (int i = noteCount + 1; i < MAX_CHANNELS; i++) {
            resetNotesAsPlayed(i);
        }
    }

    float getNextPitch(int playIndex) {
        return sortedNotes[playIndex].pitch + currentOctave;
    }

    float getAsPlayedPitch(int playIndex) {
        return notesAsPlayed[playIndex].pitch + currentOctave;
    }

    float getPitchFromChannel(int _channel) {
        // find index
        int i = 0;
        while (sortedNotes[i].channel != _channel) {
            i++;
        }
        return sortedNotes[i].pitch;
    }

    int getChannel(int _index) {
        return sortedNotes[_index].channel;
    }

    bool isNoteFromChannel(int _channel) {
        for (int i = 0; i < noteCount; i++) {
            if (sortedNotes[i].channel == _channel)
                return true;
        }
        return false;
    }

    int getOctaveFromChannel(int _channel) {
        // find index
        int i = 0;
        while (sortedNotes[i].channel != _channel) {
            i++;
        }
        return sortedNotes[i].polyOctave;
    }

    void incrementOctaveFromChannel(int _channel, int _octaveCount) {
        // find index
        int i = 0;
        while (sortedNotes[i].channel != _channel && i < MAX_CHANNELS) {
            i++;
        }
        if (i < MAX_CHANNELS)
            sortedNotes[i].polyOctave = (sortedNotes[i].polyOctave + 1) % _octaveCount;
    }
};

struct Talea : Module {
    enum OctLights {
        OFF = 1,
        PURPLE,
        BLUE,
        AQUA,
        RED,
        NUM_OCT_LIGHTS
    };
    enum BPMModes {
        BPM_CV,
        BPM_WHOLE_NOTE,
        BPM_HALF_NOTE,
        BPM_QUARTER_NOTE,
        BPM_8TH,
        BPM_16TH,
        BPM_32ND,
        BPM_P12,
        BPM_P24,
        NUM_BPM_MODES
    };
    enum ArpIds {
        UP,
        DOWN,
        DOUBLE,
        AS_PLAYED,
        RANDOM,
        NUM_ARP_MODES
    };
    enum ParamIds {
        CLOCK_TOGGLE_PARAM,
        BPM_PARAM,
        OCT_PARAM,
        MODE_PARAM,
        POLYRHYTHM_MODE_PARAM,
        HOLD_PARAM,
        GATE_LENGTH_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        EXT_CLOCK_INPUT,
        VOLTS_INPUT,
        GATES_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        VOLTS_OUTPUT,
        GATES_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        TOGGLE_LIGHT,
        HOLD_LIGHT,
        POLYRHYTHM_MODE_LIGHT,
        OCT_LIGHT = POLYRHYTHM_MODE_LIGHT + NUM_OCT_LIGHTS,
        NUM_LIGHTS = OCT_LIGHT + NUM_OCT_LIGHTS
    };

    dsp::SchmittTrigger toggleTrig, bpmInputTrig, octTrig, holdTrig, polyrhythmModeTrig;
    dsp::SchmittTrigger gateTriggers[MAX_CHANNELS];
    bool clockOn = true;
    bool anyGateOn = false;
    int bpmInputMode = BPM_QUARTER_NOTE;
    int ppqn = 0;
    float noteDur = 1.0;
    float period = 0.0;
    float gateLength = 0.5;
    int timeOut = 2;  // seconds
    int extPulseIndex = 0;
    int currentPitch = 0;
    int playIndex = 0;
    int playIndexDouble = 0;
    int octaveCount = 1;
    int arpMode = Talea::UP;
    int checkParams = 0;
    float clockFreq = 2.0;  // Hz
    bool holdPattern = false;
    bool polyrhythmMode = false;
    bool fixedMode = true;
    float phases[MAX_CHANNELS] = {};
    float volts[MAX_CHANNELS] = {};
    bool gates[MAX_CHANNELS];
    bool gatesHigh[MAX_CHANNELS];
    // {1 : 16:15, 2 : 9:8, 3 : 6:5, 4 : 5:4, 5 : 4:3, 6 : 7:5, 7 : 3:2, 8 : 8:5, 9 : 5:3, 10 : 9:5, 11 : 15:8, 12 : 2:1}
    float ratios[13] = {1.0, 1.0666666666666667, 1.125, 1.2, 1.25, 1.3333333333333333, 1.4, 1.5, 1.6, 1.6666666666666667, 1.8, 1.875, 2.0};
    PitchSet pitchSet;

    Talea() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configButton(CLOCK_TOGGLE_PARAM, "toggle clock");
        configParam(BPM_PARAM, -2.0, 6.0, 1.0, "Tempo", " bpm", 2.0, 60.0);
        configParam(OCT_PARAM, 1.0, 5.0, 1.0, "Octaves");
        configSwitch(MODE_PARAM, 0.0, Talea::NUM_ARP_MODES - 1, 0.0, "Pattern Mode", {"Up", "Down", "Doubled", "As played", "Random"});
        configSwitch(HOLD_PARAM, 0.0, 1.0, 0.0, "Hold Pattern", {"off", "on"});
        configParam(GATE_LENGTH_PARAM, 0.0, 1.0, 0.5, "Gate length", "%", 0.0, 100.0);
        configSwitch(POLYRHYTHM_MODE_PARAM, 0.0, 1.0, 0.0, "Polyrhythm Mode", {"off", "on"});

        configInput(EXT_CLOCK_INPUT, "External clock");
        configInput(VOLTS_INPUT, "Pitch (V/OCT)");
        configInput(GATES_INPUT, "Gate");

        configOutput(VOLTS_OUTPUT, "Pitch (V/OCT)");
        configOutput(GATES_OUTPUT, "Gate");

        for (int i = 0; i < MAX_CHANNELS; i++) {
            phases[i] = 0.0;
            gates[i] = false;
            gatesHigh[i] = false;
        }
    }

    int getPolyrhythmMode() {
        return fixedMode ? 0 : 1;
    }

    void setPolyrhthmMode(int mode) {
        fixedMode = (mode == 0);
    }

    void incPlayIndex() {
        bool incrementOct = false;
        if (pitchSet.noteCount < 1) {
            playIndex = 0;
            incrementOct = true;
        } else {
            switch (arpMode) {
                case Talea::AS_PLAYED:
                case Talea::UP:
                    playIndex = (playIndex + 1) % pitchSet.noteCount;
                    incrementOct = (playIndex == 0);
                    break;
                case Talea::DOWN:
                    playIndex = (playIndex + (pitchSet.noteCount - 1)) % pitchSet.noteCount;
                    incrementOct = (playIndex == (pitchSet.noteCount - 1));
                    break;
                case Talea::DOUBLE:
                    if (playIndexDouble % 2 == 1) playIndex = (playIndex + 1) % pitchSet.noteCount;
                    incrementOct = (playIndex == 0 && playIndexDouble == 1);
                    playIndexDouble = (playIndexDouble + 1) % 2;
                    break;
                case Talea::RANDOM:
                    playIndex = static_cast<int>(random::uniform() * pitchSet.noteCount);
                    incrementOct = random::uniform() < 0.5;
                    break;
                default:
                    playIndex = 0;
            }
        }
        if (incrementOct) {
            pitchSet.currentOctave = (pitchSet.currentOctave + 1) % octaveCount;
        }
    }

    // void incPlayIndex(int _whichIndex) {
    //     if (pitchSet.noteCount < 1) {
    //         pitchSet.playIndeces[_whichIndex] = 0;
    //     } else {
    //         pitchSet.playIndeces[_whichIndex] = (pitchSet.playIndeces[_whichIndex] + 1) % pitchSet.noteCount;
    //     }
    // }

    void checkPhases(int index, int channels) {
        if (pitchSet.notesQueue.size() > 0) {
            for (unsigned int i = 0; i < pitchSet.notesQueue.size(); i++) {
                PitchSet::Note n = pitchSet.notesQueue[i];
                pitchSet.addNote(n.pitch, n.channel);
            }
            pitchSet.notesQueue.clear();
        }
        if (phases[index] >= 1.0) {
            phases[index] -= 1.0;
            // phases[index] = 0.0;
            // if (pitchSet.notesQueue.size() > 0) {
            //     for (unsigned int i = 0; i < pitchSet.notesQueue.size(); i++) {
            //         PitchSet::Note n = pitchSet.notesQueue[i];
            //         pitchSet.addNote(n.pitch, n.channel);
            //     }
            //     pitchSet.notesQueue.clear();
            // }
            incPlayIndex();
            // playIndex = (pitchSet.noteCount < 1) ? 0 : (playIndex+1) % pitchSet.noteCount;
        }
        gates[index] = (phases[index] < gateLength);
    }

    float getRatioFromVolts(float _volts) {
        int trans = 0;
        if (!fixedMode) trans = pitchSet.transposition;

        int pitch = static_cast<int>(std::round(_volts * 12.0)) - trans;
        // DEBUG("pitch: %d", pitch);

        int index;
        if (pitch < 0) {
            index = (pitch % 12 + 12) % 12;
        } else {
            index = pitch % 12;
        }
        // int octave = floor(_volts);
        int octave = floor(pitch / 12.0);
        return ratios[index] * std::pow(2.0, octave);
    }

    void checkPhases(int _index) {  // (overloaded for polyrhythm stuff)
        if (phases[_index] >= 1.0) {
            phases[_index] -= 1.0;

            if (pitchSet.notesQueue.size() > 0) {
                for (unsigned int i = 0; i < pitchSet.notesQueue.size(); i++) {
                    PitchSet::Note n = pitchSet.notesQueue[i];
                    bool newNote = true;
                    for (int j = 0; j < pitchSet.noteCount; j++) {
                        if (n.pitch == pitchSet.sortedNotes[j].pitch)
                            newNote = false;
                    }
                    if (newNote)
                        pitchSet.addNote(n.pitch, n.channel);
                }
                pitchSet.notesQueue.clear();
            }
            // incPlayIndex();
            // incPlayIndex(_index);
            if (pitchSet.noteCount > 0)
                pitchSet.incrementOctaveFromChannel(_index, octaveCount);
        }
        gates[_index] = (phases[_index] < gateLength);
    }

    void removeAllNotes(int _channels) {
        for (int c = 0; c < _channels; c++) {
            pitchSet.resetSortedNotes(c);
            pitchSet.resetNotesAsPlayed(c);
        }
        pitchSet.noteCount = 0;
        pitchSet.currentOctave = 0;
        playIndexDouble = 0;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        json_object_set_new(rootJ, "clockOn", json_boolean(clockOn));
        json_object_set_new(rootJ, "polyrhythmMode", json_boolean(polyrhythmMode));
        json_object_set_new(rootJ, "fixedMode", json_boolean(fixedMode));
        json_object_set_new(rootJ, "extmode", json_integer(bpmInputMode));
        json_object_set_new(rootJ, "octaveCount", json_integer(octaveCount));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *clockOnJ = json_object_get(rootJ, "clockOn");
        if (clockOnJ) clockOn = json_boolean_value(clockOnJ);

        json_t *polyrhythmModeJ = json_object_get(rootJ, "polyrhythmMode");
        if (polyrhythmModeJ) polyrhythmMode = json_boolean_value(polyrhythmModeJ);

        json_t *fixedModeJ = json_object_get(rootJ, "fixedMode");
        if (fixedModeJ) fixedMode = json_boolean_value(fixedModeJ);

        json_t *extmodeJ = json_object_get(rootJ, "extmode");
        if (extmodeJ) bpmInputMode = json_integer_value(extmodeJ);

        json_t *octaveCountJ = json_object_get(rootJ, "octaveCount");
        if (octaveCountJ) octaveCount = json_integer_value(octaveCountJ);
    }

    void process(const ProcessArgs &args) override {
        if (checkParams == 0) {
            // if (octTrig.process(params[OCT_PARAM].getValue())) {
            //     octaveCount = octaveCount < 5 ? (octaveCount + 1) : 1;
            // }
            octaveCount = params[OCT_PARAM].getValue();
            holdPattern = (params[HOLD_PARAM].getValue() == 1);
            polyrhythmMode = (params[POLYRHYTHM_MODE_PARAM].getValue() == 1);

            if (toggleTrig.process(params[CLOCK_TOGGLE_PARAM].getValue())) {
                clockOn = !clockOn;
            }

            // if (holdTrig.process(params[HOLD_PARAM].getValue())) {
            //     holdPattern = !holdPattern;
            // }

            // if (polyrhythmModeTrig.process(params[POLYRHYTHM_MODE_PARAM].getValue())) {
            //     polyrhythmMode = !polyrhythmMode;
            // }

            gateLength = params[GATE_LENGTH_PARAM].getValue();

            arpMode = static_cast<int>(params[MODE_PARAM].getValue());
        }
        checkParams = (checkParams + 1) % 4;

        lights[TOGGLE_LIGHT].setBrightness(clockOn ? 1.0 : 0.0);
        lights[HOLD_LIGHT].setBrightness(holdPattern ? 1.0 : 0.0);
        lights[POLYRHYTHM_MODE_LIGHT].setBrightness(polyrhythmMode ? 1.0 : 0.0);

        lights[OCT_LIGHT + PURPLE].setBrightness((octaveCount == PURPLE) ? 1.0 : 0.0);
        lights[OCT_LIGHT + BLUE].setBrightness((octaveCount == BLUE) ? 1.0 : 0.0);
        lights[OCT_LIGHT + AQUA].setBrightness((octaveCount == AQUA) ? 1.0 : 0.0);
        lights[OCT_LIGHT + RED].setBrightness((octaveCount == RED) ? 1.0 : 0.0);

        bool bpmDetect = false;
        if (inputs[EXT_CLOCK_INPUT].isConnected()) {
            if (bpmInputMode == BPM_CV) {
                clockFreq = 2.0 * std::pow(2.0, inputs[EXT_CLOCK_INPUT].getVoltage());
            } else {
                bpmDetect = bpmInputTrig.process(inputs[EXT_CLOCK_INPUT].getVoltage());
                if (bpmDetect)
                    clockOn = true;
                switch (bpmInputMode) {
                    case BPM_WHOLE_NOTE:
                        noteDur = 0.25;
                        break;
                    case BPM_HALF_NOTE:
                        noteDur = 0.5;
                        break;
                    case BPM_QUARTER_NOTE:
                        noteDur = 1.0;
                        break;
                    case BPM_8TH:
                        noteDur = 2.0;
                        break;
                    case BPM_16TH:
                        noteDur = 4.0;
                        break;
                    case BPM_32ND:
                        noteDur = 8.0;
                        break;
                    case BPM_P12:
                        ppqn = 12;
                        break;
                    case BPM_P24:
                        ppqn = 24;
                        break;
                }
            }

        } else { // if no ext clock input then use the bpm knob
            float bpmParam = params[BPM_PARAM].getValue();
            clockFreq = std::pow(2.0, bpmParam);
        }

        if (inputs[VOLTS_INPUT].isConnected() && inputs[GATES_INPUT].isConnected()) {
            int channels = inputs[VOLTS_INPUT].getChannels();
            if (clockOn) {
                if (inputs[EXT_CLOCK_INPUT].isConnected()) {
                    if (bpmInputMode > BPM_32ND) {
                        period += args.sampleTime;
                        if (bpmDetect) {
                            if (extPulseIndex > 0) {
                                clockFreq = (1.0 / period) / (float)ppqn;
                            }

                            extPulseIndex++;
                            if (extPulseIndex >= ppqn) extPulseIndex = 0;
                            period = 0.0;
                        }
                    } else if (bpmInputMode != BPM_CV) {
                        period += args.sampleTime;
                        if (bpmDetect) {
                            if (period > 0.0) {
                                clockFreq = 1.0 / period * noteDur;
                            }
                            period = 0.0;
                        }
                    }
                    if (period > timeOut) clockOn = false;
                }

                // code inspired from
                // https://github.com/bogaudio/BogaudioModules/blob/master/src/Arp.cpp
                bool wasGateOn = anyGateOn;
                anyGateOn = false;
                bool firstNote = true;
                for (int c = 0; c < channels; c++) {
                    if (gateTriggers[c].process(inputs[GATES_INPUT].getPolyVoltage(c))) {
                        if (firstNote && holdPattern && !wasGateOn) {
                            removeAllNotes(channels);
                        }
                        PitchSet::Note n(inputs[VOLTS_INPUT].getPolyVoltage(c), c);
                        pitchSet.notesQueue.push_back(n);
                        gatesHigh[c] = true;
                        anyGateOn = true;
                        firstNote = false;
                    } else if (gatesHigh[c]) {
                        if (!gateTriggers[c].isHigh()) {
                            gatesHigh[c] = false;
                            if (!holdPattern) {
                                pitchSet.removeNote(c);
                            }
                        } else {
                            anyGateOn = true;
                        }
                    }
                }
                if (anyGateOn || holdPattern) {
                    if (pitchSet.noteCount > 0 || pitchSet.notesQueue.size() > 0) {
                        if (polyrhythmMode) {
                            outputs[VOLTS_OUTPUT].setChannels(channels);
                            outputs[GATES_OUTPUT].setChannels(channels);
                            for (int c = 0; c < channels; c++) {
                                float fractionRhythm = 1.0;
                                if (pitchSet.isNoteFromChannel(c)) {
                                    float volts = pitchSet.getPitchFromChannel(c);
                                    fractionRhythm = getRatioFromVolts(volts);
                                    int _oct = pitchSet.getOctaveFromChannel(c);
                                    if (arpMode == DOWN) _oct *= -1;
                                    volts += _oct;
                                    outputs[VOLTS_OUTPUT].setVoltage(volts, c);
                                    outputs[GATES_OUTPUT].setVoltage(gates[c] ? 5.0 : 0.0, c);

                                } else {
                                    outputs[GATES_OUTPUT].setVoltage(0.0, c);
                                }

                                phases[c] += clockFreq * args.sampleTime * fractionRhythm;

                                checkPhases(c);
                            }
                        } else {
                            outputs[VOLTS_OUTPUT].setChannels(channels);
                            outputs[GATES_OUTPUT].setChannels(channels);
                            float note = 0.0;
                            if (arpMode == AS_PLAYED)
                                note = pitchSet.getAsPlayedPitch(playIndex);
                            else
                                note = pitchSet.getNextPitch(playIndex);

                            outputs[VOLTS_OUTPUT].setVoltage(note, 0);
                            outputs[GATES_OUTPUT].setVoltage(gates[0] ? 5.0 : 0.0, 0);
                            phases[0] += clockFreq * args.sampleTime;
                            checkPhases(0, channels);
                        }
                    }
                } else {
                    removeAllNotes(channels);
                    for (int c = 0; c < channels; c++) {
                        phases[c] = 0.0;
                        outputs[GATES_OUTPUT].setVoltage(0.0, c);
                    }
                }
            }
        }
    }
};

namespace TaleaNS {

struct TaleaModeKnob : BlueInvertKnobLabelCentered {
    TaleaModeKnob() {}
    std::string formatCurrentValue() override {
        if (getParamQuantity() != NULL) {
            switch (int(getParamQuantity()->getValue())) {
                case Talea::UP:
                    return "↑";
                case Talea::DOWN:
                    return "↓";
                case Talea::DOUBLE:
                    return "2x";
                case Talea::AS_PLAYED:
                    return "⚡︎";
                // case Talea::AS_PLAYED:  return "→";
                case Talea::RANDOM:
                    return "R";
            }
        }
        return "";
    }
};

}  // namespace TaleaNS

struct TaleaWidget : ModuleWidget {
    TaleaWidget(Talea *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Talea.svg")));

        addChild(createWidget<JeremyScrew>(Vec(16.5, 2)));
        addChild(createWidget<JeremyScrew>(Vec(16.5, box.size.y - 14)));
        // light
        // addChild(createLight<SmallLight<DisplayAquaLight> >(Vec(34 - 3, 40.3 - 3), module, Talea::TOGGLE_LIGHT));
        addChild(createLight<SmallLight<DisplayAquaLight>>(Vec(34 - 3, 54 - 3), module, Talea::TOGGLE_LIGHT));

        addParam(createParamCentered<TinyBlueButton>(Vec(34, 54), module, Talea::CLOCK_TOGGLE_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(21.9, 76.7), module, Talea::BPM_PARAM));
        addParam(createParamCentered<TinyBlueKnob>(Vec(11, 97), module, Talea::GATE_LENGTH_PARAM));

        // octaves
        addParam(createParamCentered<NanoBlueButton>(Vec(11, 124.7), module, Talea::OCT_PARAM));
        addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(11 - 3, 124.7 - 3), module, Talea::OCT_LIGHT + Talea::PURPLE));
        addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(11 - 3, 124.7 - 3), module, Talea::OCT_LIGHT + Talea::BLUE));
        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(11 - 3, 124.7 - 3), module, Talea::OCT_LIGHT + Talea::AQUA));
        addChild(createLight<SmallLight<JeremyRedLight>>(Vec(11 - 3, 124.7 - 3), module, Talea::OCT_LIGHT + Talea::RED));

        // hold
        addParam(createParamCentered<NanoBlueButton>(Vec(34, 97), module, Talea::HOLD_PARAM));
        // addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(34 - 3, 97 - 3), module, Talea::HOLD_LIGHT));
        // addChild(createLight<SmallLight<JeremyRedLight>>(Vec(34 - 3, 97 - 3), module, Talea::HOLD_LIGHT));
        // polyrhythm mode
        addParam(createParamCentered<NanoBlueButton>(Vec(11, 137.2), module, Talea::POLYRHYTHM_MODE_PARAM));
        // addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(11 - 3, 137.2 - 3), module, Talea::POLYRHYTHM_MODE_LIGHT));
        // addChild(createLight<SmallLight<JeremyRedLight>>(Vec(11 - 3, 137.2 - 3), module, Talea::POLYRHYTHM_MODE_LIGHT));

        TaleaNS::TaleaModeKnob *modeKnob = dynamic_cast<TaleaNS::TaleaModeKnob *>(createParamCentered<TaleaNS::TaleaModeKnob>(Vec(22.5, 158.7), module, Talea::MODE_PARAM));
        CenterAlignedLabel *const modeLabel = new CenterAlignedLabel;
        modeLabel->box.pos = Vec(22.5, 182.7);
        modeLabel->text = "";
        modeKnob->connectLabel(modeLabel, module);
        modeLabel->color = nvgRGB(38, 0, 255);
        addChild(modeLabel);
        addParam(modeKnob);

        // external clock
        addInput(createInputCentered<TinyPJ301M>(Vec(11, 54), module, Talea::EXT_CLOCK_INPUT));

        addInput(createInputCentered<PJ301MPort>(Vec(22.5, 212.2), module, Talea::VOLTS_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(22.5, 250.2), module, Talea::GATES_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(Vec(22.5, 293), module, Talea::VOLTS_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(22.5, 331), module, Talea::GATES_OUTPUT));
    }

    void appendContextMenu(Menu *menu) override {
        Talea *module = dynamic_cast<Talea *>(this->module);
        menu->addChild(new MenuEntry);

        menu->addChild(createIndexPtrSubmenuItem("External Clock Mode", {"CV (0V = 120 bpm)", "Whole note", "Half note", "Quarter note", "8th note", "16th note", "32nd note", "12 PPQN", "24 PPQN"}, &module->bpmInputMode));

        menu->addChild(createIndexSubmenuItem(
            "Polyrhythm Mode", {"fixed", "movable"},
            [=]() { return module->getPolyrhythmMode(); },
            [=](int mode) {
                module->setPolyrhthmMode(mode);
            }));
    }
};

Model *modelTalea = createModel<Talea, TaleaWidget>("Talea");
