#include "plugin.hpp"

#define SLIDER_TOP 4
#define NUM_OF_CELLS 16
#define CELL_SIZE 67.5
#define MARGIN 1
#define MAX_SUBDIVISIONS 16

enum CellSequencerIds {
    PURPLE_SEQ,
    BLUE_SEQ,
    AQUA_SEQ,
    RED_SEQ,
    NUM_SEQ
};
enum PathIds {
    DEFAULT_PATH,
    RANDOM_PATH,
    RANDOM_WALK_PATH,
    NUM_PATHS
};

struct SeqCell {
    Vec startPos;
    Vec resetPos;
    int currentCellX, currentCellY;
    int currentIndex = -1;
    int length = 16;
    NVGcolor color;
    float phase = 0.0;
    float subPhase = 0.0;
    float rhythm = 1.0;
    float duration = 1.0;
    float volts = 0.0;
    int cellRhythmIndex = 0;
    bool isOn = true;
    bool playCellRhythms = false;
    bool clockGate = false;
    bool gateOn = false;
    CellSequencerIds id;
    bool beatPulse[NUM_OF_CELLS][MAX_SUBDIVISIONS] = {};
    PathIds currentPath = DEFAULT_PATH;
    int pathArray[NUM_OF_CELLS] = {};

    dsp::PulseGenerator gatePulse;

    SeqCell() {
        for (int i = 0; i < NUM_OF_CELLS; i++)
            for (int j = 0; j < MAX_SUBDIVISIONS; j++)
                beatPulse[i][j] = false;
    }

    SeqCell(CellSequencerIds _id) {
        id = _id;

        // set their arrays here because switch() won't let me do it there
        if (id == PURPLE_SEQ) {
            int tempArray[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
            std::copy(tempArray, tempArray + NUM_OF_CELLS, pathArray);
        } else if (id == BLUE_SEQ) {
            int tempArray[] = {3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12};
            std::copy(tempArray, tempArray + NUM_OF_CELLS, pathArray);
        } else if (id == AQUA_SEQ) {
            int tempArray[] = {12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3};
            std::copy(tempArray, tempArray + NUM_OF_CELLS, pathArray);
        } else if (id == RED_SEQ) {
            int tempArray[] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
            std::copy(tempArray, tempArray + NUM_OF_CELLS, pathArray);
        }

        switch (id) {
            case PURPLE_SEQ:
                currentCellX = -1;
                currentCellY = 0;
                startPos = Vec(0, 0);
                resetPos = Vec(-1, 0);
                color = getPurple();
                break;
            case BLUE_SEQ:
                currentCellX = 3;
                currentCellY = -1;
                startPos = Vec(3, 0);
                resetPos = Vec(3, -1);
                color = getBlue();
                break;
            case AQUA_SEQ:
                currentCellX = 0;
                currentCellY = 4;
                startPos = Vec(0, 3);
                resetPos = Vec(0, 4);
                color = getAqua();
                break;
            case RED_SEQ:
                currentCellX = 4;
                currentCellY = 3;
                startPos = Vec(3, 3);
                resetPos = Vec(4, 3);
                color = getRed();
                break;
            default:
                break;
        }
    }

    void doDefaultPath() {
        switch(id) {
            case PURPLE_SEQ:
                currentCellX++;
                if (currentCellX >= 4) {
                    currentCellX = 0;
                    currentCellY = (currentCellY + 1) % 4;
                }
                break;
            case BLUE_SEQ:
                currentCellY++;
                if (currentCellY >= 4) {
                    currentCellY = 0;
                    currentCellX--;
                    if (currentCellX < 0) currentCellX = 3;
                }
                break;
            case AQUA_SEQ:
                currentCellY--;
                if (currentCellY < 0) {
                    currentCellY = 3;
                    currentCellX = (currentCellX + 1) % 4;
                }
                break;
            case RED_SEQ:
                currentCellX--;
                if (currentCellX < 0) {
                    currentCellX = 3;
                    currentCellY--;
                    if (currentCellY < 0) currentCellY = 3;
                }
                break;
            default:
                break;
        }
        cellRhythmIndex = 0;
        currentIndex++;
        if (currentIndex >= length) {
            currentIndex = 0;
            currentCellX = startPos.x;
            currentCellY = startPos.y;
        }
    }

    void doRandomPath() {
        // TODO: fix for blue, aqua, red
        int rIndex = pathArray[randRange(length)];
        Vec pos = getXYfromIndex(rIndex);
        currentCellX = pos.x;
        currentCellY = pos.y;
        cellRhythmIndex = 0;
        currentIndex = (currentIndex + 1) % length;
    }

    void doRandomWalkPath() {
        int dir = randRange(5);
        switch (dir) {
            case 1:
                currentCellY -= 1;
                break;
            case 2:
                currentCellX += 1;
                break;
            case 3:
                currentCellY += 1;
                break;
            case 4:
                currentCellX -= 1;
                break;
            default:
                break;
        }

        // bounce of edges
        if (currentCellX < 0) currentCellX = 1;
        else if (currentCellX > 3) currentCellX = 2;
        if (currentCellY < 0) currentCellY = 1;
        else if (currentCellY > 3) currentCellY = 2;

        cellRhythmIndex = 0;
        currentIndex = (currentIndex + 1) % length;
    }

    void clockStep() {
        if (currentPath == DEFAULT_PATH) {
            doDefaultPath();
        } else if (currentPath == RANDOM_PATH) {
            doRandomPath();
        } else {
            doRandomWalkPath();
        }
    }

    void setBeatPulse() {
        beatPulse[getCurrentCellIndex()][cellRhythmIndex] = gateOn && clockGate;
    }

    void reset() {
        // TODO
        // currentCellX = resetPos.x;
        // currentCellY = resetPos.y;
        // currentIndex = -1;
        currentCellX = startPos.x;
        currentCellY = startPos.y;
        currentIndex = 0;
        cellRhythmIndex = 0;
        phase = 0.0;
        subPhase = 0.0;
    }

    int getCurrentCellIndex() {
        return clamp(currentCellX, 0, 3) + clamp(currentCellY, 0, 3) * 4;
    }

    Vec getXYfromIndex(int _index) {
        return Vec(_index % 4, (int)(_index / 4));
    }
};

struct StochSeqGrid : Module {
    enum BPMModes {
        BPM_CV,
        BPM_P2,
        BPM_P4,
        BPM_P8,
        BPM_P12,
        BPM_P24,
        NUM_BPM_MODES
    };
    enum ModeIds {
        GATE_MODE,
        TRIG_MODE,
        NUM_MODES
    };
    enum ParamIds {
        CLOCK_TOGGLE_PARAM,
        BPM_PARAM,
        LENGTH_PARAMS,
        PATHS_PARAM = LENGTH_PARAMS + NUM_SEQ,
        RHYTHM_PARAMS = PATHS_PARAM + NUM_SEQ,
        DUR_PARAMS = RHYTHM_PARAMS + NUM_SEQ,
        CELL_PROB_PARAM = DUR_PARAMS + NUM_SEQ,
        SUBDIVISION_PARAM = CELL_PROB_PARAM + NUM_OF_CELLS,
        RESET_PARAM = SUBDIVISION_PARAM + NUM_OF_CELLS,
        ON_PARAMS,
        PATTERN_PARAM = ON_PARAMS + NUM_SEQ,
        NUM_PARAMS
    };
    enum InputIds {
        RANDOM_INPUT,
        DIMINUTION_INPUT,
        EXT_CLOCK_INPUT,
        RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        GATES_OUTPUT,
        VOLTS_OUTPUT = GATES_OUTPUT + NUM_SEQ,
        NUM_OUTPUTS = VOLTS_OUTPUT + NUM_SEQ
    };
    enum LightIds {
        BANG_LIGHTS,
        TOGGLE_LIGHT = BANG_LIGHTS + 4,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger toggleTrig;
    dsp::SchmittTrigger resetTrig;
    dsp::SchmittTrigger bpmInputTrig;
    dsp::PulseGenerator gatePulse;

