#include "plugin.hpp"

#define DISPLAY_SIZE 378
#define MAX_STARS 25
#define INTERNAL_SAMP_TIME 4

struct Star {
    Rect box;
    Vec originalPos;
    Vec posOffset;
    NVGcolor color = nvgRGB(255, 255, 255);
    float radius;
    // bool triggered = false;
    bool alreadyTriggered = false;
    bool blipTrigger = false;
    bool locked = true;
    bool visible = false;
    int haloTime = 0;
    float haloRadius;
    float haloAlpha = 0.0;

    Star() {
        // box.pos.x = _x;
        // box.pos.y = _y;
        posOffset.x = 0;
        posOffset.y = 0;
        radius = randRange(5, 12);
        haloRadius = radius;
    }

    void setPos(Vec pos) {
        // originalPos.x = pos.x;
        // originalPos.y = pos.y;
        // box.pos = originalPos.minus(posOffset);
        box.pos = pos;
    }

    Vec getPos() {
        // returns position with offset
        return box.pos.minus(posOffset);
        // return box.pos;
    }

    void blip() {
        if (haloTime > 22000) {
            haloTime = 0;
            haloRadius = radius;
            blipTrigger = false;
        }

        haloAlpha = rescale(haloTime, 0, 22000, 200, 0);

        haloRadius += 18.0 / (APP->engine->getSampleRate() / INTERNAL_SAMP_TIME);
        haloTime += INTERNAL_SAMP_TIME;
    }
};

struct Cosmosis : Module, Constellations, Quantize {
    enum SeqModeIds {
        PURPLE_SEQ,
        BLUE_SEQ,
        AQUA_SEQ,
        RED_SEQ,
        CLOCKWISE_SEQ,
        COUNTER_CLOCKWISE_SEQ,
        RANDOM_SEQ,
        NUM_SEQ_MODES
    };
    enum ParamIds {
        SPEED_PARAM,
        PLAY_PARAM,
        ROOT_NOTE_PARAM,
        SCALE_PARAM,
        PITCH_PARAM,
        PATTERN_PARAM,
        SIZE_PARAM,
        CLEAR_STARS_PARAM,
        MODE_PARAM,
        RANDOM_POS_PARAM,
        RANDOM_RAD_PARAM,
        RANDOM_SIZE_PARAM,
        OCTAVE_PARAM = MODE_PARAM + 4,
        NUM_PARAMS = OCTAVE_PARAM + 4
    };
    enum InputIds {
        EXT_PLAY_INPUT,
        SPEED_INPUT,
        RESET_INPUT,
        PITCH_CV_INPUT,
        RANDOM_POS_INPUT,
        RANDOM_RAD_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        GATE_OUT,
        VOLT_OUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        PAUSE_LIGHT,
        PURPLE_LIGHT,
        BLUE_LIGHT,
        AQUA_LIGHT,
        RED_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger playTrig, resetTrig, clearTrig, rndPosTrig, rndRadTrig;
    dsp::PulseGenerator gatePulsePoly[16];
    Star *stars = new Star[MAX_STARS];
    Vec center = Vec(DISPLAY_SIZE/2.0, DISPLAY_SIZE/2.0);
    int visibleStars = 0;
    int currentSeqMode = 0;
    int clockWiseIndex = 0;
    int currentConstellation = 0;
    std::string constellationText = "";
    float maxDist;
    NVGcolor blipColor;
    float *seqPos = new float[NUM_SEQ_MODES];
    float seqSpeed = 1.0;
    bool isPlaying = false;
    bool pitchChoice = false;
    int checkParams = 0;
    int processStars = 0;
    int channels = 1;

