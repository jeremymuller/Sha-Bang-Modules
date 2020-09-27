#include "plugin.hpp"

#define DISPLAY_SIZE 378
#define MAX_STARS 25
#define INTERNAL_SAMP_TIME 4

struct Star {
    Rect box;
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
        box.pos.x = pos.x;
        box.pos.y = pos.y;
    }

    Vec getPos() {
        // returns position with offset
        return box.pos.minus(posOffset);
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
        NUM_SEQ_MODES
    };
    enum ParamIds {
        SPEED_PARAM,
        PLAY_PARAM,
        ROOT_NOTE_PARAM,
        SCALE_PARAM,
        PITCH_PARAM,
        PATTERN_PARAM,
        OCTAVE_PARAM,
        SIZE_PARAM,
        CLEAR_STARS_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        GATE_OUT,
        VOLT_OUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        PAUSE_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger playTrig, clearTrig;
    dsp::PulseGenerator gatePulsePoly[16];
    Star *stars = new Star[MAX_STARS];
    int visibleStars = 0;
    int currentSeqMode = PURPLE_SEQ;
    float seqPos = 0;
    float seqSpeed = 1.0;
    bool isPlaying = false;
    bool pitchChoice = false;
    int checkParams = 0;
    int processStars = 0;
    int channels = 1;

    Cosmosis() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SPEED_PARAM, -2.0, 2.0, 0.0);
        configParam(PLAY_PARAM, 0.0, 1.0, 0.0);
        configParam(ROOT_NOTE_PARAM, 0.0, Quantize::NUM_OF_NOTES - 1, 0.0, "Root note");
        configParam(SCALE_PARAM, 0.0, Quantize::NUM_OF_SCALES, 0.0, "Scale");
        configParam(PITCH_PARAM, 0.0, 1.0, 0.0);
        configParam(OCTAVE_PARAM, -5.0, 5.0, 0.0, "Octave");
        configParam(SIZE_PARAM, 0.0, 1.0, 1.0, "Resize Constellation");
        configParam(CLEAR_STARS_PARAM, 0.0, 1.0, 0.0, "Clear stars");

        for (int i = 0; i < 9; i++) {
            Vec pos = Vec(ANDROMEDA[i].x, ANDROMEDA[i].y);
            addStar(pos, i, ANDROMEDA[i].r);
        }
    }

    ~Cosmosis () {
        delete[] stars;
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
        // check params every 4th sample
        if (checkParams == 0) {
            if (playTrig.process(params[PLAY_PARAM].getValue())) {
                isPlaying = !isPlaying;
            }
            if (clearTrig.process(params[CLEAR_STARS_PARAM].getValue())) {
                removeAllStars();
            }

            pitchChoice = params[PITCH_PARAM].getValue();
            float scl = std::pow(2, params[SPEED_PARAM].getValue());
            seqSpeed = scl * (INTERNAL_SAMP_TIME / args.sampleRate * 60.0);

            resizeConstellation();
        }
        checkParams = (checkParams+1) % 4;

        if (processStars == 0) {
            int polyChannelIndex = 0;
            int rootNote = params[ROOT_NOTE_PARAM].getValue();
            int scale = params[SCALE_PARAM].getValue();

            lights[PAUSE_LIGHT].setBrightness(isPlaying ? 1.0 : 0.0);

            if (isPlaying) {
                seqPos += seqSpeed;
                if (seqPos > DISPLAY_SIZE) {
                    seqPos = 0;
                    for (int i = 0; i < MAX_STARS; i++) {
                        stars[i].alreadyTriggered = false;
                    }
                }

                int oct = params[OCTAVE_PARAM].getValue();
                // change the MAX_STARS to the current length of selected constellation
                for (int i = 0; i < MAX_STARS; i++) {
                    if (stars[i].visible && isStarTrigger(i)) {
                        stars[i].blipTrigger = true;
                        stars[i].alreadyTriggered = true;
                        gatePulsePoly[polyChannelIndex].trigger(1e-3f);

                        float margin = 5.0;
                        float volts;
                        Vec pos = stars[i].getPos();
                        if (pitchChoice) volts = rescale(pos.y, DISPLAY_SIZE-margin, margin, 0.0, 2.0);
                        else volts = rescale(stars[i].radius, 5.0, 12.0, 2.0, 0.0);
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

    void resizeConstellation() {
        // TODO: put limits on this
        Vec center = Vec(DISPLAY_SIZE/2.0, DISPLAY_SIZE/2.0);
        for (int i = 0; i < MAX_STARS; i++) {
            Vec pos = stars[i].box.getCenter();
            Vec mag = pos.minus(center);
            float maxDist = sqrt(mag.x * mag.x + mag.y * mag.y) * 0.5;
            mag = mag.normalize();
            float m = rescale(params[SIZE_PARAM].getValue(), 0, 1, maxDist, 0);
            mag = mag.mult(m);
            stars[i].posOffset = mag;
        }

        // return mag;
        // pos = pos.minus(mag);
        // stars[index].box.pos.x = pos.x;
        // stars[index].box.pos.y = pos.y;
    }

    bool isStarTrigger(int index) {
        if (!stars[index].alreadyTriggered) {
            Vec pos = stars[index].getPos();
            switch (currentSeqMode) {
                case PURPLE_SEQ:    return seqPos > pos.x ? true : false;
                case BLUE_SEQ:      return seqPos > pos.y ? true : false;
                case AQUA_SEQ:      return seqPos < pos.x ? true : false;
                case RED_SEQ:       return seqPos < pos.y ? true : false;
            }
        } else {
            return false;
        }
    }

    void addStar(Vec pos, int index) {
        visibleStars++;
        stars[index].setPos(pos);
        stars[index].radius = randRange(5, 10);
        stars[index].visible = true;
        stars[index].locked = false;
        stars[index].alreadyTriggered = pos.x < seqPos ? true : false;
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
    }

    void removeAllStars() {
        for (int i = 0; i < MAX_STARS; i++) {
            removeStar(i);
        }
    }
};

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
                    Vec starPos = module->stars[i].box.getCenter();
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

            if (!clickedOnStar) {
                if (module->visibleStars < MAX_STARS) {
                    module->addStar(inits, nextAvailableIndex);
                }
            }

        }

    }

    void onDragStart(const event::DragStart &e) override {
        dragX = APP->scene->rack->mousePos.x;
        dragY = APP->scene->rack->mousePos.y;
    }

    void onDragMove(const event::DragMove &e) override {
        float newDragX = APP->scene->rack->mousePos.x;
        float newDragY = APP->scene->rack->mousePos.y;

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
        //background
        nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

        if (module != NULL) {
            // draw stars
            for (int i = 0; i < MAX_STARS; i++) {
                if (module->stars[i].visible) {
                    Vec pos = module->stars[i].getPos();

                    if (module->stars[i].blipTrigger) {
                        // use current seq line color
                        nvgFillColor(args.vg, nvgTransRGBA(nvgRGB(213, 153, 255), module->stars[i].haloAlpha));
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
            // draw line
            nvgStrokeWidth(args.vg, 2.0);
            nvgStrokeColor(args.vg, nvgRGB(128, 0, 219));
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, module->seqPos, 0);
            nvgLineTo(args.vg, module->seqPos, box.size.y);
            nvgStroke(args.vg);
        }

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

        addParam(createParamCentered<DefaultButton>(Vec(26.4, 78.1), module, Cosmosis::PLAY_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(61.2, 78.1), module, Cosmosis::SPEED_PARAM));
        addParam(createParamCentered<BlueKnob>(Vec(130.7, 78.1), module, Cosmosis::SIZE_PARAM));
        addParam(createParamCentered<DefaultButton>(Vec(130.7, 201.8), module, Cosmosis::CLEAR_STARS_PARAM));

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

        addParam(createParamCentered<Jeremy_HSwitchBlue>(Vec(83.2, 122.8), module, Cosmosis::PITCH_PARAM));
        addParam(createParamCentered<BlueInvertKnob>(Vec(130.7, 121.9), module, Cosmosis::OCTAVE_PARAM));

        addOutput(createOutputCentered<PJ301MPort>(Vec(32.1, 343.2), module, Cosmosis::GATE_OUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(64.4, 343.2), module, Cosmosis::VOLT_OUT));
    }

    void appendContextMenu(Menu *menu) override {
        Cosmosis *module = dynamic_cast<Cosmosis*>(this->module);

        MenuLabel *spacerLabel = new MenuLabel();
        menu->addChild(spacerLabel);

        ChannelItem *channelItem = new ChannelItem;
        channelItem->text = "Polyphony channels";
        channelItem->rightText = string::f("%d", module->channels) + " " + RIGHT_ARROW;
        channelItem->module = module;
        menu->addChild(channelItem);
    }
};

Model *modelCosmosis = createModel<Cosmosis, CosmosisWidget>("Cosmosis");