    int bpmInputMode = BPM_CV;
    int ppqn = 0;
    float period = 0.0;
    int timeOut = 1; // seconds
    int extPulseIndex = 0;
    int gateMode = GATE_MODE;
    int currentCellX = -1;
    int currentCellY = -1;
    bool clockOn = false;
    float clockFreq = 2.0; // Hz
    float globalPhase = 0.0;
    bool playCellRhythms = false;
    bool gateOn = false;
    int cellRhythmIndex = 0;
    int voltMode = 0;
    float minMaxVolts[2] = {0.0, 1.0};
    int currentPattern = 0;

    SeqCell *seqs = new SeqCell[NUM_SEQ];
    float *gateProbabilities = new float[NUM_OF_CELLS];
    float *rhythmProbabilities = new float[NUM_OF_CELLS];
    int *subdivisions = new int[NUM_OF_CELLS];
    bool beats[NUM_OF_CELLS][MAX_SUBDIVISIONS] = {};
    bool beatPulse[NUM_OF_CELLS][MAX_SUBDIVISIONS] = {};
    bool isCtrlClick = false;
    bool resetMode = false;
    bool useMouseDeltaY = false;
    bool displayCircles = false;

    StochSeqGrid() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configButton(CLOCK_TOGGLE_PARAM, "Run");
        configParam(BPM_PARAM, -2.0, 4.0, 1.0, "Tempo", " bpm", 2.0, 60.0);
        configButton(RESET_PARAM, "Reset");
        configParam(PATTERN_PARAM, 1.0, 16.0, 1.0, "Subdivisions");
        configParam(LENGTH_PARAMS + PURPLE_SEQ, 1, 16, 4, "Purple seq length");
        configParam(LENGTH_PARAMS + BLUE_SEQ, 1, 16, 4, "Blue seq length");
        configParam(LENGTH_PARAMS + AQUA_SEQ, 1, 16, 4, "Aqua seq length");
        configParam(LENGTH_PARAMS + RED_SEQ, 1, 16, 4, "Red seq length");
        configSwitch(PATHS_PARAM + PURPLE_SEQ, 0, 2, 0, "Purple path", {"default", "random", "random walk"});
        configSwitch(PATHS_PARAM + BLUE_SEQ, 0, 2, 0, "Blue path", {"default", "random", "random walk"});
        configSwitch(PATHS_PARAM + AQUA_SEQ, 0, 2, 0, "Aqua path", {"default", "random", "random walk"});
        configSwitch(PATHS_PARAM + RED_SEQ, 0, 2, 0, "Red path", {"default", "random", "random walk"});

        configParam(RHYTHM_PARAMS + PURPLE_SEQ, 1, 24, 1, "Purple rhythm");
        configParam(RHYTHM_PARAMS + BLUE_SEQ, 1, 24, 1, "Blue rhythm");
        configParam(RHYTHM_PARAMS + AQUA_SEQ, 1, 24, 1, "Aqua rhythm");
        configParam(RHYTHM_PARAMS + RED_SEQ, 1, 24, 1, "Red rhythm");
        configParam(DUR_PARAMS + PURPLE_SEQ, 1, 24, 1, "Purple duration");
        configParam(DUR_PARAMS + BLUE_SEQ, 1, 24, 1, "Blue duration");
        configParam(DUR_PARAMS + AQUA_SEQ, 1, 24, 1, "Aqua duration");
        configParam(DUR_PARAMS + RED_SEQ, 1, 24, 1, "Red duration");

        configSwitch(ON_PARAMS + PURPLE_SEQ, 0, 1, 1, "Purple", {"off", "on"});
        configSwitch(ON_PARAMS + BLUE_SEQ, 0, 1, 1, "Blue", {"off", "on"});
        configSwitch(ON_PARAMS + AQUA_SEQ, 0, 1, 1, "Aqua", {"off", "on"});
        configSwitch(ON_PARAMS + RED_SEQ, 0, 1, 1, "Red", {"off", "on"});