    Cosmosis() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SPEED_PARAM, -2.0, 2.0, 0.0, "Line speed");
        configInput(SPEED_INPUT, "Lines speed");
        configButton(PLAY_PARAM, "Play");
        configInput(EXT_PLAY_INPUT, "Play");
        configInput(RESET_INPUT, "Reset");
        configInput(PITCH_CV_INPUT, "Root note");
        configButton(RANDOM_POS_PARAM, "Randomize position");
        configInput(RANDOM_POS_INPUT, "Randomize position");
        configButton(RANDOM_RAD_PARAM, "Randomize radius");
        configInput(RANDOM_RAD_INPUT, "Randomize radius");
        configParam(RANDOM_SIZE_PARAM, 0.0, 1.0, 0.0, "Randomize constellation size");
        configParam(ROOT_NOTE_PARAM, 0.0, Quantize::NUM_OF_NOTES - 1, 0.0, "Root note");
        configParam(SCALE_PARAM, 0.0, Quantize::NUM_OF_SCALES, 0.0, "Scale");
        configSwitch(PITCH_PARAM, 0.0, 1.0, 0.0, "Pitch mode", {"size", "position"});
        configParam(PATTERN_PARAM, 0.0, Constellations::NUM_OF_CONSTELLATIONS-1, 0.0, "Constellation");
        configParam(OCTAVE_PARAM + PURPLE_SEQ, -5.0, 5.0, 0.0, "Purple", " oct");
        configParam(OCTAVE_PARAM + BLUE_SEQ, -5.0, 5.0, 0.0, "Blue", " oct");
        configParam(OCTAVE_PARAM + AQUA_SEQ, -5.0, 5.0, 0.0, "Aqua", " oct");
        configParam(OCTAVE_PARAM + RED_SEQ, -5.0, 5.0, 0.0, "Red", " oct");
        configParam(SIZE_PARAM, 0.0, 1.0, 1.0, "Resize Constellation"); // OLD
        configButton(CLEAR_STARS_PARAM, "Clear stars");
        configParam(MODE_PARAM, 0.0, NUM_SEQ_MODES-1, 0.0, "Mode");

        configOutput(VOLT_OUT, "Pitch (V/OCT)");
        configOutput(GATE_OUT, "Trigger");

        Vec corner = Vec(0, 0);
        Vec dir = corner.minus(center);
        maxDist = sqrt(dir.x * dir.x + dir.y * dir.y) * 0.5;

        setConstellation(0);

        // resetSeq();

