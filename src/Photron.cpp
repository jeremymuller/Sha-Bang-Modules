#include "Photron.hpp"

#define DISPLAY_SIZE_WIDTH 690
#define DISPLAY_SIZE_HEIGHT 380
#define CELL_SIZE 10
#define BUFFER_SIZE 512  // 512
#define MARGIN 4
#define NUM_OF_MARCHING_CIRCLES 5

struct Photron : Module {
    enum QuadrantIds { NW,
                       NE,
                       SW,
                       SE };
    enum WaveformIds { LINES,
                       BLOCKS,
                       NONE,
                       NUM_WAVEFORMS };
    enum BackgroundIds { COLOR,
                         B_AND_W,
                         BLACK,
                         NUM_BG };
    enum ParamIds {
        RANDOMIZE_PARAM,
        RESET_PARAM,
        BG_COLOR_PARAM,
        X_POS_PARAM,
        Y_POS_PARAM,
        X_SCALE_PARAM,
        Y_SCALE_PARAM,
        COLOR_PARAM,
        TRIG_PARAM,
        WAVEFORM_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        SEPARATE_INPUT,
        ALIGN_INPUT,
        COHESION_INPUT,
        TARGET_INPUT,
        WAVEFORM_INPUT,
        COLOR_TRIGGER_INPUT,
        INVERT_INPUT,
        X_INPUT,
        Y_INPUT,
        NUM_INPUTS
    };
    enum OutputIds { NUM_OUTPUTS };
    enum LightIds { NUM_LIGHTS };

    float bufferX[BUFFER_SIZE] = {};
    float bufferY[BUFFER_SIZE] = {};
    int bufferIndex = 0;
    float frameIndex = 0;
    dsp::SchmittTrigger resetTrigger;

    dsp::SchmittTrigger waveTrig, colorTrig, invertTrig, resetTrig, velTrig;
    int background = COLOR;
    bool waveformLines = true;
    int waveform = LINES;
    bool lissajous = true;
    int resetIndex = 0;
    int checkParams = 0;
    // int srIncrement = static_cast<int>(APP->engine->getSampleRate() /
    // INTERNAL_HZ);
    int hertzIndex = 2;
    int hertz[7] = {60, 45, 30, 20, 15, 12, 10};
    int internalHz = 30;
    float srIncrement = APP->engine->getSampleTime() * internalHz;
    float sr = 0;
    static const int cols = DISPLAY_SIZE_WIDTH / CELL_SIZE;
    static const int rows = DISPLAY_SIZE_HEIGHT / CELL_SIZE;
    Block blocks[rows][cols];
    float field[rows][cols];
    int blockAlpha[rows][cols];
    json_t *patternsRootJ;
    int patternIndex = 3;
    bool lockPattern = false;
    std::vector<std::string> labels;

    MarchingCircle circles[NUM_OF_MARCHING_CIRCLES];

    // expander stuff
    BlockMessage outputValues[rows];
    BlockMessage leftMessages[2][rows];
    Block rightOutputValues[rows];
    Block rightMessages[2][rows];

    Photron() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(X_POS_PARAM, -10.0, 10.0, 0.0, "X offset");
        configParam(Y_POS_PARAM, -10.0, 10.0, 0.0, "Y offset");
        configParam(X_SCALE_PARAM, -2.0, 8.0, 0.5, "X scale");
        configParam(Y_SCALE_PARAM, -2.0, 8.0, 0.5, "Y scale");
        configButton(WAVEFORM_PARAM, "Waveform");
        configButton(COLOR_PARAM, "Background");

        configInput(SEPARATE_INPUT, "Separate CV");
        configInput(ALIGN_INPUT, "Align CV");
        configInput(COHESION_INPUT, "Cohesion CV");
        configInput(TARGET_INPUT, "Target color CV");

        configInput(X_INPUT, "X");
        configInput(Y_INPUT, "Y");

        configInput(WAVEFORM_INPUT, "Waveform CV");
        configInput(COLOR_TRIGGER_INPUT, "Background CV");
        configInput(INVERT_INPUT, "Invert background CV");

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Block b(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE);
                blocks[y][x] = b;
                blocks[y][x].isSet = true;