        configInput(EXT_CLOCK_INPUT, "External clock");
        configInput(RESET_INPUT, "Reset");

        configOutput(VOLTS_OUTPUT + PURPLE_SEQ, "Purple V/OCT");
        configOutput(VOLTS_OUTPUT + BLUE_SEQ, "Blue V/OCT");
        configOutput(VOLTS_OUTPUT + AQUA_SEQ, "Aqua V/OCT");
        configOutput(VOLTS_OUTPUT + RED_SEQ, "Red V/OCT");

        configOutput(GATES_OUTPUT + PURPLE_SEQ, "Purple Gates");
        configOutput(GATES_OUTPUT + BLUE_SEQ, "Blue Gates");
        configOutput(GATES_OUTPUT + AQUA_SEQ, "Aqua Gates");
        configOutput(GATES_OUTPUT + RED_SEQ, "Red Gates");

        seqs[0] = SeqCell(PURPLE_SEQ);
        seqs[1] = SeqCell(BLUE_SEQ);
        seqs[2] = SeqCell(AQUA_SEQ);
        seqs[3] = SeqCell(RED_SEQ);

        for (int i = 0; i < NUM_OF_CELLS; i++) {
            // cellOn[i] = true;
            configParam(CELL_PROB_PARAM + i, 0.0, 1.0, 1.0, "Cell Probability", "%", 0, 100);
            configParam(SUBDIVISION_PARAM + i, 0.0, 1.0, 1.0, "Rhythm Probability", "%", 0, 100);
            gateProbabilities[i] = params[CELL_PROB_PARAM].getValue();
            // gateProbabilities[i] = (float)i / NUM_OF_CELLS;
            rhythmProbabilities[i] = params[SUBDIVISION_PARAM].getValue();
            subdivisions[i] = 1;
            for (int j = 0; j < MAX_SUBDIVISIONS; j++) {
                beats[i][j] = true;
                beatPulse[i][j] = false;
            }
        }
    }

    ~StochSeqGrid() {
        delete[] gateProbabilities;
        delete[] rhythmProbabilities;
        delete[] subdivisions;
        delete[] seqs;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        json_t *subdivisionsJ = json_array();
        json_t *cellBeatsJ = json_array();

        for (int i = 0; i < NUM_OF_CELLS; i++) {
            json_t *subJ = json_integer(subdivisions[i]);
            json_array_append_new(subdivisionsJ, subJ);

            json_t *beatsJ = json_array();
            for (int j = 0; j < MAX_SUBDIVISIONS; j++) {
                json_t *beatJ = json_boolean(beats[i][j]);
                json_array_append_new(beatsJ, beatJ);
            }
            json_array_append_new(cellBeatsJ, beatsJ);
        }

        json_object_set_new(rootJ, "beats", cellBeatsJ);
        json_object_set_new(rootJ, "subdivisions", subdivisionsJ);
        json_object_set_new(rootJ, "gateMode", json_integer(gateMode));
        json_object_set_new(rootJ, "voltMode", json_integer(voltMode));
        json_object_set_new(rootJ, "currentPattern", json_integer(currentPattern));
        json_object_set_new(rootJ, "bpmInputMode", json_integer(bpmInputMode));
        json_object_set_new(rootJ, "run", json_boolean(clockOn));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *subdivisionsJ = json_object_get(rootJ, "subdivisions");
		if (subdivisionsJ) {
			for (int i = 0; i < NUM_OF_CELLS; i++) {
				json_t *subJ = json_array_get(subdivisionsJ, i);
				if (subJ)
					subdivisions[i] = json_integer_value(subJ);
			}
		}

        json_t *cellBeatsJ = json_object_get(rootJ, "beats");
        if (cellBeatsJ) {
            for (int i = 0; i < NUM_OF_CELLS; i++) {
                json_t *beatsJ = json_array_get(cellBeatsJ, i);

                for (int j = 0; j < MAX_SUBDIVISIONS; j++) {
                    json_t *beatJ = json_array_get(beatsJ, j);
                    if (beatJ) 
                        beats[i][j] = json_boolean_value(beatJ);
                }
            }
        }

        json_t *gateModeJ = json_object_get(rootJ, "gateMode");
        if (gateModeJ)
            gateMode = json_integer_value(gateModeJ);

        json_t *voltModeJ = json_object_get(rootJ, "voltMode");
        if (voltModeJ)
            voltMode = json_integer_value(voltModeJ);

        json_t *currentPatternJ = json_object_get(rootJ, "currentPattern");
        if (currentPatternJ)
            currentPattern = json_integer_value(currentPatternJ);

        json_t *bpmInputModeJ = json_object_get(rootJ, "bpmInputMode");
        if (bpmInputModeJ)
            bpmInputMode = json_integer_value(bpmInputModeJ);

        json_t *runJ = json_object_get(rootJ, "run");
        if (runJ) 
            clockOn = json_boolean_value(runJ);
    }

    void onReset() override {
        genPatterns(100); // randomize probabilities
    }

    Vec getXYfromIndex(int _index) {
        int x = _index % 4;
        int y = _index / 4;
        return Vec(x, y);
    }

