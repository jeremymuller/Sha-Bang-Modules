#include "Photron.hpp"

#define DISPLAY_SIZE_WIDTH 690
#define DISPLAY_SIZE_HEIGHT 380
#define CELL_SIZE 10

struct Photron : Module {
    enum ParamIds {
        COLOR_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
        SEPARATE_INPUT,
        ALIGN_INPUT,
        COHESION_INPUT,
        COLOR_TRIGGER_INPUT,
        INVERT_INPUT,
        RESET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    dsp::SchmittTrigger colorTrig, invertTrig, resetTrig;
    bool isColor = true;
    int resetIndex = 0;
    int checkParams = 0;
    // int srIncrement = static_cast<int>(APP->engine->getSampleRate() / INTERNAL_HZ);
    int internalHz = 60;
    float srIncrement = APP->engine->getSampleTime() * internalHz;
    float sr = 0;
    static const int cols = DISPLAY_SIZE_WIDTH / CELL_SIZE;
    static const int rows = DISPLAY_SIZE_HEIGHT / CELL_SIZE;
    Block blocks[rows][cols];

    Photron() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(COLOR_PARAM, 0.0, 1.0, 0.0, "color/black & white");

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Block b(x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE);
                blocks[y][x] = b;
                blocks[y][x].isSet = true;
            }
        }
    }

    void onSampleRateChange() override {
        srIncrement = APP->engine->getSampleTime() * internalHz;
    }

    void onRandomize() override {
        resetBlocks();
    }

    void onReset() override {
        resetBlocks();
    }

    void setHz(int hz) {
        internalHz = hz;
        srIncrement = APP->engine->getSampleTime() * internalHz;
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

        json_object_set_new(rootJ, "internalHz", json_integer(internalHz));
        json_object_set_new(rootJ, "color", json_boolean(isColor));
        json_object_set_new(rootJ, "blocks", blocksJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *internalHzJ = json_object_get(rootJ, "internalHz");
        if (internalHzJ) setHz(json_integer_value(internalHzJ));

        json_t *colorJ = json_object_get(rootJ, "color");
        if (colorJ) isColor = json_boolean_value(colorJ);

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
                        if (greenJ) blocks[y][x].rgb.y = json_integer_value(greenJ);
                        if (blueJ) blocks[y][x].rgb.z = json_integer_value(blueJ);
                    }
                }
            }
        }
    }

    void process(const ProcessArgs &args) override {
        if (checkParams == 0) {
            if (colorTrig.process(params[COLOR_PARAM].getValue() + inputs[COLOR_TRIGGER_INPUT].getVoltage())) {
                isColor = !isColor;
            }
            if (invertTrig.process(inputs[INVERT_INPUT].getVoltage())) {
                invertColors();
            }
            if (resetTrig.process(inputs[RESET_INPUT].getVoltage())) {
                resetBlocks();
            }
        }
        checkParams = (checkParams+1) % 4;

        if (sr == 0) {
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
                    if (x > 0) west = blocks[y][x-1];
                    if (x < cols-1) east = blocks[y][x+1];
                    if (y > 0) north = blocks[y-1][x];
                    if (y < rows-1) south = blocks[y+1][x];

                    // corners
                    Block northwest;
                    Block northeast;
                    Block southwest;
                    Block southeast;
                    if ((x > 0) && (y > 0)) northwest = blocks[y-1][x-1]; 
                    if ((x < cols-1) && (y > 0)) northeast = blocks[y-1][x+1];
                    if ((y < rows-1) && (x > 0)) southwest = blocks[y+1][x-1];
                    if ((y < rows-1) && (x < cols-1)) southeast = blocks[y+1][x+1];

                    Block b[8] = {west, east, north, south, northwest, northeast, southwest, southeast};
                    blocks[y][x].flock(b, 8);
                    blocks[y][x].update();
                }
            }
        }
        sr += srIncrement;
        if (sr >= 1.0) {
            sr = 0.0;
        }
    }

    void resetBlocks() {
        if (resetIndex == 0) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    blocks[y][x].reset();
                }
            }
        } else {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    if (x < cols/2.0) {
                        if (y < rows/2.0)
                            blocks[y][x].setColor(128, 0, 219); // purple
                        else
                            blocks[y][x].setColor(0, 238, 255); // aqua
                    }
                    else {
                        if (y < rows/2.0)
                            blocks[y][x].setColor(38, 0, 255); // blue
                        else
                            blocks[y][x].setColor(255, 0, 0); // red
                    }
                }
            }           
        }

        resetIndex = (resetIndex+1) % 2;

    }

    void invertColors() {
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Vec3 rgb = blocks[y][x].rgb;
                blocks[y][x].rgb = Vec3(255-rgb.x, 255-rgb.y, 255-rgb.z);
            }
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
}

struct PhotronDisplay : Widget {
    Photron *module;

    // void onButton(const event::Button &e) override {
    //     if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
    //         e.consume(this);
            
    //     }
    // }

    void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        //background
        // nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

        for (int y = 0; y < DISPLAY_SIZE_HEIGHT/CELL_SIZE; y++) {
            for (int x = 0; x < DISPLAY_SIZE_WIDTH/CELL_SIZE; x++) {
                Vec3 rgb = module->blocks[y][x].rgb;
                if (module->isColor) {
                    nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                } else {
                    NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                    nvgFillColor(args.vg, nvgTransRGBA(color, rgb.y));
                }
                nvgBeginPath(args.vg);
                nvgRect(args.vg, module->blocks[y][x].pos.x, module->blocks[y][x].pos.y, CELL_SIZE, CELL_SIZE);
                nvgFill(args.vg);
            }
        }
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

        addParam(createParamCentered<TinyBlueButton>(Vec(9.7, 316.2), module, Photron::COLOR_PARAM));
        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 333.8), module, Photron::COLOR_TRIGGER_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 352.1), module, Photron::INVERT_INPUT));
        addInput(createInputCentered<TinyPJ301M>(Vec(9.7, 370.3), module, Photron::RESET_INPUT));
    }

    void appendContextMenu(Menu *menu) override {
        Photron *module = dynamic_cast<Photron*>(this->module);
        menu->addChild(new MenuEntry);

        PhotronNS::HzModeItem *hzModeItem = new PhotronNS::HzModeItem;
        hzModeItem->text = "Processing rate";
        hzModeItem->rightText = string::f("%d Hz ", module->internalHz) + RIGHT_ARROW; 
        hzModeItem->module = module;
        menu->addChild(hzModeItem);
    }
};

Model *modelPhotron = createModel<Photron, PhotronWidget>("Photron");