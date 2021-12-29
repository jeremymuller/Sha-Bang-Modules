#include "plugin.hpp"

#define SLIDER_TOP 4
#define NUM_OF_CELLS 16
#define CELL_SIZE 67.5
#define MARGIN 1

struct StochSeqGrid : Module {
    enum SequencerIds {
        PURPLE_SEQ,
        BLUE_SEQ,
        AQUA_SEQ,
        RED_SEQ,
        NUM_SEQ
    };
    enum ModeIds {
        GATE_MODE,
        TRIG_MODE,
        NUM_MODES
    };
    enum ParamIds {
        CLOCK_TOGGLE_PARAM,
        BPM_PARAM,
        CELL_PROB_PARAM,
        SUBDIVISION_PARAM = CELL_PROB_PARAM + NUM_OF_CELLS,
        RESET_PARAM = SUBDIVISION_PARAM + NUM_OF_CELLS,
        INVERT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        RANDOM_INPUT,
        INVERT_INPUT,
        DIMINUTION_INPUT,
        EXT_CLOCK_INPUT,
        RESET_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        GATES_OUTPUT,
        VOLT_OUTPUT = GATES_OUTPUT + NUM_SEQ,
        NUM_OUTPUTS
    };
    enum LightIds {
        BANG_LIGHTS,
        TOGGLE_LIGHT = BANG_LIGHTS + 4,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger toggleTrig;
    dsp::SchmittTrigger resetTrig;
    dsp::SchmittTrigger invertTrig;
    dsp::PulseGenerator gatePulse;

    int gateMode = GATE_MODE;
    int currentCellX = -1;
    int currentCellY = -1;
    bool clockOn = false;
    float currentBPM = 120.0;
    float clockFreq = 2.0; // Hz
    float phase = 0.0;
    float subPhase = 0.0;
    bool playCellRhythms = false;
    bool gateOn = false;
    int cellRhythmIndex = 0;

    float *gateProbabilities = new float[NUM_OF_CELLS];
    float *rhythmProbabilities = new float[NUM_OF_CELLS];
    int *subdivisions = new int[NUM_OF_CELLS];
    bool beats[NUM_OF_CELLS][12] = {};
    bool isCtrlClick = false;
    bool resetMode = false;

    StochSeqGrid() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configButton(CLOCK_TOGGLE_PARAM, "Run");
        configParam(BPM_PARAM, -2.0, 4.0, 1.0, "Tempo", " bpm", 2.0, 60.0);
        configButton(RESET_PARAM, "Reset");
        configButton(INVERT_PARAM, "Invert pattern");

        configInput(EXT_CLOCK_INPUT, "External clock");
        configInput(RESET_INPUT, "Reset");
        configInput(INVERT_INPUT, "Invert pattern");

        configOutput(GATES_OUTPUT + PURPLE_SEQ, "Purple Gates");
        configOutput(GATES_OUTPUT + BLUE_SEQ, "Blue Gates");
        configOutput(GATES_OUTPUT + AQUA_SEQ, "Aqua Gates");
        configOutput(GATES_OUTPUT + RED_SEQ, "Red Gates");
        configOutput(VOLT_OUTPUT, "Pitch (V/OCT)");

        for (int i = 0; i < NUM_OF_CELLS; i++) {
            // cellOn[i] = true;
            configParam(CELL_PROB_PARAM + i, 0.0, 1.0, 1.0, "Cell Probability", "%", 0, 100);
            configParam(SUBDIVISION_PARAM + i, 0.0, 1.0, 1.0, "Rhythm Probability", "%", 0, 100);
            gateProbabilities[i] = params[CELL_PROB_PARAM].getValue();
            // gateProbabilities[i] = (float)i / NUM_OF_CELLS;
            rhythmProbabilities[i] = params[SUBDIVISION_PARAM].getValue();
            subdivisions[i] = 1;
            for (int j = 0; j < 12; j++) {
                beats[i][j] = true;
            }
        }

    }