    void genPatterns(float patt) {
        if (patt <= 16) {
            float prob = fmod(patt, 1);
            int c = (int)patt;
            for (int i = 0; i < NUM_OF_CELLS; i++) {
                int sub = random::uniform() < prob ? c + 1 : c;
                sub = clamp(sub, 1, 16);
                subdivisions[i] = sub;
            }
        } else {
            for (int i = 0; i < NUM_OF_CELLS; i++) {
                subdivisions[i] = static_cast<int>(random::uniform() * NUM_OF_CELLS);
            }
        }


        // switch (c) {
        //     case 0: // duples
        //         for (int i = 0; i < NUM_OF_CELLS; i++) {
        //             subdivisions[i] = (int)pow(2, randRange(3));
        //         }
        //         break;
        //     case 1: // triples
        //         for (int i = 0; i < NUM_OF_CELLS; i++) {
        //             int r = randRange(3);
        //             if (r == 0)
        //                 subdivisions[i] = 1;
        //             else if (r == 1)
        //                 subdivisions[i] = 3;
        //             else
        //                 subdivisions[i] = 6;
        //         }
        //         break;
        //     case 2:
        //         for (int i = 0; i < NUM_OF_CELLS; i++) {
        //             int r = randRange(3);
        //             if (r == 0)
        //                 subdivisions[i] = 1;
        //             else if (r == 1)
        //                 subdivisions[i] = 5;
        //             else
        //                 subdivisions[i] = 10;
        //         }
        //         break;
        //     case 3:
        //         for (int i = 0; i < NUM_OF_CELLS; i++) {
        //             gateProbabilities[i] = (i % 4 == 0) ? 1.0 : 0.0;
        //         }
        //         break;
        //     default:
        //         for (int i = 0; i < NUM_OF_CELLS; i++) {
        //             subdivisions[i] = static_cast<int>(random::uniform() * NUM_OF_CELLS);
        //         }
        //         break;
        // }
    }

    void resetSeqs() {
        for (int i = 0; i < NUM_SEQ; i++) {
            seqs[i].reset();
        }
    }

    void resetRhythms(int _index) {
        for (int i = 0; i < MAX_SUBDIVISIONS; i++) {
            beats[_index][i] = true;
        }
    }

    int getCurrentSubdivision() {
        int _index = clamp(currentCellX, 0, 3) + clamp(currentCellY, 0, 3) * 4;
        return subdivisions[_index];
    }

    int getSubdivision(int cellX, int cellY) {
        int _index = clamp(cellX, 0, 3) + clamp(cellY, 0, 3) * 4;
        return subdivisions[_index];
    }

    int getCurrentCellIndex() {
        return clamp(currentCellX, 0, 3) + clamp(currentCellY, 0, 3) * 4;
    }

    void setMinMaxVolts() {
        switch (voltMode) {
            case 0:
                minMaxVolts[0] = 0.0;
                minMaxVolts[1] = 1.0;
                break;
            case 1:
                minMaxVolts[0] = 0.0;
                minMaxVolts[1] = 2.0;
                break;
            case 2:
                minMaxVolts[0] = -5.0;
                minMaxVolts[1] = 5.0;
                break;
            case 3:
                minMaxVolts[0] = 0.0;
                minMaxVolts[1] = 10.0;
                break;
        }
    }