        // Point *constellation = Constellations::getConstellation(Constellations::ANDROMEDA);
        // for (int i = 0; i < Constellations::constellationLength; i++) {
        //     Vec pos = Vec(constellation[i].x, constellation[i].y);
        //     addStar(pos, i, constellation[i].r);
        // }
        // for (int i = 0; i < 9; i++) {
        //     Vec pos = Vec(CONSTELLATION_ANDROMEDA[i].x, CONSTELLATION_ANDROMEDA[i].y);
        //     addStar(pos, i, CONSTELLATION_ANDROMEDA[i].r);
        // }
    }

    ~Cosmosis () {
        delete[] stars;
        delete[] seqPos;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        json_t *starsJ = json_array();
        for (int i = 0; i < MAX_STARS; i++) {
            json_t *dataJ = json_array();

            json_t *starVisibleJ = json_boolean(stars[i].visible);
            json_t *starPosXJ = json_real(stars[i].box.pos.x);
            json_t *starPosYJ = json_real(stars[i].box.pos.y);
            json_t *starRadiusJ = json_real(stars[i].radius);

            json_array_append_new(dataJ, starVisibleJ);
            json_array_append_new(dataJ, starPosXJ);
            json_array_append_new(dataJ, starPosYJ);
            json_array_append_new(dataJ, starRadiusJ);

            json_array_append_new(starsJ, dataJ);
        }

        json_object_set_new(rootJ, "constellationText", json_string(constellationText.c_str())); // errors here
        json_object_set_new(rootJ, "currentConstellation", json_integer(currentConstellation));
        json_object_set_new(rootJ, "channels", json_integer(channels));
        json_object_set_new(rootJ, "playing", json_boolean(isPlaying));
        json_object_set_new(rootJ, "stars", starsJ);

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *channelsJ = json_object_get(rootJ, "channels");
        if (channelsJ) channels = json_integer_value(channelsJ);

        json_t *playingJ = json_object_get(rootJ, "playing");
        if (playingJ) isPlaying = json_boolean_value(playingJ);

        json_t *currConstJ = json_object_get(rootJ, "currentConstellation");
        if (currConstJ) currentConstellation = json_integer_value(currConstJ);

        json_t *textJ = json_object_get(rootJ, "constellationText");
        if (textJ) constellationText = json_string_value(textJ);

        json_t *starsJ = json_object_get(rootJ, "stars");
        if (starsJ) {
            for (int i = 0; i < MAX_STARS; i++) {
                json_t *dataJ = json_array_get(starsJ, i);
                if (dataJ) {
                    json_t *starVisibleJ = json_array_get(dataJ, 0);
                    json_t *starPosXJ = json_array_get(dataJ, 1);
                    json_t *starPosYJ = json_array_get(dataJ, 2);
                    json_t * starRadiusJ = json_array_get(dataJ, 3);
                    if (starVisibleJ) stars[i].visible = json_boolean_value(starVisibleJ);
                    if (starPosXJ) stars[i].box.pos.x = json_real_value(starPosXJ);
                    if (starPosYJ) stars[i].box.pos.y = json_real_value(starPosYJ);
                    if (starRadiusJ) stars[i].radius = json_real_value(starRadiusJ);
                }
            }
        }
    }

    void process(const ProcessArgs &args) override {
        // check params every 4th sample
        if (checkParams == 0) {
            if (playTrig.process(params[PLAY_PARAM].getValue() + inputs[EXT_PLAY_INPUT].getVoltage())) {
                isPlaying = !isPlaying;
            }
            if (resetTrig.process(inputs[RESET_INPUT].getVoltage())) {
                resetSeq();
            }
            if (clearTrig.process(params[CLEAR_STARS_PARAM].getValue())) {
                removeAllStars();
            }
            if (rndPosTrig.process(params[RANDOM_POS_PARAM].getValue() + inputs[RANDOM_POS_INPUT].getVoltage())) {
                randomizePosition();
            }
            if (rndRadTrig.process(params[RANDOM_RAD_PARAM].getValue() + inputs[RANDOM_RAD_INPUT].getVoltage())) {
                randomizeRadii();
            }
            int mode = params[MODE_PARAM].getValue();
            if (mode != currentSeqMode) {
                setSeqMode(mode);
            }

            pitchChoice = params[PITCH_PARAM].getValue();
            float scl = std::pow(2.0, params[SPEED_PARAM].getValue() + inputs[SPEED_INPUT].getVoltage() * 0.5);
            seqSpeed = scl * (INTERNAL_SAMP_TIME / args.sampleRate * 60.0);

            int paramVal = params[PATTERN_PARAM].getValue();
            if (currentConstellation != paramVal) {
                currentConstellation = paramVal;
                setConstellation(paramVal);
            }

            resizeConstellation();
        }
        checkParams = (checkParams+1) % 4;

        if (processStars == 0) {
            int polyChannelIndex = 0;
            int rootNote = 0;
            if (inputs[PITCH_CV_INPUT].isConnected()) {
                rootNote = static_cast<int>(inputs[PITCH_CV_INPUT].getVoltage(0) * 12) % 12;
            } else {
                rootNote = params[ROOT_NOTE_PARAM].getValue();
            }
            int scale = params[SCALE_PARAM].getValue();

            lights[PAUSE_LIGHT].setBrightness(isPlaying ? 1.0 : 0.0);

            if (currentSeqMode < CLOCKWISE_SEQ) {
                lights[PURPLE_LIGHT].setBrightness(currentSeqMode == PURPLE_SEQ ? 1.0 : 0.0);
                lights[BLUE_LIGHT].setBrightness(currentSeqMode == BLUE_SEQ ? 1.0 : 0.0);
                lights[AQUA_LIGHT].setBrightness(currentSeqMode == AQUA_SEQ ? 1.0 : 0.0);
                lights[RED_LIGHT].setBrightness(currentSeqMode == RED_SEQ ? 1.0 : 0.0);
            } else {
                lights[PURPLE_LIGHT].setBrightness(1.0);
                lights[BLUE_LIGHT].setBrightness(1.0);
                lights[AQUA_LIGHT].setBrightness(1.0);
                lights[RED_LIGHT].setBrightness(1.0);
            }

            if (isPlaying) {
                advanceSeqPos();
                checkSeqEdges();

                int oct = params[OCTAVE_PARAM + getSeqMode()].getValue();
                // change the MAX_STARS to the current length of selected constellation
                for (int i = 0; i < MAX_STARS; i++) {
                    if (stars[i].visible && isStarTrigger(i)) {
                        stars[i].blipTrigger = true;
                        stars[i].alreadyTriggered = true;
                        gatePulsePoly[polyChannelIndex].trigger(1e-3f);

                        float volts = getVolts(stars[i]);
                        // float margin = 5.0;
                        // Vec pos = stars[i].getPos();
                        // if (pitchChoice) volts = rescale(pos.y, DISPLAY_SIZE-margin, margin, 0.0, 2.0);
                        // else volts = rescale(stars[i].radius, 5.0, 12.0, 2.0, 0.0);
                        float pitch = Quantize::quantizeRawVoltage(volts, rootNote, scale) + oct;

                        outputs[VOLT_OUT].setVoltage(pitch, polyChannelIndex);
                    }
                    if (stars[i].blipTrigger) stars[i].blip();

                    bool pulse = gatePulsePoly[polyChannelIndex].process(1.0 / args.sampleRate);
                    outputs[GATE_OUT].setVoltage(pulse ? 10.0 : 0.0, polyChannelIndex);
                    polyChannelIndex = (polyChannelIndex + 1) % channels;
                }
            }

            outputs[GATE_OUT].setChannels(channels);
            outputs[VOLT_OUT].setChannels(channels);
        }
        processStars = (processStars+1) % INTERNAL_SAMP_TIME;

    }

    void setConstellation(int patt) {
        resetSeq();
        removeAllStars();

        // for (int i = 0; i < 9; i++) {
        //     Vec pos = Vec(CONSTELLATION_ANDROMEDA[i].x, CONSTELLATION_ANDROMEDA[i].y);
        //     addStar(pos, i, CONSTELLATION_ANDROMEDA[i].r);
        // }

        constellationText = Constellations::constellationName(patt);
        Point *constellation = Constellations::getConstellation(patt);
        for (int i = 0; i < Constellations::constellationLength; i++) {
            Vec pos = Vec(constellation[i].x, constellation[i].y);
            addStar(pos, i, constellation[i].r);
        }
    }

    void advanceSeqPos() {
        // int seqMode = (currentSeqMode < CLOCKWISE_SEQ) ? currentSeqMode : clockWiseIndex;
        int seqMode = getSeqMode();
        switch (seqMode) {
            case PURPLE_SEQ:
                blipColor = nvgRGB(213, 153, 255);
                seqPos[PURPLE_SEQ] += seqSpeed;
                break;
            case BLUE_SEQ:
                blipColor = nvgRGB(165, 152, 255);
                seqPos[BLUE_SEQ] += seqSpeed;
                break;
            case AQUA_SEQ:
                blipColor = nvgRGB(104, 245, 255);
                seqPos[AQUA_SEQ] -= seqSpeed;
                break;
            case RED_SEQ:
                blipColor = nvgRGB(255, 101, 101);
                seqPos[RED_SEQ] -= seqSpeed;
                break;
        }

    }

    int getSeqMode() {
        if (currentSeqMode < CLOCKWISE_SEQ) return currentSeqMode;
        else return clockWiseIndex;
    }

    void setSeqMode(int mode) {
        resetSeq();
        currentSeqMode = mode;
    }

    void resetSeq() {
        seqPos[PURPLE_SEQ] = 0;
        seqPos[BLUE_SEQ] = 0;
        seqPos[AQUA_SEQ] = DISPLAY_SIZE;
        seqPos[RED_SEQ] = DISPLAY_SIZE;
        for (int i = 0; i < MAX_STARS; i++) {
            stars[i].alreadyTriggered = false;
        }
    }

    void checkSeqEdges() {
        bool reset = false;
        if (seqPos[PURPLE_SEQ] > DISPLAY_SIZE) {
            seqPos[PURPLE_SEQ] = 0;
            reset = true;
        } else if (seqPos[BLUE_SEQ] > DISPLAY_SIZE) {
            seqPos[BLUE_SEQ] = 0;
            reset = true;
        } else if (seqPos[AQUA_SEQ] < 0) {
            seqPos[AQUA_SEQ] = DISPLAY_SIZE;
            reset = true;
        } else if (seqPos[RED_SEQ] < 0) {
            seqPos[RED_SEQ] = DISPLAY_SIZE;
            reset = true;
        }

        if (reset) {
            if (currentSeqMode == CLOCKWISE_SEQ) clockWiseIndex = (clockWiseIndex+1) % 4;
            else if (currentSeqMode == COUNTER_CLOCKWISE_SEQ) clockWiseIndex = (clockWiseIndex ? clockWiseIndex : 4) - 1;
            else if (currentSeqMode == RANDOM_SEQ) clockWiseIndex = static_cast<int>(random::uniform() * 4);
            for (int i = 0; i < MAX_STARS; i++) {
                stars[i].alreadyTriggered = false;
            }
        }
    }

    void resizeConstellation() {
        // TODO: for a future version
        // when resizing and trying to add stars it's weird
        for (int i = 0; i < MAX_STARS; i++) {
            if (stars[i].visible) {
                Vec pos = stars[i].box.getCenter();
                Vec mag = pos.minus(center);
                float maxDist = sqrt(mag.x * mag.x + mag.y * mag.y) * 0.5;
                mag = mag.normalize();
                float m = rescale(params[SIZE_PARAM].getValue(), 0, 1, maxDist, 0);
                mag = mag.mult(m);
                stars[i].posOffset = mag;
                stars[i].setPos(pos);
            } else {
                stars[i].posOffset = Vec(0, 0);
            }
        }

        // return mag;
        // pos = pos.minus(mag);
        // stars[index].box.pos.x = pos.x;
        // stars[index].box.pos.y = pos.y;
    }

    bool isStarTrigger(int index) {
        if (!stars[index].alreadyTriggered) {
            Vec pos = stars[index].getPos();
            // int seqMode = (currentSeqMode < CLOCKWISE_SEQ) ? currentSeqMode : clockWiseIndex;
            int seqMode = getSeqMode();
            switch (seqMode) {
                case PURPLE_SEQ:    return seqPos[PURPLE_SEQ] > pos.x ? true : false;
                case BLUE_SEQ:      return seqPos[BLUE_SEQ] > pos.y ? true : false;
                case AQUA_SEQ:      return seqPos[AQUA_SEQ] < pos.x ? true : false;
                case RED_SEQ:       return seqPos[RED_SEQ] < pos.y ? true : false;
                default:            return false;
            }
        } else {
            return false;
        }
    }

    void addStar(Vec pos, int index) {
        // this one is called when a user clicks somewhere
        visibleStars++;
        stars[index].setPos(pos);
        stars[index].radius = randRange(5, 10);
        stars[index].visible = true;
        stars[index].locked = false;
        int seqMode = getSeqMode();
        stars[index].alreadyTriggered = pos.x < seqPos[seqMode] ? true : false;

        Vec mag = pos.minus(center);
        float maxDist = sqrt(mag.x * mag.x + mag.y * mag.y) * 0.5;
        mag = mag.normalize();
        float m = rescale(params[SIZE_PARAM].getValue(), 0, 1, maxDist, 0);
        mag = mag.mult(m);
        stars[index].setPos(pos.plus(mag));
        stars[index].posOffset = mag;

        constellationText = "";

        // stars[index].posOffset = Vec(0, 0);
    }

    void addStar(Vec pos, int index, float _radius) {
        visibleStars++;
        stars[index].setPos(pos);
        stars[index].radius = _radius;
        stars[index].visible = true;
        stars[index].locked = false;
    }

    void removeStar(int index) {
        visibleStars--;
        stars[index].visible = false;
        stars[index].locked = true;
        constellationText = "";
    }

    void removeAllStars() {
        for (int i = 0; i < MAX_STARS; i++) {
            removeStar(i);
        }
        visibleStars = 0;
        constellationText = "";
    }

    void randomizeRadii() {
        for (int i = 0; i < MAX_STARS; i++) {
            if (stars[i].visible) {
                stars[i].radius = randRange(5.0, 12.0);
            }
        }
        constellationText = "";
    }

    void randomizePosition() {
        for (int i = 0; i < MAX_STARS; i++) {
            if (stars[i].visible) {
                float r = stars[i].radius;
                float x = randRange(r, DISPLAY_SIZE - r);
                float y = randRange(r, DISPLAY_SIZE - r);
                stars[i].setPos(Vec(x, y));
            }
            constellationText = "";
        }
    }

    float getVolts(Star star) {
        if (pitchChoice){
            float margin = 7.0;
            Vec pos = star.getPos();
            // int seqMode = (currentSeqMode < CLOCKWISE_SEQ) ? currentSeqMode : clockWiseIndex;
            int seqMode = getSeqMode();
            switch (seqMode) {
                case PURPLE_SEQ:    return rescale(pos.y, DISPLAY_SIZE - margin, margin, 0.0, 2.0);
                case BLUE_SEQ:      return rescale(pos.x, DISPLAY_SIZE - margin, margin, 0.0, 2.0);
                case AQUA_SEQ:      return rescale(pos.y, DISPLAY_SIZE - margin, margin, 0.0, 2.0);
                case RED_SEQ:       return rescale(pos.x, DISPLAY_SIZE - margin, margin, 0.0, 2.0);
                default:            return rescale(pos.y, DISPLAY_SIZE - margin, margin, 0.0, 2.0);
            }
        }
        else
            return rescale(star.radius, 5.0, 12.0, 2.0, 0.0);
    }
};