                field[y][x] = random::uniform();
                blockAlpha[y][x] = 0;
            }
        }

        for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            MarchingCircle c(randRange(DISPLAY_SIZE_WIDTH),
                             randRange(DISPLAY_SIZE_HEIGHT));
            circles[i] = c;
        }

        leftExpander.producerMessage = leftMessages[0];
        leftExpander.consumerMessage = leftMessages[1];

        rightExpander.producerMessage = rightMessages[0];
        rightExpander.consumerMessage = rightMessages[1];

        // load json pattern (will be moved to other file eventually)
        std::string pattern =
            asset::plugin(pluginInstance, "res/invaders.json");

        FILE *file = fopen(pattern.c_str(), "r");
        json_error_t error;
        json_t *patternJson = json_loadf(file, 0, &error);
        fclose(file);

        patternsRootJ = json_object_get(patternJson, "patterns");

        const char *key;
        void *iter = json_object_iter(patternsRootJ);
        while (iter) {
            key = json_object_iter_key(iter);
            labels.push_back(key);
            iter = json_object_iter_next(patternsRootJ, iter);
        }

        resetBlocks(RESET_PARAM);
    }

    ~Photron() { json_decref(patternsRootJ); }

    void onSampleRateChange() override {
        srIncrement = APP->engine->getSampleTime() * internalHz;
    }

    void onRandomize() override {
        resetBlocks(RANDOMIZE_PARAM);
    }

    void onReset() override {
        resetBlocks(RESET_PARAM);
    }

    void setHz(int hz) {
        hertzIndex = hz;
        internalHz = hertz[hertzIndex];
        srIncrement = APP->engine->getSampleTime() * internalHz;
    }

    int getHz() {
        return hertzIndex;
    }

    Vec getCenter() {
        return Vec(cols / 2, rows / 2);
    }

    Vec getQuadrant(QuadrantIds quadrant) {
        switch (quadrant) {
            case NW:
                return Vec(cols / 4, rows / 4);
            case NE:
                return Vec(cols / 4 + cols / 2, rows / 4);
            case SW:
                return Vec(cols / 4, rows / 4 + rows / 2);
            default:
                return Vec(cols / 4 + cols / 2, rows / 4 + rows / 2);
        }
    }

    bool isPatternLocked() {
        return lockPattern;
    }

    void setLockPattern(bool key) {
        lockPattern = key;
        if (!lockPattern) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    blocks[y][x].isLocked = false;
                }
            }
        }
    }

    int *getRandomColor(int randNum) {
        switch (randNum) {
            case 0:
                return getRedAsArray();
                break;
            case 1:
                return getBlueAsArray();
                break;
            case 2:
                return getAquaAsArray();
                break;
            default:
                return getPurpleAsArray();
                break;
        }
    }

    json_t *dataToJson() override {
        // stores whether it's black and white
        // stores current RGB of each block
        json_t *rootJ = json_object();

        json_t *blocksJ = json_array();
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                json_t *rgbJ = json_array();
                Vec3 rgb = blocks[y][x].rgb;

                json_t *redJ = json_integer(rgb.x);
                json_t *greenJ = json_integer(rgb.y);
                json_t *blueJ = json_integer(rgb.z);

                json_array_append_new(rgbJ, redJ);
                json_array_append_new(rgbJ, greenJ);
                json_array_append_new(rgbJ, blueJ);

                json_array_append_new(blocksJ, rgbJ);
            }
        }

        // json_object_set_new(rootJ, "internalHz", json_integer(internalHz));
        json_object_set_new(rootJ, "hertzIndex", json_integer(getHz()));
        json_object_set_new(rootJ, "background", json_integer(background));
        json_object_set_new(rootJ, "waveform", json_integer(waveform));
        json_object_set_new(rootJ, "lissajous", json_boolean(lissajous));
        json_object_set_new(rootJ, "blocks", blocksJ);
        json_object_set_new(rootJ, "pattern", json_integer(patternIndex));
        json_object_set_new(rootJ, "lockPattern", json_boolean(lockPattern));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *hertzIndexJ = json_object_get(rootJ, "hertzIndex");
        if (hertzIndexJ) setHz(json_integer_value(hertzIndexJ));

        json_t *backgroundJ = json_object_get(rootJ, "background");
        if (backgroundJ) background = json_integer_value(backgroundJ);

        json_t *waveformJ = json_object_get(rootJ, "waveform");
        if (waveformJ) waveform = json_integer_value(waveformJ);

        json_t *lissajousJ = json_object_get(rootJ, "lissajous");
        if (lissajousJ) lissajous = json_boolean_value(lissajousJ);

        json_t *patternJ = json_object_get(rootJ, "pattern");
        if (patternJ) patternIndex = json_integer_value(patternJ);

        json_t *lockPatternJ = json_object_get(rootJ, "lockPattern");
        if (lockPatternJ) lockPattern = json_boolean_value(lockPatternJ);

        json_t *blocksJ = json_object_get(rootJ, "blocks");
        if (blocksJ) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    int i = x + y * cols;
                    json_t *rgbJ = json_array_get(blocksJ, i);
                    if (rgbJ) {
                        json_t *redJ = json_array_get(rgbJ, 0);
                        json_t *greenJ = json_array_get(rgbJ, 1);
                        json_t *blueJ = json_array_get(rgbJ, 2);
                        if (redJ) blocks[y][x].rgb.x = json_integer_value(redJ);
                        if (greenJ)
                            blocks[y][x].rgb.y = json_integer_value(greenJ);
                        if (blueJ)
                            blocks[y][x].rgb.z = json_integer_value(blueJ);
                    }
                }
            }
        }
    }

    void process(const ProcessArgs &args) override {
        if (checkParams == 0) {
            if (waveTrig.process(params[WAVEFORM_PARAM].getValue() +
                                 inputs[WAVEFORM_INPUT].getVoltage())) {
                waveformLines = !waveformLines;
                waveform = (waveform + 1) % NUM_WAVEFORMS;
            }
            if (colorTrig.process(params[COLOR_PARAM].getValue() +
                                  inputs[COLOR_TRIGGER_INPUT].getVoltage())) {
                background = (background + 1) % NUM_BG;
            }
            if (invertTrig.process(inputs[INVERT_INPUT].getVoltage())) {
                invertColors();
            }
        }
        checkParams = (checkParams + 1) % 4;

        if (sr == 0) {
            bool isParent = (leftExpander.module &&
                             (leftExpander.module->model == modelPhotron));
            if (isParent) {
                BlockMessage *outputFromParent =
                    (BlockMessage *)(leftExpander.consumerMessage);
                memcpy(outputValues, outputFromParent,
                       sizeof(BlockMessage) * rows);

                setHz(outputValues[0].hertzIndex);
                background = (BackgroundIds)outputValues[0].colorMode;
            }

            bool isRightExpander =
                (rightExpander.module &&
                 (rightExpander.module->model == modelPhotron));
            if (isRightExpander) {
                Block *outputFromRight =
                    (Block *)(rightExpander.consumerMessage);
                memcpy(rightOutputValues, outputFromRight,
                       sizeof(Block) * rows);
            }

            bool isTargetConnected = inputs[TARGET_INPUT].isConnected();

            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    // TODO: clamp these input values?
                    blocks[y][x].sepInput = inputs[SEPARATE_INPUT].getVoltage();
                    blocks[y][x].aliInput = inputs[ALIGN_INPUT].getVoltage();
                    blocks[y][x].cohInput = inputs[COHESION_INPUT].getVoltage();

                    // adjacents
                    Block west;
                    Block east;
                    Block north;
                    Block south;
                    if (isParent && x == 0)
                        west = outputValues[y].block;
                    else if (x > 0)
                        west = blocks[y][x - 1];

                    if (isRightExpander && x == cols - 1)
                        east = rightOutputValues[y];
                    else if (x < cols - 1)
                        east = blocks[y][x + 1];

                    if (y > 0) north = blocks[y - 1][x];
                    if (y < rows - 1) south = blocks[y + 1][x];

                    // corners
                    Block northwest;
                    Block northeast;
                    Block southwest;
                    Block southeast;

                    if (isParent && (x == 0) && (y > 0))
                        northwest = outputValues[y - 1].block;
                    else if ((x > 0) && (y > 0))
                        northwest = blocks[y - 1][x - 1];

                    if (isRightExpander && (x == cols - 1) && (y > 0))
                        northeast = rightOutputValues[y - 1];
                    else if ((x < cols - 1) && (y > 0))
                        northeast = blocks[y - 1][x + 1];

                    if (isParent && (x == 0) && (y < rows - 1))
                        southwest = outputValues[y + 1].block;
                    else if ((y < rows - 1) && (x > 0))
                        southwest = blocks[y + 1][x - 1];

                    if (isRightExpander && (x == cols - 1) && (y < rows - 1))
                        southeast = rightOutputValues[y + 1];
                    else if ((y < rows - 1) && (x < cols - 1))
                        southeast = blocks[y + 1][x + 1];

                    Block b[8] = {west, east, north, south,
                                  northwest, northeast, southwest, southeast};
                    blocks[y][x].flock(b, 8);
                    if (isTargetConnected) {
                        NVGcolor rgbColor =
                            nvgHSL(inputs[TARGET_INPUT].getVoltage(), 1.0, 0.5);
                        Vec3 color = Vec3(rgbColor.r, rgbColor.g, rgbColor.b);
                        Vec3 target = blocks[y][x].seek(color.mult(255.0));
                        target = target.mult(0.7);
                        blocks[y][x].applyForce(target);
                    }

                    blocks[y][x].update();

                    // layer 1 marching stuff
                    blockAlpha[y][x] = calculateCell(blocks[y][x].getCenter());
                }
            }

            for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
                circles[i].update();
            }

            // to expander (right side)
            if (rightExpander.module &&
                (rightExpander.module->model == modelPhotron)) {
                BlockMessage *messageToExpander =
                    (BlockMessage *)(rightExpander.module->leftExpander
                                         .producerMessage);

                messageToExpander[0].hertzIndex = getHz();
                messageToExpander[0].colorMode = (int)background;

                for (int y = 0; y < rows; y++) {
                    messageToExpander[y].block = blocks[y][cols - 1];
                }

                rightExpander.module->leftExpander.messageFlipRequested = true;
            }

            // to expander (left side to parent)
            if (leftExpander.module &&
                (leftExpander.module->model == modelPhotron)) {
                Block *messageToExpander =
                    (Block *)(leftExpander.module->rightExpander
                                  .producerMessage);

                for (int y = 0; y < rows; y++) {
                    messageToExpander[y] = blocks[y][0];
                }

                leftExpander.module->rightExpander.messageFlipRequested = true;
            }
        }
        sr += srIncrement;
        if (sr >= 1.0) {
            sr = 0.0;
        }

        /************ SCOPE STUFF ************/
        // Compute time
        float deltaTime =
            powf(2.0, -14.0);  // powf(2.0, params[TIME_PARAM].value);
        int frameCount = (int)ceilf(deltaTime * args.sampleRate);

        // Add frame to buffer
        if (bufferIndex < BUFFER_SIZE) {
            if (++frameIndex > frameCount) {
                frameIndex = 0;
                bufferX[bufferIndex] = inputs[X_INPUT].value;
                bufferY[bufferIndex] = inputs[Y_INPUT].value;
                bufferIndex++;
            }
        }

        // Are we waiting on the next trigger?
        if (bufferIndex >= BUFFER_SIZE) {
            // Trigger immediately if external but nothing plugged in, or in
            // Lissajous mode if (lissajous || (external &&
            // !inputs[TRIG_INPUT].isConnected())) {
            if (lissajous) {
                bufferIndex = 0;
                frameIndex = 0;
                return;
            }

            // Reset the Schmitt trigger so we don't trigger immediately if the
            // input is high
            if (frameIndex == 0) {
                resetTrigger.reset();
            }
            frameIndex++;

            // Must go below 0.1V to trigger
            // resetTrigger.setThresholds(params[TRIG_PARAM].getValue() - 0.1,
            // params[TRIG_PARAM].getValue()); float gate = external ?
            // inputs[TRIG_INPUT].getVoltage() : inputs[X_INPUT].getVoltage();
            float gate = inputs[X_INPUT].getVoltage();

            // Reset if triggered
            float holdTime = 0.1;
            if (resetTrigger.process(gate) ||
                (frameIndex >= args.sampleRate * holdTime)) {
                bufferIndex = 0;
                frameIndex = 0;
                return;
            }

            // Reset if we've waited too long
            if (frameIndex >= args.sampleRate * holdTime) {
                bufferIndex = 0;
                frameIndex = 0;
                return;
            }
        }
    }

    void generatePattern(Vec pos, int w, int h) {
        int half = static_cast<int>(w / 2);

        Vec patternCenter = Vec(w / 2, h / 2);

        int values[w][h];
        int xOffset = pos.x - patternCenter.x;
        int yOffset = pos.y - patternCenter.y;

        for (int x = 1; x < w; x += 3) {
            for (int y = 1; y < h; y += 3) {
                if (x <= half) {
                    int r = random::uniform() < 0.75 ? 1 : 0;
                    if (random::uniform() < 0.05) r = 2;

                    values[x][y] = r;
                    values[x + 1][y] = r;
                    values[x + 1][y + 1] = r;
                    values[x][y + 1] = r;
                    values[x - 1][y + 1] = r;
                    values[x - 1][y] = r;
                    values[x - 1][y - 1] = r;
                    values[x][y - 1] = r;
                    values[x + 1][y - 1] = r;
                } else {
                    int index = x - (x - half) * 2;
                    int n = values[index][y];
                    values[x][y] = n;
                    values[x + 1][y] = n;
                    values[x + 1][y + 1] = n;
                    values[x][y + 1] = n;
                    values[x - 1][y + 1] = n;
                    values[x - 1][y] = n;
                    values[x - 1][y - 1] = n;
                    values[x][y - 1] = n;
                    values[x + 1][y - 1] = n;
                }
            }
        }

        int *color = getRandomColor(randRange(4));

        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                if (values[x][y] == 1) {
                    blocks[y + yOffset][x + xOffset].setColor(color[0], color[1], color[2]);
                    blocks[y + yOffset][x + xOffset].isLocked = lockPattern;
                } else if (values[x][y] == 2) {
                    blocks[y + yOffset][x + xOffset].setColor(255, 255, 255);
                    blocks[y + yOffset][x + xOffset].isLocked = lockPattern;
                }
            }
        }
    }

    void setPattern(Vec pos) {
        int *color = getRandomColor(randRange(4));

        const char *key = labels.at(patternIndex).c_str();

        const char *keys[] = {"Small Crab", "Medium Crab", "Small Squid", "Medium Squid", "Small Octopus", "Medium Octopus", "Mushrooms"};
        bool doGrid = false;
        int length = sizeof(keys) / sizeof(char *);
        for (int i = 0; i < length; i++) {
            if (strcmp(key, keys[i]) == 0) {
                doGrid = true;
                break;
            }
        }

        if (doGrid) {
            setPattern(getQuadrant(NW), getPurpleAsArray(), labels.at(patternIndex).c_str());
            setPattern(getQuadrant(NE), getBlueAsArray(), labels.at(patternIndex).c_str());
            setPattern(getQuadrant(SW), getAquaAsArray(), labels.at(patternIndex).c_str());
            setPattern(getQuadrant(SE), getRedAsArray(), labels.at(patternIndex).c_str());
        } else if (strcmp(key, "Ghosts") == 0) {
            int spacing = 8;
            setPattern(Vec(pos.x + 2 - spacing * 2 - spacing, pos.y), getPurpleAsArray(), labels.at(patternIndex).c_str());
            setPattern(Vec(pos.x + 2 - spacing, pos.y), getBlueAsArray(), labels.at(patternIndex).c_str());
            setPattern(Vec(pos.x + 2 + spacing, pos.y), getAquaAsArray(), labels.at(patternIndex).c_str());
            setPattern(Vec(pos.x + 2 + spacing * 2 + spacing, pos.y), getRedAsArray(), labels.at(patternIndex).c_str());
        } else {
            setPattern(pos, color, labels.at(patternIndex).c_str());
        }
    }

    void setPattern(Vec pos, int *color, const char *key) {
        json_t *patternsJ = json_object_get(patternsRootJ, key);

        if (patternsJ) {
            int isGen = strcmp(key, "Generate");
            if (strcmp(key, "Generate") == 0) {
                json_t *wJ = json_object_get(patternsJ, "width");
                json_t *hJ = json_object_get(patternsJ, "height");

                if (wJ && hJ) {
                    int w = json_integer_value(wJ);
                    int h = json_integer_value(hJ);
                    generatePattern(pos, w, h);
                }

            } else if (strcmp(key, "Generate Grid") == 0) {
                json_t *wJ = json_object_get(patternsJ, "width");
                json_t *hJ = json_object_get(patternsJ, "height");

                if (wJ && hJ) {
                    int w = json_integer_value(wJ);
                    int h = json_integer_value(hJ);
                    generatePattern(getQuadrant(NW), w, h);
                    generatePattern(getQuadrant(NE), w, h);
                    generatePattern(getQuadrant(SW), w, h);
                    generatePattern(getQuadrant(SE), w, h);
                }
            } else {
                json_t *wJ = json_object_get(patternsJ, "width");
                json_t *hJ = json_object_get(patternsJ, "height");
                Vec patternCenter = Vec(0, 0);
                if (wJ && hJ) {
                    int w = json_integer_value(wJ);
                    int h = json_integer_value(hJ);
                    patternCenter = Vec((int)w / 2, (int)h / 2);
                }

                int xOffset = pos.x - patternCenter.x;
                int yOffset = pos.y - patternCenter.y;

                // int randNum = randRange(4);

                for (int y = 0; y < rows; y++) {
                    for (int x = 0; x < cols; x++) {
                        // blocks[y][x].isLocked = false;

                        auto sX = std::to_string(x);
                        auto sY = std::to_string(y);
                        std::string s = sX + ", " + sY;
                        const char *key = s.c_str();
                        json_t *numJ = json_object_get(patternsJ, key);

                        if (numJ) {
                            // int *color = getRandomColor(randNum);
                            // 0 = white, 1 = black, 2 = random color, 3 =
                            // another color?
                            int n = json_integer_value(numJ);
                            switch (n) {
                                case 0:
                                    blocks[y + yOffset][x + xOffset].setColor(255, 255, 255);
                                    break;
                                case 1:
                                    blocks[y + yOffset][x + xOffset].setColor(0, 0, 0);
                                    break;
                                case 2:
                                    blocks[y + yOffset][x + xOffset].setColor(color[0], color[1], color[2]);
                                    break;
                                default:
                                    blocks[y + yOffset][x + xOffset].setColor(255, 255, 255);
                                    break;
                            }

                            blocks[y + yOffset][x + xOffset].isLocked = lockPattern;
                        }
                    }
                }
            }
        }
    }

    void resetBlocks(int param) {
        if (param == RANDOMIZE_PARAM) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    blocks[y][x].reset();
                }
            }
        } else if (param == BG_COLOR_PARAM) {
            int randNum = randRange(4);

            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    int *color = getRandomColor(randNum);
                    blocks[y][x].setColor(color[0], color[1], color[2]);
                }
            }
        } else if (param == RESET_PARAM) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    blocks[y][x].isLocked = false;
                }
            }

            // if (random::uniform() < 0.5)
            //     resetBlocks(RANDOMIZE_PARAM);
            // else
            //     resetBlocks(BG_COLOR_PARAM);

            // setPattern(getQuadrant(NW));
            // setPattern(getQuadrant(NE));
            // setPattern(getQuadrant(SW));
            // setPattern(getQuadrant(SE));

            setPattern(getCenter());
        }
    }

    void invertColors() {
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Vec3 rgb = blocks[y][x].rgb;
                blocks[y][x].rgb = Vec3(255 - rgb.x, 255 - rgb.y, 255 - rgb.z);
            }
        }
    }

    int calculateCell(Vec blockPos) {
        float sum = 0.0;
        for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            float r = circles[i].radius * 0.9;
            float d = (blockPos.x - circles[i].pos.x) * (blockPos.x - circles[i].pos.x) + (blockPos.y - circles[i].pos.y) * (blockPos.y - circles[i].pos.y);
            d = std::fmax(d, 0.001);  // to prevent dividing by zero
            sum += (r * r) / d;
        }

        if (sum >= 1) {
            return 255;
        } else {
            sum = sum * sum;
            float newSum = rescale(sum, 0.2, 1, 0, 254);
            sum = clamp(newSum, 0.0, 255.0);
            return (int)sum;
        }
    }
};