    void process(const ProcessArgs &args) override {
        if (resetTrig.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
            resetMode = true;
        }

        if (toggleTrig.process(params[CLOCK_TOGGLE_PARAM].getValue())) {
            clockOn ^= true;
        }

		if (params[PATTERN_PARAM].getValue() != currentPattern) {
			currentPattern = (int)params[PATTERN_PARAM].getValue();
            // int patt = (int)params[PATTERN_PARAM].getValue();
            for (int i = 0; i < NUM_OF_CELLS; i++) {
                subdivisions[i] = currentPattern;
            }
			// genPatterns(currentPattern);
		}
        
        for (int i = 0; i < NUM_SEQ; i++) {
            seqs[i].isOn = params[ON_PARAMS + i].getValue();
            seqs[i].rhythm = params[RHYTHM_PARAMS + i].getValue();
            seqs[i].duration = params[DUR_PARAMS + i].getValue();
            seqs[i].currentPath = (PathIds)params[PATHS_PARAM + i].getValue();
        }

        bool bpmDetect = false;
        if (inputs[EXT_CLOCK_INPUT].isConnected()) {
            if (bpmInputMode == BPM_CV) {
                clockFreq = 2.0 * std::pow(2.0, inputs[EXT_CLOCK_INPUT].getVoltage());
            } else {
                bpmDetect = bpmInputTrig.process(inputs[EXT_CLOCK_INPUT].getVoltage());
                if (bpmDetect)
                    clockOn = true;
                switch(bpmInputMode) {
                    case BPM_P2: 
                        ppqn = 2; 
                        break;
                    case BPM_P4: 
                        ppqn = 4;
                        break;
                    case BPM_P8:
                        ppqn = 8;
                        break;
                    case BPM_P12:
                        ppqn = 12;
                        break;
                    case BPM_P24:
                        ppqn = 24;
                        break;
                }
            }
        } else {
            float bpmParam = params[BPM_PARAM].getValue();
            clockFreq = std::pow(2.0, bpmParam);
        }

        if (clockOn) {
            if (bpmInputMode != BPM_CV && inputs[EXT_CLOCK_INPUT].isConnected()) {
                period += args.sampleTime;
                if (period > timeOut) clockOn = false;
                if (bpmDetect) {
                    if (extPulseIndex > 1) {
                        clockFreq = (1.0 / period) / (float)ppqn;
                    }

                    extPulseIndex++;
                    if (extPulseIndex >= ppqn) extPulseIndex = 0;
                    period = 0.0;
                }
            }

            setMinMaxVolts();

            
            // float bpmParam = params[BPM_PARAM].getValue();
            // clockFreq = std::pow(2.0, bpmParam);

            if (resetMode) {
                resetMode = false;
                resetSeqs();
            }

            for (int i = 0; i < NUM_SEQ; i++) {
                seqs[i].length = params[LENGTH_PARAMS + i].getValue();

                if (seqs[i].isOn) {
                    float rhythmFraction = seqs[i].rhythm / seqs[i].duration;
                    seqs[i].phase += clockFreq * rhythmFraction * args.sampleTime;
                    seqs[i].subPhase += clockFreq * rhythmFraction * getSubdivision(seqs[i].currentCellX, seqs[i].currentCellY) * args.sampleTime;

                    seqs[i].clockGate = (seqs[i].subPhase < 0.5);

                    if (seqs[i].phase >= 1.0) {
                        seqs[i].phase -= 1.0;
                        seqs[i].subPhase = 0.0;
                        seqs[i].playCellRhythms = false;

                        // TODO: reset here?
                        // if (resetMode) {
                        //     resetMode = false;
                        //     resetSeqs();
                        // }

                        seqs[i].clockStep();

                        int _index = seqs[i].getCurrentCellIndex();


                        float gateProb = params[CELL_PROB_PARAM + _index].getValue();
                        float rhythmProb = params[SUBDIVISION_PARAM + _index].getValue();
                        if (random::uniform() < gateProb) {
                            seqs[i].volts = rescale(rhythmProb, 0.0, 1.0, minMaxVolts[0], minMaxVolts[1]);

                            if (subdivisions[_index] == 1) { // if 1 subdivision then don't check rhythm probability
                                seqs[i].gatePulse.trigger(1e-3);
                                seqs[i].gateOn = true;
                            } else if (random::uniform() < rhythmProb) {
                                seqs[i].playCellRhythms = true;
                                if (beats[_index][seqs[i].cellRhythmIndex]) {
                                    seqs[i].gatePulse.trigger(1e-3);
                                    seqs[i].gateOn = true;
                                } else {
                                    seqs[i].gateOn = false;
                                }
                            } else {
                                seqs[i].gatePulse.trigger(1e-3);
                                seqs[i].gateOn = true;
                            }
                        } else {
                            seqs[i].gateOn = false;
                        }
                    }

                    if (seqs[i].subPhase >= 1.0) {
                        seqs[i].subPhase -= 1.0;

                        if (subdivisions[seqs[i].getCurrentCellIndex()] > 1) seqs[i].cellRhythmIndex++;
                        // clockGate = (phase < 0.5);

                        if (seqs[i].playCellRhythms && beats[seqs[i].getCurrentCellIndex()][seqs[i].cellRhythmIndex]) {
                            seqs[i].gatePulse.trigger(1e-3);
                            seqs[i].gateOn = true;
                        } else {
                            seqs[i].gateOn = false;
                        }
                    }

                } else {
                    if (seqs[i].phase >= 1.0) {
                        seqs[i].phase = 0.0;
                        seqs[i].subPhase = 0.0;
                        seqs[i].playCellRhythms = false;

                        seqs[i].clockStep();
                    }
                }

                seqs[i].setBeatPulse();

                // seqs[i].beatPulse[seqs[i].getCurrentCellIndex()][seqs[i].cellRhythmIndex] = seqs[i].gateOn && seqs[i].clockGate;

                bool gateVolt = false;
                if (gateMode == GATE_MODE) {
                    gateVolt = seqs[i].gateOn && seqs[i].clockGate;
                } else {
                    gateVolt = seqs[i].gatePulse.process(args.sampleTime);
                }

                outputs[GATES_OUTPUT + i].setVoltage(gateVolt ? 10.0 : 0.0);
                outputs[VOLTS_OUTPUT + i].setVoltage(seqs[i].volts);
            }

            // keep track of global phase for synchronization
            globalPhase += clockFreq * args.sampleTime;
            if (globalPhase >= 1.0) {
                globalPhase = 0.0;
            }

        } else {
            globalPhase = 0.0;
            for (int i = 0; i < NUM_SEQ; i++) {
                seqs[i].phase = 0.0;
                seqs[i].subPhase = 0.0;
            }
        }

        // bool gateVolt = false;
        // if (gateMode == GATE_MODE) {
        //     gateVolt = gateOn && clockGate;
        // } else {
        //     gateVolt = gatePulse.process(args.sampleTime);
        // }

        // for (int i = 0; i < NUM_SEQ; i++) {
        //     outputs[GATES_OUTPUT + i].setVoltage(gateVolt ? 10.0 : 0.0);
        // }

        lights[TOGGLE_LIGHT].setBrightness(clockOn ? 1.0 : 0.0);
    }
};

struct CellOverlay : Widget {
    StochSeqGrid *module;

    void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                int index = x + y * 4;
                float alpha = rescale(module->getParam(StochSeqGrid::CELL_PROB_PARAM + index).getValue(), 0.0, 1.0, 175, 0);
                nvgStrokeColor(args.vg, nvgRGB(60, 60, 73));
                nvgFillColor(args.vg, nvgRGBA(0, 0, 0, alpha));
                nvgBeginPath(args.vg);
                float xPos = x * CELL_SIZE;
                float yPos = y * CELL_SIZE;
                nvgRect(args.vg, xPos, yPos, CELL_SIZE, CELL_SIZE);
                nvgFill(args.vg);
            }
        }
    }
};

struct BGGrid : Widget {

    void draw(const DrawArgs &args) override {

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                nvgStrokeColor(args.vg, nvgRGB(60, 60, 73));
                nvgBeginPath(args.vg);
                float xPos = x * CELL_SIZE;
                float yPos = y * CELL_SIZE;
                nvgRect(args.vg, xPos, yPos, CELL_SIZE, CELL_SIZE);
                nvgStroke(args.vg);
            }
        }
    }  
};

struct SubdivisionDisplay : Widget {
    Vec positions[16] = {};
    bool isBeatOn = false;
    bool clickedOnBeat = false;
    float circleRad;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;
    int index;
    StochSeqGrid *module;

    SubdivisionDisplay() {}

    void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if ((e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
                module->isCtrlClick = true;
                e.consume(this);
                initX = e.pos.x;
                initY = e.pos.y;
                isBeatOn = !isSubdivisionOn(initX, initY);
                clickedOnBeat = false;
                toggleRhythms(initX, initY, isBeatOn);
                if (!clickedOnBeat)
                    module->resetRhythms(index);
            } else if ((e.mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
                module->isCtrlClick = false;
                e.consume(this);
                // module->resetRhythms(index);
                int rhythms = module->subdivisions[index];
                rhythms *= 2;
                if (rhythms <= MAX_SUBDIVISIONS)
                    module->subdivisions[index] = rhythms;
            } else {
                module->isCtrlClick = false;
                e.consume(this);
                incrementSubdivisions();
            }
        }
    }