    ~StochSeqGrid() {
        delete[] gateProbabilities;
        delete[] rhythmProbabilities;
        delete[] subdivisions;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        json_t *subdivisionsJ = json_array();
        for (int i = 0; i < NUM_OF_CELLS; i++) {
            json_t *subJ = json_integer(subdivisions[i]);
            json_array_append_new(subdivisionsJ, subJ);
        }

        json_object_set_new(rootJ, "subdivisions", subdivisionsJ);
        json_object_set_new(rootJ, "gateMode", json_integer(gateMode));

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

        json_t *gateModeJ = json_object_get(rootJ, "gateMode");
        if (gateModeJ)
            gateMode = json_integer_value(gateModeJ);
    }

    void onReset() override {
        genPatterns(100); // randomize probabilities
    }

    void genPatterns(int c) {
        switch (c) {
            case 0:
                for (int i = 0; i < NUM_OF_CELLS; i++) {
                    gateProbabilities[i] = 0.0;
                }
                break;
            case 1:
                for (int i = 0; i < NUM_OF_CELLS; i++) {
                    gateProbabilities[i] = 0.5;
                }
                break;
            case 2:
                for (int i = 0; i < NUM_OF_CELLS; i++) {
                    gateProbabilities[i] = 1.0;
                }
                break;
            case 3:
                for (int i = 0; i < NUM_OF_CELLS; i++) {
                    gateProbabilities[i] = (i % 4 == 0) ? 1.0 : 0.0;
                }
                break;
            default:
                for (int i = 0; i < NUM_OF_CELLS; i++) {
                    gateProbabilities[i] = random::uniform();
                    rhythmProbabilities[i] = random::uniform();
                    subdivisions[i] = 1;
                }
                break;
        }
    }

    void invert() {
        for (int i = 0; i < NUM_OF_CELLS; i++) {
            float currentProb = gateProbabilities[i];
            float currentRhythmProb = rhythmProbabilities[i];
            gateProbabilities[i] = 1.0 - currentProb;
            rhythmProbabilities[i] = 1.0 - currentRhythmProb;
        }
    }

    void clockStep() {

        currentCellX++;
        if (currentCellX >= 4) {
            currentCellX = 0;
            // currentCellY = (currentCellY + 1) % 4;
        }
        cellRhythmIndex = 0;
    }

    void resetSeq() {
        currentCellX = -1;
        currentCellY = -1;
    }

    int getCurrentSubdivision() {
        int _index = clamp(currentCellX, 0, 4) + clamp(currentCellY, 0, 4) * 4;
        return subdivisions[_index];
    }

    int getCurrentCellIndex() {
        return clamp(currentCellX, 0, 4) + clamp(currentCellY, 0, 4) * 4;
    }