namespace CosmosisNS {
    struct ChannelValueItem : MenuItem {
        Cosmosis *module;
        int channels;
        void onAction(const event::Action &e) override {
            module->channels = channels;
        }
    };

    struct ChannelItem : MenuItem {
        Cosmosis *module;
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

struct CosmosisDisplay : Widget {
    Cosmosis *module;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;
    CosmosisDisplay() {}

    void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            e.consume(this);
            initX = e.pos.x;
            initY = e.pos.y;
            Vec inits = Vec(initX, initY);
            bool clickedOnStar = false;
            int nextAvailableIndex = 0;
            for (int i = 0; i < MAX_STARS; i++) {
                if (module->stars[i].visible) {
                    Vec starPos = module->stars[i].getPos();
                    float d = dist(inits, starPos);
                    float r = module->stars[i].radius;
                    if (d < r && !clickedOnStar) {
                        module->stars[i].box.pos.x = initX;
                        module->stars[i].box.pos.y = initY;
                        module->stars[i].locked = false;
                        clickedOnStar = true;
                    } else {
                        module->stars[i].locked = true;
                    }
                } else {
                    nextAvailableIndex = i;
                }
            }

            if (!clickedOnStar && (module->visibleStars < MAX_STARS)) {
                module->addStar(inits, nextAvailableIndex);
                // if (module->visibleStars < MAX_STARS) {
                // }
            }

        }

    }