    void onDragStart(const event::DragStart &e) override {
        dragX = APP->scene->rack->getMousePos().x;
        dragY = APP->scene->rack->getMousePos().y;
    }

    void onDragMove(const event::DragMove &e) override {
        float delta;
        if (module->useMouseDeltaY)
            delta = -e.mouseDelta.y;
        else
            delta = e.mouseDelta.x;
        float newDragX = APP->scene->rack->getMousePos().x;
        float newDragY = APP->scene->rack->getMousePos().y;
        if (module->isCtrlClick) {
            toggleRhythms(initX + (newDragX - dragX), initY + (newDragY - dragY), isBeatOn);
        } else {
            incrementSubdivisions(delta);
        }
    }

    void incrementSubdivisions() {
        module->subdivisions[index] = clamp(++module->subdivisions[index], 1, MAX_SUBDIVISIONS);
    }

    void incrementSubdivisions(float dy) {
        int sd = static_cast<int>(round(module->subdivisions[index] + dy * 0.25));
        module->subdivisions[index] = clamp(sd, 1, MAX_SUBDIVISIONS);
    }

    void toggleRhythms(float currentX, float currentY, bool on) {
        Vec mouse = Vec(currentX, currentY);
        int subRhythms = module->subdivisions[index];
        for (int i = 0; i < subRhythms; i++) {
            float d = dist(mouse, positions[i]);
            if (d < circleRad) {
                module->beats[index][i] = on;
                clickedOnBeat = true;
            }
        }
    }

    bool isSubdivisionOn(float x, float y) {
        Vec mouse = Vec(x, y);
        int subRhythms = module->subdivisions[index];
        for (int i = 0; i < subRhythms; i++) {
            float d = dist(mouse, positions[i]);
            if (d < circleRad) {
                return module->beats[index][i];
            }
        }
        return false;
    }

    void draw(const DrawArgs &args) override {
        if (module == NULL) {
            // TODO: draw stuff for preview 
            return;
        }
        
        float radius = 22.0;
        Vec center = Vec(box.size.x / 2, box.size.y / 2);
        int subRhythms = module->subdivisions[index];

        // if only one beat
        if (subRhythms == 1) {
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, center.x, center.y, 35.0/2);
            nvgFill(args.vg);

            for (int i = 0; i < NUM_SEQ; i++) {
                // visual pulse stuff
                if (module->seqs[i].beatPulse[index][0])  {
                    nvgBeginPath(args.vg);
                    nvgCircle(args.vg, center.x, center.y, radius);
                    nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
                    nvgStroke(args.vg);
                }
            }
        } else {
            circleRad = rescale(subRhythms, 2.0, MAX_SUBDIVISIONS, 16.0 / 2, 8.0 / 2);

            if (module->displayCircles) {
                nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
                nvgStrokeWidth(args.vg, 1.0);
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, center.x, center.y, radius);
                nvgStroke(args.vg);
            }

            for (int i = 0; i < subRhythms; i++) {
                bool beatOn = module->beats[index][i];

                float angle = rescale((float)i, 0.0, subRhythms, -M_PI/2, M_PI*2.0 - M_PI/2);
                float x = cos(angle) * radius;
                float y = sin(angle) * radius;
                Vec pos = Vec(x, y).plus(center);
                positions[i] = pos;

                // connected lines
                if (beatOn && !module->displayCircles) {
                    float alpha = rescale(module->getParam(StochSeqGrid::SUBDIVISION_PARAM + index).getValue(), 0.0, 1.0, 25, 200);
                    nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, alpha));
                    nvgStrokeWidth(args.vg, i == 0 ? 2.5 : 1.0);
                    nvgBeginPath(args.vg);
                    nvgMoveTo(args.vg, center.x, center.y);
                    nvgLineTo(args.vg, pos.x, pos.y);
                    nvgStroke(args.vg);
                }

                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, circleRad);
                if (beatOn) {
                    nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                } else {
                    nvgFillColor(args.vg, nvgRGB(51, 51, 51));
                    nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
                    nvgStrokeWidth(args.vg, 2.0);
                    nvgStroke(args.vg);
                }
                nvgFill(args.vg);

                for (int j = 0; j < NUM_SEQ; j++) {
                    // visual pulse stuff
                    if (module->seqs[j].beatPulse[index][i] && beatOn)  {
                        nvgBeginPath(args.vg);
                        nvgCircle(args.vg, pos.x, pos.y, circleRad * 1.2);
                        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 200));
                        nvgFill(args.vg);
                    }

                }
            }
        }
    }
};

struct CellsDisplay : Widget {
    StochSeqGrid *module;

    CellsDisplay() {}

    // void draw(const DrawArgs &args) override {
    //     if (module == NULL) return;

    //     for (int i = 0; i < NUM_SEQ; i++) {            
    //         if (module->seqs[i].isOn) {
    //             int xPos = clamp(module->seqs[i].currentCellX, 0, 3);
    //             int yPos = clamp(module->seqs[i].currentCellY, 0, 3);
    //             nvgStrokeColor(args.vg, module->seqs[i].color);
    //             nvgFillColor(args.vg, nvgTransRGBA(module->seqs[i].color, 50));
    //             nvgBeginPath(args.vg);
    //             nvgRect(args.vg, xPos * CELL_SIZE, yPos * CELL_SIZE, CELL_SIZE, CELL_SIZE);
    //             nvgFill(args.vg);
    //             nvgStrokeWidth(args.vg, 2.0);
    //             nvgStroke(args.vg);
    //         }
    //     }
    // }