namespace PhotronNS {
struct HzModeValueItem : MenuItem {
    Photron *module;
    int hz;
    void onAction(const event::Action &e) override {
        module->setHz(hz);
    }
};

struct HzModeItem : MenuItem {
    Photron *module;
    Menu *createChildMenu() override {
        Menu *menu = new Menu;
        std::vector<std::string> hzModes = {"60 Hz", "45 Hz", "30 Hz", "20 Hz", "15 Hz", "12 Hz", "10 Hz"};
        int hertz[] = {60, 45, 30, 20, 15, 12, 10};
        for (int i = 0; i < 7; i++) {
            HzModeValueItem *item = new HzModeValueItem;
            item->text = hzModes[i];
            item->rightText = CHECKMARK(module->internalHz == hertz[i]);
            item->module = module;
            item->hz = hertz[i];
            menu->addChild(item);
        }
        return menu;
    }
};
}  // namespace PhotronNS

struct PhotronDisplay : LightWidget {
    Photron *module;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;
    bool isDKeyHeld = false;

    void onButton(const event::Button &e) override {
        if (module == NULL) return;

        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if (isDKeyHeld) {
                e.consume(this);
                initX = e.pos.x;
                initY = e.pos.y;
            }
        }
    }

    void onDragStart(const event::DragStart &e) override {
        dragX = APP->scene->rack->getMousePos().x;
        dragY = APP->scene->rack->getMousePos().y;
    }

    void onDragMove(const event::DragMove &e) override {
        if (isDKeyHeld) {
            float newDragX = APP->scene->rack->getMousePos().x;
            float newDragY = APP->scene->rack->getMousePos().y;
            drawRandoms(initX + (newDragX - dragX), initY + (newDragY - dragY));
        }
    }

    void onDragEnd(const DragEndEvent &e) override { isDKeyHeld = false; }

    void onHoverKey(const event::HoverKey &e) override {
        if (module == NULL) return;

        if (e.key == GLFW_KEY_D) {
            e.consume(this);
            if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT)
                isDKeyHeld = true;
            else if (e.action == GLFW_RELEASE)
                isDKeyHeld = false;
        } else {
            e.consume(this);
            isDKeyHeld = false;
        }

        // ModuleWidget::onHoverKey(e);
    }

    void drawRandoms(float mX, float mY) {
        int x = static_cast<int>(mX / CELL_SIZE);
        int y = static_cast<int>(mY / CELL_SIZE);

        // module->blocks[y][x].reset();
        module->blocks[y][x].distortColor();

        if (x > 0) module->blocks[y][x - 1].distortColor();
        if (x < module->rows - 1) module->blocks[y][x + 1].distortColor();
        if (y > 0) module->blocks[y - 1][x].distortColor();
        if (x < module->cols - 1) module->blocks[y + 1][x].distortColor();
    }

    void drawWaveform(NVGcontext *vg, float *valuesX, float *valuesY) {
        if (!valuesX) return;
        nvgSave(vg);
        // Rect b = Rect(Vec(0, 15), box.size.minus(Vec(0, 15*2)));
        // nvgScissor(vg, b.pos.x, b.pos.y, b.size.x, b.size.y);
        Rect b = box;
        nvgScissor(vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
        nvgBeginPath(vg);
        // Draw maximum display left to right
        for (int i = 0; i < BUFFER_SIZE; i++) {
            float x, y;
            if (valuesY) {
                x = valuesX[i] / 2.0 + 0.5;
                y = valuesY[i] / 2.0 + 0.5;
            } else {
                x = (float)i / (BUFFER_SIZE - 1);
                y = valuesX[i] / 2.0 + 0.5;
            }
            Vec p;
            p.x = b.pos.x + b.size.x * x;
            p.y = b.pos.y + b.size.y * (1.0 - y);
            int col = clamp((int)(p.x / CELL_SIZE), 0, module->cols - 1);
            int row = clamp((int)(p.y / CELL_SIZE), 0, module->rows - 1);
            Vec3 rgb = module->blocks[0][0].rgb;
            if (module->waveform == Photron::LINES) {
                if (i == 0)
                    nvgMoveTo(vg, p.x, p.y);
                else {
                    // nvgStrokeColor(vg, nvgRGB(rgb.x, rgb.x, rgb.x));
                    // nvgStrokeColor(vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                    // nvgStrokeColor(vg, nvgRGBA(rgb.x, rgb.y, rgb.z, 0xc0));
                    nvgLineTo(vg, p.x, p.y);

                    // nvgBeginPath(vg);
                    // nvgStrokeColor(vg, nvgRGBA(0x9f, 0xe4, 0x36, 0xc0));
                }
            } else if (module->waveform == Photron::BLOCKS) {
                nvgFillColor(vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                // nvgFillColor(vg, nvgRGBA(rgb.x, rgb.x, rgb.x, rgb.y));
                nvgBeginPath(vg);
                nvgRect(vg, module->blocks[row][col].pos.x,
                        module->blocks[row][col].pos.y, CELL_SIZE, CELL_SIZE);
                nvgFill(vg);
            }
        }
        nvgLineCap(vg, NVG_ROUND);
        nvgMiterLimit(vg, 2.0);
        nvgStrokeWidth(vg, 1.5);  // 1.5
        nvgGlobalCompositeOperation(vg, NVG_LIGHTER);
        if (module->waveform == Photron::LINES) nvgStroke(vg);
        nvgResetScissor(vg);
        nvgRestore(vg);
    }

    void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        // background
        //  nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

        /************ COLOR FLOCKING STUFF ************/
        if (module->background == Photron::BLACK) {
            nvgFillColor(args.vg, nvgRGB(0, 0, 0));
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, DISPLAY_SIZE_WIDTH, DISPLAY_SIZE_HEIGHT);
            nvgFill(args.vg);
        } else {
            for (int y = 0; y < DISPLAY_SIZE_HEIGHT / CELL_SIZE; y++) {
                for (int x = 0; x < DISPLAY_SIZE_WIDTH / CELL_SIZE; x++) {
                    Vec3 rgb = module->blocks[y][x].rgb;
                    if (module->background == Photron::COLOR) {
                        nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                    } else {
                        // NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                        // nvgFillColor(args.vg, nvgTransRGBA(color, rgb.y));
                        nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.x, rgb.x));
                    }

                    nvgBeginPath(args.vg);
                    nvgRect(args.vg, module->blocks[y][x].pos.x, module->blocks[y][x].pos.y, CELL_SIZE, CELL_SIZE);
                    nvgFill(args.vg);

                    // bool topEdge = y < MARGIN;
                    // bool bottomEdge = y >= DISPLAY_SIZE_HEIGHT/CELL_SIZE -
                    // MARGIN; bool leftEdge = x < MARGIN; bool rightEdge = x >=
                    // DISPLAY_SIZE_WIDTH/CELL_SIZE - MARGIN;

                    // if (topEdge || bottomEdge || leftEdge || rightEdge) {
                    //     Vec3 rgb = module->blocks[y][x].rgb;
                    //     if (module->background == Photron::COLOR) {
                    //         nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.y,
                    //         rgb.z));
                    //     } else {
                    //         NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                    //         nvgFillColor(args.vg, nvgTransRGBA(color,
                    //         rgb.y));
                    //     }

                    //     // if (module->isColor) {
                    //     //     nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.y,
                    //     rgb.z));
                    //     // } else {
                    //     //     NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                    //     //     nvgFillColor(args.vg, nvgTransRGBA(color,
                    //     rgb.y));
                    //     // }

                    //     nvgBeginPath(args.vg);
                    //     nvgRect(args.vg, module->blocks[y][x].pos.x,
                    //     module->blocks[y][x].pos.y, CELL_SIZE, CELL_SIZE);
                    //     nvgFill(args.vg);
                    // }
                }
            }
        }
    }

    void drawLayer(const DrawArgs &args, int layer) override {
        if (module == NULL) return;

        if (layer == 1) {
            // //background
            // // nvgFillColor(args.vg, nvgRGB(40, 40, 40));
            // nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            // nvgBeginPath(args.vg);
            // nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            // nvgFill(args.vg);

            // /************ COLOR FLOCKING STUFF ************/
            if (module->background != Photron::BLACK) {
                for (int y = 0; y < DISPLAY_SIZE_HEIGHT / CELL_SIZE; y++) {
                    for (int x = MARGIN; x < DISPLAY_SIZE_WIDTH / CELL_SIZE; x++) {
                        Vec3 rgb = module->blocks[y][x].rgb;
                        if (module->background == Photron::COLOR) {
                            nvgFillColor(args.vg, nvgRGBA(rgb.x, rgb.y, rgb.z, module->blockAlpha[y][x]));
                        } else {
                            // NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                            // nvgFillColor(args.vg, nvgTransRGBA(color,
                            // rgb.y));
                            nvgFillColor(args.vg, nvgRGBA(rgb.x, rgb.x, rgb.x, module->blockAlpha[y][x]));
                        }

                        nvgBeginPath(args.vg);
                        nvgRect(args.vg, module->blocks[y][x].pos.x, module->blocks[y][x].pos.y, CELL_SIZE, CELL_SIZE);
                        nvgFill(args.vg);
                    }
                }

                // // draw green circles for debugging
                // for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
                //     Vec circle = module->circles[i].getPos();
                //     float cRadius = module->circles[i].getRadius();

                //     nvgStrokeColor(args.vg, nvgRGB(0, 255, 0));
                //     nvgBeginPath(args.vg);
                //     nvgCircle(args.vg, circle.x, circle.y, cRadius);
                //     nvgStroke(args.vg);
                // }
            }

            /************ SCOPE STUFF ************/
            // code modified from:
            // https://github.com/VCVRack/Fundamental/blob/v0.4.0/src/Scope.cpp
            float gainX = powf(2.0, module->params[Photron::X_SCALE_PARAM].value);
            float gainY = powf(2.0, module->params[Photron::Y_SCALE_PARAM].value);
            float offsetX = module->params[Photron::X_POS_PARAM].value;
            float offsetY = module->params[Photron::Y_POS_PARAM].value;

            float valuesX[BUFFER_SIZE];
            float valuesY[BUFFER_SIZE];
            for (int i = 0; i < BUFFER_SIZE; i++) {
                int j = i;
                // Lock display to buffer if buffer update deltaTime <= 2^-11
                if (module->lissajous)
                    j = (i + module->bufferIndex) % BUFFER_SIZE;
                valuesX[i] = (module->bufferX[j] + offsetX) * gainX / 10.0;
                valuesY[i] = (module->bufferY[j] + offsetY) * gainY / 10.0;
            }

            // Draw waveforms
            if (module->lissajous) {  // module->lissajous
                // X x Y
                if (module->inputs[Photron::X_INPUT].active || module->inputs[Photron::Y_INPUT].active) {
                    Vec3 rgb = module->blocks[0][0].rgb;
                    nvgStrokeColor(args.vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                    // nvgStrokeColor(args.vg, nvgRGBA(0x9f, 0xe4, 0x36, 0xc0));
                    drawWaveform(args.vg, valuesX, valuesY);
                }
            } else {
                // Y
                if (module->inputs[Photron::Y_INPUT].active) {
                    Vec3 rgb = module->blocks[0][0].rgb;
                    nvgStrokeColor(args.vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                    // nvgStrokeColor(args.vg, nvgRGBA(0xe1, 0x02, 0x78, 0xc0));
                    drawWaveform(args.vg, valuesY, NULL);
                }

                // X
                if (module->inputs[Photron::X_INPUT].active) {
                    Vec3 rgb = module->blocks[12][12].rgb;
                    nvgStrokeColor(args.vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                    // nvgStrokeColor(args.vg, nvgRGBA(0x28, 0xb0, 0xf3, 0xc0));
                    drawWaveform(args.vg, valuesX, NULL);
                }

                // float valueTrig = (module->params[Photron::TRIG_PARAM].value
                // + offsetX) * gainX / 10.0; drawTrig(args.vg, valueTrig);
            }
        }
        Widget::drawLayer(args, layer);
    }
};

struct PhotronWidget : ModuleWidget {
    PhotronWidget(Photron *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Photron.svg")));

        PhotronDisplay *display = new PhotronDisplay();
        display->module = module;
        display->box.pos = Vec(0.0, 0.0);
        display->box.size = Vec(DISPLAY_SIZE_WIDTH, DISPLAY_SIZE_HEIGHT);
        addChild(display);

        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 9.7), module, Photron::SEPARATE_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 27.9), module, Photron::ALIGN_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 46.1), module, Photron::COHESION_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 64.4), module, Photron::TARGET_INPUT));

        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 180.9), module, Photron::X_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 199.1), module, Photron::Y_INPUT));

        addParam(createParamCentered<TinyPurpleKnob>(Vec(9.7, 146.4), module, Photron::X_POS_PARAM));
        addParam(createParamCentered<TinyPurpleKnob>(Vec(9.7, 163.3), module, Photron::Y_POS_PARAM));
        addParam(createParamCentered<TinyBlueKnob>(Vec(9.7, 216.7), module, Photron::X_SCALE_PARAM));
        addParam(createParamCentered<TinyBlueKnob>(Vec(9.7, 233.6), module, Photron::Y_SCALE_PARAM));
        addParam(createParamCentered<TinyAquaButton>(Vec(27.2, 333.8), module, Photron::WAVEFORM_PARAM));
        addParam(createParamCentered<TinyRedButton>(Vec(27.2, 352.1), module, Photron::COLOR_PARAM));

        addInput(createInputCentered<TinyPJ301MAqua>(Vec(9.7, 333.8), module, Photron::WAVEFORM_INPUT));
        addInput(createInputCentered<TinyPJ301MRed>(Vec(9.7, 352.1), module, Photron::COLOR_TRIGGER_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 370.3), module, Photron::INVERT_INPUT));
    }

    void appendContextMenu(Menu *menu) override {
        Photron *module = dynamic_cast<Photron *>(this->module);
        // menu->addChild(new MenuEntry);
        menu->addChild(new MenuSeparator);

        // PhotronNS::HzModeItem *hzModeItem = new PhotronNS::HzModeItem;
        // hzModeItem->text = "Processing rate";
        // hzModeItem->rightText = string::f("%d Hz ", module->internalHz) +
        // RIGHT_ARROW; hzModeItem->module = module; menu->addChild(hzModeItem);

        menu->addChild(createIndexSubmenuItem(
            "Processing rate",
            {"60 Hz", "45 Hz", "30 Hz", "20 Hz", "15 Hz", "12 Hz", "10 Hz"},
            [=]() { return module->getHz(); },
            [=](int hz) { module->setHz(hz); }));

        menu->addChild(createBoolPtrMenuItem("Lissajous mode", "", &module->lissajous));

        menu->addChild(new MenuEntry);

        // std::vector<std::string> labels;
        // const char *key;
        // void *iter = json_object_iter(module->patternsRootJ);
        // while (iter) {
        //     key = json_object_iter_key(iter);
        //     labels.push_back(key);
        //     iter = json_object_iter_next(module->patternsRootJ, iter);
        // }

        menu->addChild(createIndexPtrSubmenuItem("Pattern", module->labels, &module->patternIndex));

        // menu->addChild(createBoolPtrMenuItem("Lock Pattern", "",
        // &module->lockPattern));

        menu->addChild(createBoolMenuItem(
            "Lock Pattern", "",
            [=]() { return module->isPatternLocked(); },
            [=](bool lock) {
                module->setLockPattern(lock);
            }));
    }
};

Model *modelPhotron = createModel<Photron, PhotronWidget>("Photron");