    void onDragStart(const event::DragStart &e) override {
        dragX = APP->scene->rack->getMousePos().x;
        dragY = APP->scene->rack->getMousePos().y;
    }

    void onDragMove(const event::DragMove &e) override {
        float newDragX = APP->scene->rack->getMousePos().x;
        float newDragY = APP->scene->rack->getMousePos().y;

        for (int i = 0; i < MAX_STARS; i++) {
            if (module->stars[i].visible && !module->stars[i].locked) {
                module->stars[i].box.pos.x = initX + (newDragX - dragX);
                module->stars[i].box.pos.y = initY + (newDragY - dragY);
                checkEdgesForRemove(i);
            }
        }
    }

    void checkEdgesForRemove(int index) {
        if (module != NULL) {
            bool eraseStar = false;
            float r = module->stars[index].radius;

            if (module->stars[index].box.pos.x < r) eraseStar = true;
            else if (module->stars[index].box.pos.x > box.size.x - r) eraseStar = true;
            else if (module->stars[index].box.pos.y < r) eraseStar = true;
            else if (module->stars[index].box.pos.y > box.size.y - r) eraseStar = true;

            if (eraseStar) {
                module->removeStar(index);
            }
        }
    }

    void draw(const DrawArgs &args) override {
        if (module == NULL) return;


        //background
        nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

        nvgGlobalTint(args.vg, color::WHITE);

        // name of constellation
        std::string text = module->constellationText;
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        // nvgFillColor(args.vg, nvgRGB(128, 0, 219));
        nvgFontSize(args.vg, 12);
        nvgText(args.vg, 5, 12, text.c_str(), NULL);

        // draw stars
        for (int i = 0; i < MAX_STARS; i++) {
            if (module->stars[i].visible) {
                Vec pos = module->stars[i].getPos();

                if (module->stars[i].blipTrigger) {
                    // use current seq line color
                    nvgFillColor(args.vg, nvgTransRGBA(module->blipColor, module->stars[i].haloAlpha));
                    nvgBeginPath(args.vg);
                    nvgCircle(args.vg, pos.x, pos.y, module->stars[i].haloRadius);
                    nvgFill(args.vg);
                }

                nvgFillColor(args.vg, nvgTransRGBA(module->stars[i].color, 90));
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, module->stars[i].radius);
                nvgFill(args.vg);

                nvgFillColor(args.vg, module->stars[i].color);
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, 2.5);
                nvgFill(args.vg);
            }
        }

        nvgStrokeWidth(args.vg, 2.0);

        // draw Purple line
        nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, module->seqPos[Cosmosis::PURPLE_SEQ], 0);
        nvgLineTo(args.vg, module->seqPos[Cosmosis::PURPLE_SEQ], box.size.y);
        nvgStroke(args.vg);
        // draw Blue line
        nvgStrokeColor(args.vg, nvgRGB(38, 0, 255));
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, module->seqPos[Cosmosis::BLUE_SEQ]);
        nvgLineTo(args.vg, box.size.x, module->seqPos[Cosmosis::BLUE_SEQ]);
        nvgStroke(args.vg);
        // draw Aqua line
        nvgStrokeColor(args.vg, nvgRGB(0, 238, 219));
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, module->seqPos[Cosmosis::AQUA_SEQ], 0);
        nvgLineTo(args.vg, module->seqPos[Cosmosis::AQUA_SEQ], box.size.y);
        nvgStroke(args.vg);
        // draw Red line
        nvgStrokeColor(args.vg, nvgRGB(255, 0, 0));
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, module->seqPos[Cosmosis::RED_SEQ]);
        nvgLineTo(args.vg, box.size.x, module->seqPos[Cosmosis::RED_SEQ]);
        nvgStroke(args.vg);

    }
};