    void drawLayer(const DrawArgs &args, int layer) override {
        if (module == NULL) return;

        if (layer == 1) {
            for (int i = 0; i < NUM_SEQ; i++) {            
                    if (module->seqs[i].isOn) {
                        int xPos = clamp(module->seqs[i].currentCellX, 0, 3);
                        int yPos = clamp(module->seqs[i].currentCellY, 0, 3);
                        nvgStrokeColor(args.vg, module->seqs[i].color);
                        nvgFillColor(args.vg, nvgTransRGBA(module->seqs[i].color, 35));
                        nvgBeginPath(args.vg);
                        nvgRect(args.vg, xPos * CELL_SIZE, yPos * CELL_SIZE, CELL_SIZE, CELL_SIZE);
                        nvgFill(args.vg);
                        nvgStrokeWidth(args.vg, 2.0);
                        nvgStroke(args.vg);
                    }
                }
        }
        Widget::drawLayer(args, layer);
    }
};

struct RatioDisplayLabel : Widget {
    std::string text;
    int id;
	int fontSize;
    StochSeqGrid *module;

	RatioDisplayLabel(int _fontSize = 13) {
		fontSize = _fontSize;
		box.size.y = BND_WIDGET_HEIGHT;
	}

	void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        int num = (int)module->seqs[id].rhythm;
        int dur = (int)module->seqs[id].duration;

        text = std::to_string(num) + ":" + std::to_string(dur);
        float xPos1 = num < 10 ? 7.6 : 0.0;
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT + NVG_ALIGN_TOP);
        // nvgTextAlign(args.vg, NVG_ALIGN_CENTER + NVG_ALIGN_TOP);
        // nvgFillColor(args.vg, nvgRGB(38, 0, 255));
        nvgFillColor(args.vg, nvgRGB(0, 0, 0));
        nvgFontSize(args.vg, fontSize);
        nvgText(args.vg, xPos1, 0, text.c_str(), NULL);
    }
};