    void process(const ProcessArgs &args) override {
        if (resetTrig.process(params[RESET_PARAM].getValue() + inputs[RESET_INPUT].getVoltage())) {
            resetMode = true;
        }

        if (toggleTrig.process(params[CLOCK_TOGGLE_PARAM].getValue())) {
            clockOn ^= true;
        }

        if (invertTrig.process(params[INVERT_PARAM].getValue() + inputs[INVERT_INPUT].getVoltage())) {
            invert();
        }

        bool clockGate = false;
        if (clockOn) {
            float bpmParam = params[BPM_PARAM].getValue();
            clockFreq = std::pow(2.0, bpmParam);

            currentBPM = clockFreq * 60;

            phase += clockFreq * args.sampleTime;
            subPhase += clockFreq * getCurrentSubdivision() * args.sampleTime;

            clockGate = (subPhase < 0.5);

            if (phase >= 1.0) {
                phase = 0.0;
                subPhase = 0.0;
                playCellRhythms = false;
                if (resetMode) {
                    resetMode = false;
                    resetSeq();
                }
                clockStep();

                // _index = clamp(currentCellX, 0, 3) + clamp(currentCellY, 0, 3) * 4;
                int _index = getCurrentCellIndex();

                float gateProb = params[CELL_PROB_PARAM + _index].getValue();
                float rhythmProb = params[SUBDIVISION_PARAM + _index].getValue();
                if (random::uniform() < gateProb) {
                    if (random::uniform() < rhythmProb) {
                        playCellRhythms = true;
                        if (beats[_index][cellRhythmIndex]) {
                            gatePulse.trigger(1e-3);
                            gateOn = true;
                        } else {
                            gateOn = false;
                        }
                    } else {
                        gatePulse.trigger(1e-3);
                        gateOn = true;
                    }
                } else {
                    gateOn = false;
                }
            }

            if (subPhase >= 1.0) {
                subPhase = 0.0;
                cellRhythmIndex++;
                // clockGate = (phase < 0.5);

                if (playCellRhythms && beats[getCurrentCellIndex()][cellRhythmIndex]) {
                    gatePulse.trigger(1e-3);
                    gateOn = true;
                } else {
                    gateOn = false;
                }
            }
        }

        bool gateVolt = false;
        if (gateMode == GATE_MODE) {
            gateVolt = gateOn && clockGate;
        } else {
            gateVolt = gatePulse.process(args.sampleTime);
        }

        for (int i = 0; i < NUM_SEQ; i++) {
            outputs[GATES_OUTPUT + i].setVoltage(gateVolt ? 10.0 : 0.0);
        }

        lights[TOGGLE_LIGHT].setBrightness(clockOn ? 1.0 : 0.0);
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
    float circleRad;
    float initX;
    float initY;
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
                toggleRhythms(initX, initY);
            } else {
                module->isCtrlClick = false;
                e.consume(this);
                module->subdivisions[index]++;
                if (module->subdivisions[index] > 12) module->subdivisions[index] = 1;
            }
        }
    }

    void toggleRhythms(float currentX, float currentY) {
        Vec mouse = Vec(currentX, currentY);
        int subRhythms = module->subdivisions[index];
        for (int i = 0; i < subRhythms; i++) {
            float d = dist(mouse, positions[i]);
            if (d < circleRad) {
                module->beats[index][i] ^= true;
            }
        }
    }

    void draw(const DrawArgs &args) override {
        if (module == NULL) {
            // TODO: draw stuff for preview 
            return;
        }

        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 200));
        nvgStrokeWidth(args.vg, 1.0);
        Vec center = Vec(box.size.x / 2, box.size.y / 2);
        float radius = 22.0;
        int subRhythms = module->subdivisions[index];
        circleRad = rescale(subRhythms, 2.0, 12.0, 16.0/2, 11.0/2);
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, center.x, center.y, radius);
        nvgStroke(args.vg);
        if (subRhythms < 2) {
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, center.x, center.y, 35.0/2);
            nvgFill(args.vg);
        } else {
            for (int i = 0; i < subRhythms; i++) {
                float angle = rescale((float)i, 0.0, subRhythms, -M_PI/2, M_PI*2.0 - M_PI/2);
                float x = cos(angle) * radius;
                float y = sin(angle) * radius;
                Vec pos = Vec(x, y).plus(center);
                positions[i] = pos;
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, circleRad);
                if (module->beats[index][i]) {
                    nvgFillColor(args.vg, nvgRGB(255, 255, 255));
                } else {
                    nvgFillColor(args.vg, nvgRGB(51, 51, 51));
                    nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
                    nvgStrokeWidth(args.vg, 2.0);
                    nvgStroke(args.vg);
                }
                nvgFill(args.vg);
            }
        }
    }
};

struct CellsDisplay : Widget {
    StochSeqGrid *module;

    CellsDisplay() {}

    void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        int xPos = clamp(module->currentCellX, 0, 4);
        int yPos = clamp(module->currentCellY, 0, 4);
        nvgStrokeColor(args.vg, getAqua());
        nvgBeginPath(args.vg);
        nvgRect(args.vg, xPos * CELL_SIZE, yPos * CELL_SIZE, CELL_SIZE, CELL_SIZE);
        nvgStrokeWidth(args.vg, 2.0);
        nvgStroke(args.vg);
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