struct ModeKnob : BlueInvertKnobLabelCentered {
	ModeKnob(){}
	std::string formatCurrentValue() override {
		if(getParamQuantity() != NULL){
			switch(int(getParamQuantity()->getValue())){
				case Cosmosis::PURPLE_SEQ:              return "→";
				case Cosmosis::BLUE_SEQ:                return "↓";
				case Cosmosis::AQUA_SEQ:                return "←";
				case Cosmosis::RED_SEQ:                 return "↑";
				case Cosmosis::CLOCKWISE_SEQ:           return "→";
                case Cosmosis::COUNTER_CLOCKWISE_SEQ:   return "←";
                case Cosmosis::RANDOM_SEQ:              return "R";
			}
		}
		return "";
	}
};

struct CosmosisWidget : ModuleWidget {
    CosmosisWidget(Cosmosis *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Cosmosis.svg")));

        CosmosisDisplay *display = new CosmosisDisplay();
        display->module = module;
        display->box.pos = Vec(161.2, 0.8);
        display->box.size = Vec(DISPLAY_SIZE, DISPLAY_SIZE);
        addChild(display);

        // screws
        addChild(createWidget<JeremyScrew>(Vec(74.6, 2)));
        addChild(createWidget<JeremyScrew>(Vec(74.6, box.size.y - 14)));
        // addChild(createWidget<JeremyScrew>(Vec(431, 2)));
        // addChild(createWidget<JeremyScrew>(Vec(431, box.size.y - 14)));

        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(48.3 - 3.21, 23.6 - 3.21), module, Cosmosis::PAUSE_LIGHT));

        addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(15.9 - 3.21, 276.8 - 3.21), module, Cosmosis::PURPLE_LIGHT));
        addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(26.4 - 3.21, 267.1 - 3.21), module, Cosmosis::BLUE_LIGHT));
        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(37 - 3.21, 276.8 - 3.21), module, Cosmosis::AQUA_LIGHT));
        addChild(createLight<SmallLight<JeremyRedLight>>(Vec(26.4 - 3.21, 286.6 - 3.21), module, Cosmosis::RED_LIGHT));

        addParam(createParamCentered<DefaultButton>(Vec(26.4, 65.3), module, Cosmosis::PLAY_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(61.2, 65.3), module, Cosmosis::SPEED_PARAM));
        addParam(createParamCentered<BlueInvertKnob>(Vec(96, 65.3), module, Cosmosis::PATTERN_PARAM));
        addParam(createParamCentered<DefaultButton>(Vec(130.7, 65.3), module, Cosmosis::CLEAR_STARS_PARAM));

        // note and scale knobs
        BlueNoteKnob *noteKnob = dynamic_cast<BlueNoteKnob *>(createParamCentered<BlueNoteKnob>(Vec(26.4, 122.3), module, Cosmosis::ROOT_NOTE_PARAM));
        LeftAlignedLabel *const noteLabel = new LeftAlignedLabel;
        noteLabel->box.pos = Vec(42.6, 125.8);
        noteLabel->text = "";
        noteKnob->connectLabel(noteLabel, module);
        addChild(noteLabel);
        addParam(noteKnob);

        BlueScaleKnob *scaleKnob = dynamic_cast<BlueScaleKnob *>(createParamCentered<BlueScaleKnob>(Vec(26.4, 153.4), module, Cosmosis::SCALE_PARAM));
        LeftAlignedLabel *const scaleLabel = new LeftAlignedLabel;
        scaleLabel->box.pos = Vec(42.6, 157.7);
        scaleLabel->text = "";
        scaleKnob->connectLabel(scaleLabel, module);
        addChild(scaleLabel);
        addParam(scaleKnob);

        ModeKnob *modeKnob = dynamic_cast<ModeKnob *>(createParamCentered<ModeKnob>(Vec(26.4, 249.4), module, Cosmosis::MODE_PARAM));
        CenterAlignedLabel *const modeLabel = new CenterAlignedLabel;
        modeLabel->box.pos = Vec(26.4, 280.4);
        modeLabel->text = "";
        modeKnob->connectLabel(modeLabel, module);
        addChild(modeLabel);
        addParam(modeKnob);

        addParam(createParamCentered<TinyBlueButton>(Vec(108.9, 243.7), module, Cosmosis::RANDOM_POS_PARAM));
        addParam(createParamCentered<TinyBlueButton>(Vec(108.9, 272.7), module, Cosmosis::RANDOM_RAD_PARAM));
        addParam(createParamCentered<Jeremy_HSwitchBlue>(Vec(111.4, 122.8), module, Cosmosis::PITCH_PARAM));
        addParam(createParamCentered<PurpleInvertKnob>(Vec(26.4, 195.3), module, Cosmosis::OCTAVE_PARAM + Cosmosis::PURPLE_SEQ));
        addParam(createParamCentered<BlueInvertKnob>(Vec(61.2, 195.3), module, Cosmosis::OCTAVE_PARAM + Cosmosis::BLUE_SEQ));
        addParam(createParamCentered<AquaInvertKnob>(Vec(96, 195.3), module, Cosmosis::OCTAVE_PARAM + Cosmosis::AQUA_SEQ));
        addParam(createParamCentered<RedInvertKnob>(Vec(130.7, 195.3), module, Cosmosis::OCTAVE_PARAM + Cosmosis::RED_SEQ));

        addInput(createInputCentered<TinyPJ301M>(Vec(26.4, 90.7), module, Cosmosis::EXT_PLAY_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(61.2, 90.7), module, Cosmosis::SPEED_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(130.7, 90.7), module, Cosmosis::RESET_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(72.3, 122.8), module, Cosmosis::PITCH_CV_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(80.7, 243.5), module, Cosmosis::RANDOM_POS_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(80.7, 272.7), module, Cosmosis::RANDOM_RAD_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(Vec(32.1, 343.2), module, Cosmosis::GATE_OUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64.4, 343.2), module, Cosmosis::VOLT_OUT));
    }

    void appendContextMenu(Menu *menu) override {
        Cosmosis *module = dynamic_cast<Cosmosis*>(this->module);

        menu->addChild(new MenuEntry);

        CosmosisNS::ChannelItem *channelItem = new CosmosisNS::ChannelItem;
        channelItem->text = "Polyphony channels";
        channelItem->rightText = string::f("%d", module->channels) + " " + RIGHT_ARROW;
        channelItem->module = module;
        menu->addChild(channelItem);
    }
};

Model *modelCosmosis = createModel<Cosmosis, CosmosisWidget>("Cosmosis");