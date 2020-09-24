#include "plugin.hpp"

#define DISPLAY_SIZE 378
#define MAX_STARS 25

struct Star {
    Rect box;
    NVGcolor color = nvgRGB(255, 255, 255);
    float radius;
    bool locked = true;
    bool visible = false;

    Star() {
        // box.pos.x = _x;
        // box.pos.y = _y;
        radius = randRange(5, 10);
    }

    void setPos(Vec pos) {
        box.pos.x = pos.x;
        box.pos.y = pos.y;
    }

};

struct Cosmosis : Module, Constellations {
    enum SeqIds {
        PURPLE_SEQ,
        BLUE_SEQ,
        AQUA_SEQ,
        RED_SEQ,
        NUM_SEQS
    };
    enum ParamIds {
        BPM_PARAM,
        PLAY_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        PAUSE_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger playTrig;
    Star stars[MAX_STARS];
    int visibleStars = 0;
    float seqPos = 0;
    bool isPlaying = false;


    Cosmosis() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BPM_PARAM, 15, 120, 30, "Tempo", " bpm");
        configParam(PLAY_PARAM, 0.0, 1.0, 0.0);

        for (int i = 0; i < 14; i++) {
            Vec pos = Vec(AQUARIUS[i].x, AQUARIUS[i].y);
            addStar(pos, i, AQUARIUS[i].r);
        }
    }

    void process(const ProcessArgs &args) override {
        // TODO
        if (playTrig.process(params[PLAY_PARAM].getValue())) {
            isPlaying = !isPlaying;
        }

        lights[PAUSE_LIGHT].setBrightness(isPlaying ? 1.0 : 0.0);

        if (isPlaying) {
            seqPos += 1.0 / 44100 * 60.0;
            if (seqPos > DISPLAY_SIZE) seqPos = 0;
        }

    }

    void addStar(Vec pos, int index) {
        visibleStars++;
        stars[index].setPos(pos);
        stars[index].radius = randRange(5, 10);
        stars[index].visible = true;
        stars[index].locked = false;
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
                    Vec pos = module->stars[i].box.getCenter();
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
        addParam(createParamCentered<BlueInvertKnob>(Vec(58.8, 78.1), module, Cosmosis::BPM_PARAM));
    }
};

Model *modelCosmosis = createModel<Cosmosis, CosmosisWidget>("Cosmosis");