        CellsDisplay *cells = new CellsDisplay();
        cells->module = module;
        cells->box.pos = Vec(82.5, 54.8);
        cells->box.size = Vec(270, 270);
        addChild(cells);

        addChild(createWidget<JeremyScrew>(Vec(33, 2)));
        addChild(createWidget<JeremyScrew>(Vec(33, box.size.y - 14)));
        addChild(createWidget<JeremyScrew>(Vec(315, 2)));
        addChild(createWidget<JeremyScrew>(Vec(315, box.size.y - 14)));

        addInput(createInputCentered<TinyPJ301M>(Vec(26.3, 116.1), module, StochSeqGrid::RESET_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(50.3, 116.1), module, StochSeqGrid::EXT_CLOCK_INPUT));

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                int index = x + y * 4;
                addParam(createParamCentered<TinyWhiteKnob>(Vec(116.3 + (x * CELL_SIZE), 88.5 + (y * CELL_SIZE)), module, StochSeqGrid::SUBDIVISION_PARAM + index));
                if (index < 4) {
                    addParam(createParamCentered<NanoPurpleKnob>(Vec(89.4 + (x * CELL_SIZE), 61.7 + (y * CELL_SIZE)), module, StochSeqGrid::CELL_PROB_PARAM + index));
                } else if (index < 8) {
                    addParam(createParamCentered<NanoBlueKnob>(Vec(89.4 + (x * CELL_SIZE), 61.7 + (y * CELL_SIZE)), module, StochSeqGrid::CELL_PROB_PARAM + index));
                } else if (index < 12) {
                    addParam(createParamCentered<NanoAquaKnob>(Vec(89.4 + (x * CELL_SIZE), 61.7 + (y * CELL_SIZE)), module, StochSeqGrid::CELL_PROB_PARAM + index));
                } else {
                    addParam(createParamCentered<NanoRedKnob>(Vec(89.4 + (x * CELL_SIZE), 61.7 + (y * CELL_SIZE)), module, StochSeqGrid::CELL_PROB_PARAM + index));
                }
            }
        }

        addParam(createParamCentered<TinyBlueButton>(Vec(26.3, 93), module, StochSeqGrid::CLOCK_TOGGLE_PARAM));
        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(26.3 - 3, 93 - 3), module, StochSeqGrid::TOGGLE_LIGHT));
        addParam(createParamCentered<BlueKnob>(Vec(50.3, 93), module, StochSeqGrid::BPM_PARAM));

        addOutput(createOutputCentered<PJ301MPurple>(Vec(242.6, 354.8), module, StochSeqGrid::GATES_OUTPUT + StochSeqGrid::PURPLE_SEQ));
        addOutput(createOutputCentered<PJ301MBlue>(Vec(272.6, 354.8), module, StochSeqGrid::GATES_OUTPUT + StochSeqGrid::BLUE_SEQ));
        addOutput(createOutputCentered<PJ301MAqua>(Vec(302.6, 354.8), module, StochSeqGrid::GATES_OUTPUT + StochSeqGrid::AQUA_SEQ));
        addOutput(createOutputCentered<PJ301MRed>(Vec(332.6, 354.8), module, StochSeqGrid::GATES_OUTPUT + StochSeqGrid::RED_SEQ));
        // addOutput(createOutputCentered<PJ301MPort>(Vec(275.4, 354.8), module, StochSeqGrid::VOLT_OUTPUT));
    }

    void appendContextMenu(Menu *menu) override {
        StochSeqGrid *module = dynamic_cast<StochSeqGrid *>(this->module);

        menu->addChild(new MenuSeparator);

        menu->addChild(createIndexPtrSubmenuItem("Gate mode", {"Gates", "Triggers"}, &module->gateMode));

    }
};

Model *modelStochSeqGrid = createModel<StochSeqGrid, StochSeqGridWidget>("StochSeqGrid");