struct StochSeqGridWidget : ModuleWidget {
    StochSeqGridWidget(StochSeqGrid *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/StochSeqGrid.svg")));

        BGGrid *gridDisplay = new BGGrid();
        gridDisplay->box.pos = Vec(82.5, 54.8);
        gridDisplay->box.size = Vec(270, 270);
        addChild(gridDisplay);

        CellsDisplay *cells = new CellsDisplay();
        cells->module = module;
        cells->box.pos = Vec(82.5, 54.8);
        cells->box.size = Vec(270, 270);
        addChild(cells);

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                int _index = x + y * 4;
                float xPos = 82.5 + x * CELL_SIZE;
                float yPos = 54.8 + y * CELL_SIZE;

                SubdivisionDisplay *subdivision = new SubdivisionDisplay();
                subdivision->module = module;
                subdivision->index = _index;
                subdivision->box.pos = Vec(xPos, yPos);
                subdivision->box.size = Vec(CELL_SIZE, CELL_SIZE);
                addChild(subdivision);
            }
        }

        CellOverlay *cellOverlay = new CellOverlay();
        cellOverlay->module = module;
        cellOverlay->box.pos = Vec(82.5, 54.8);
        cellOverlay->box.size = Vec(270, 270);
        addChild(cellOverlay);

        for (int i = 0; i < NUM_SEQ; i++) {
            RatioDisplayLabel *ratioLabel = new RatioDisplayLabel();
            ratioLabel->module = module;
            ratioLabel->box.pos = Vec(26.2, 156.8 + i * 50);
            ratioLabel->box.size.x = 30; // 10
            ratioLabel->id = PURPLE_SEQ + i;
            addChild(ratioLabel);
        }

        addChild(createWidget<JeremyScrew>(Vec(33, 2)));
        addChild(createWidget<JeremyScrew>(Vec(33, box.size.y - 14)));
        addChild(createWidget<JeremyScrew>(Vec(315, 2)));
        addChild(createWidget<JeremyScrew>(Vec(315, box.size.y - 14)));

        addInput(createInputCentered<TinyPJ301M>(Vec(28.3, 116.1), module, StochSeqGrid::RESET_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(52.4, 116.1), module, StochSeqGrid::EXT_CLOCK_INPUT));

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                int index = x + y * 4;
                addParam(createParamCentered<TinyWhiteKnob>(Vec(116.3 + (x * CELL_SIZE), 88.5 + (y * CELL_SIZE)), module, StochSeqGrid::SUBDIVISION_PARAM + index));
                // addParam(createParamCentered<NanoWhiteKnob>(Vec(116.3 + (x * CELL_SIZE), 88.5 + (y * CELL_SIZE)), module, StochSeqGrid::SUBDIVISION_PARAM + index));
                addParam(createParamCentered<NanoWhiteKnob>(Vec(89.4 + (x * CELL_SIZE), 61.7 + (y * CELL_SIZE)), module, StochSeqGrid::CELL_PROB_PARAM + index));
            }
        }

        addParam(createParamCentered<TinyBlueButton>(Vec(28.3, 79), module, StochSeqGrid::CLOCK_TOGGLE_PARAM));
        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(28.3 - 3, 79 - 3), module, StochSeqGrid::TOGGLE_LIGHT));
        addParam(createParamCentered<BlueKnob>(Vec(52.4, 79), module, StochSeqGrid::BPM_PARAM));
        addParam(createParamCentered<TinyBlueButton>(Vec(28.3, 97.3), module, StochSeqGrid::RESET_PARAM));
        addParam(createParamCentered<BlueInvertKnob>(Vec(282.9, 29.8), module, StochSeqGrid::PATTERN_PARAM));

        // lengths
        addParam(createParamCentered<PurpleInvertKnob>(Vec(97.1, 29.8), module, StochSeqGrid::LENGTH_PARAMS + PURPLE_SEQ));
        addParam(createParamCentered<BlueInvertKnob>(Vec(140.1, 29.8), module, StochSeqGrid::LENGTH_PARAMS + BLUE_SEQ));
        addParam(createParamCentered<AquaInvertKnob>(Vec(183.1, 29.8), module, StochSeqGrid::LENGTH_PARAMS + AQUA_SEQ));
        addParam(createParamCentered<RedInvertKnob>(Vec(226.1, 29.8), module, StochSeqGrid::LENGTH_PARAMS + RED_SEQ));
        // paths
        addParam(createParamCentered<Purple_VSwitch>(Vec(118.6, 29.8), module, StochSeqGrid::PATHS_PARAM + PURPLE_SEQ));
        addParam(createParamCentered<Blue_VSwitch>(Vec(161.6, 29.8), module, StochSeqGrid::PATHS_PARAM + BLUE_SEQ));
        addParam(createParamCentered<Aqua_VSwitch>(Vec(204.6, 29.8), module, StochSeqGrid::PATHS_PARAM + AQUA_SEQ));
        addParam(createParamCentered<Red_VSwitch>(Vec(247.6, 29.8), module, StochSeqGrid::PATHS_PARAM + RED_SEQ));

        // purple
        addParam(createParamCentered<PurpleInvertKnob>(Vec(28.3, 180.3), module, StochSeqGrid::RHYTHM_PARAMS + PURPLE_SEQ));
        addParam(createParamCentered<PurpleInvertKnob>(Vec(54.2, 180.3), module, StochSeqGrid::DUR_PARAMS + PURPLE_SEQ));
        addParam(createParamCentered<NanoPurpleButton>(Vec(41.3, 194.8), module, StochSeqGrid::ON_PARAMS + PURPLE_SEQ));
        // blue
        addParam(createParamCentered<BlueInvertKnob>(Vec(28.3, 230.3), module, StochSeqGrid::RHYTHM_PARAMS + BLUE_SEQ));
        addParam(createParamCentered<BlueInvertKnob>(Vec(54.2, 230.3), module, StochSeqGrid::DUR_PARAMS + BLUE_SEQ));
        addParam(createParamCentered<NanoBlueButton>(Vec(41.3, 244.8), module, StochSeqGrid::ON_PARAMS + BLUE_SEQ));
        // aqua
        addParam(createParamCentered<AquaInvertKnob>(Vec(28.3, 280.3), module, StochSeqGrid::RHYTHM_PARAMS + AQUA_SEQ));
        addParam(createParamCentered<AquaInvertKnob>(Vec(54.2, 280.3), module, StochSeqGrid::DUR_PARAMS + AQUA_SEQ));
        addParam(createParamCentered<NanoAquaButton>(Vec(41.3, 294.8), module, StochSeqGrid::ON_PARAMS + AQUA_SEQ));
        // red
        addParam(createParamCentered<RedInvertKnob>(Vec(28.3, 330.3), module, StochSeqGrid::RHYTHM_PARAMS + RED_SEQ));
        addParam(createParamCentered<RedInvertKnob>(Vec(54.2, 330.3), module, StochSeqGrid::DUR_PARAMS + RED_SEQ));
        addParam(createParamCentered<NanoRedButton>(Vec(41.3, 344.8), module, StochSeqGrid::ON_PARAMS + RED_SEQ));

        // v/oct outputs
        addOutput(createOutputCentered<PJ301MPurple>(Vec(98, 347.6), module, StochSeqGrid::VOLTS_OUTPUT + PURPLE_SEQ));
        addOutput(createOutputCentered<PJ301MBlue>(Vec(125, 347.6), module, StochSeqGrid::VOLTS_OUTPUT + BLUE_SEQ));
        addOutput(createOutputCentered<PJ301MAqua>(Vec(152, 347.6), module, StochSeqGrid::VOLTS_OUTPUT + AQUA_SEQ));
        addOutput(createOutputCentered<PJ301MRed>(Vec(179, 347.6), module, StochSeqGrid::VOLTS_OUTPUT + RED_SEQ));
        // gates outputs
        addOutput(createOutputCentered<PJ301MPurple>(Vec(228.9, 347.6), module, StochSeqGrid::GATES_OUTPUT + PURPLE_SEQ));
        addOutput(createOutputCentered<PJ301MBlue>(Vec(255.9, 347.6), module, StochSeqGrid::GATES_OUTPUT + BLUE_SEQ));
        addOutput(createOutputCentered<PJ301MAqua>(Vec(282.9, 347.6), module, StochSeqGrid::GATES_OUTPUT + AQUA_SEQ));
        addOutput(createOutputCentered<PJ301MRed>(Vec(309.9, 347.6), module, StochSeqGrid::GATES_OUTPUT + RED_SEQ));
    }

    void appendContextMenu(Menu *menu) override {
        StochSeqGrid *module = dynamic_cast<StochSeqGrid *>(this->module);

        menu->addChild(new MenuSeparator);

        menu->addChild(createIndexPtrSubmenuItem("Gate mMde", {"gates", "triggers"}, &module->gateMode));
        menu->addChild(createIndexPtrSubmenuItem("Volt Range", {"0V 1V", "0V 2V", "-5V +5V", "0V 10V"}, &module->voltMode));
        menu->addChild(createIndexPtrSubmenuItem("Mouse Drag", {"horizontal", "vertical"}, &module->useMouseDeltaY));

        menu->addChild(new MenuEntry);
        
        menu->addChild(createIndexPtrSubmenuItem("External Clock Mode", {"CV (0V = 120 bpm)", "2 PPQN", "4 PPQN", "8 PPQN", "12 PPQN", "24 PPQN"}, &module->bpmInputMode));

        menu->addChild(new MenuEntry);

        menu->addChild(createIndexPtrSubmenuItem("Display", {"blooms", "circles"}, &module->displayCircles));
    }
};

Model *modelStochSeqGrid = createModel<StochSeqGrid, StochSeqGridWidget>("StochSeqGrid");
