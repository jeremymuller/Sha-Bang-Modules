#include "Photron.hpp"

#define DISPLAY_SIZE_WIDTH 60
#define DISPLAY_SIZE_HEIGHT 380
#define CELL_SIZE 5 // 5? or 10?
#define NUM_OF_MARCHING_CIRCLES 5

struct PhotronPanel : Module {
    enum ParamIds {
        RANDOMIZE_PARAM,
        RESET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
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
    bool darkRoomBlobs = true;
    // int srIncrement = static_cast<int>(APP->engine->getSampleRate() / INTERNAL_HZ);
    int internalHz = 30;
    float srIncrement = APP->engine->getSampleTime() * internalHz;
    float sr = 0;
    float sep = 0.1;
    float ali = 0.0;
    float coh = 0.0;
    float cellScale = 1.0;
    static const int cols = DISPLAY_SIZE_WIDTH / CELL_SIZE;
    static const int rows = DISPLAY_SIZE_HEIGHT / CELL_SIZE;
    Block blocks[rows][cols];
    int blockAlpha[rows][cols];
    MarchingCircle circles[NUM_OF_MARCHING_CIRCLES];

    PhotronPanel() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Block b(x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE);
                blocks[y][x] = b;
                blocks[y][x].isSet = true;

                blockAlpha[y][x] = 0;
            }
        }

        for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            MarchingCircle c(randRange(DISPLAY_SIZE_WIDTH), randRange(DISPLAY_SIZE_HEIGHT), randRange(10.0, 35.0));
            c.setSize(DISPLAY_SIZE_WIDTH, DISPLAY_SIZE_HEIGHT);
            c.velLimit = 0.5;
            circles[i] = c;
        }

        resetBlocks(PhotronPanel::RESET_PARAM);
    }

    void onSampleRateChange() override {
        srIncrement = APP->engine->getSampleTime() * internalHz;
    }

    void onRandomize() override {
        resetBlocks(PhotronPanel::RANDOMIZE_PARAM);

        for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            circles[i].radius = randRange(10.0, 35.0);
        }
    }

    void onReset() override {
        resetBlocks(PhotronPanel::RESET_PARAM);
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
        json_object_set_new(rootJ, "blobs", json_boolean(darkRoomBlobs));
        json_object_set_new(rootJ, "blocks", blocksJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *internalHzJ = json_object_get(rootJ, "internalHz");
        if (internalHzJ) setHz(json_integer_value(internalHzJ));

        json_t *colorJ = json_object_get(rootJ, "color");
        if (colorJ) isColor = json_boolean_value(colorJ);

        json_t *blobsJ = json_object_get(rootJ, "blobs");
        if (blobsJ) darkRoomBlobs = json_boolean_value(blobsJ);

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

        if (sr == 0) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    blocks[y][x].sepInput = PhotronPanel::sep;
                    // blocks[y][x].aliInput = PhotronPanel::ali;
                    // blocks[y][x].cohInput = PhotronPanel::coh;

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

                    blockAlpha[y][x] = calculateCell(blocks[y][x].getCenter());
                }
            }

            for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
                circles[i].update();
            }
        }
        sr += srIncrement;
        if (sr >= 1.0) {
            sr = 0.0;
        }
    }

    void resetBlocks(int param) {
        if (param == PhotronPanel::RANDOMIZE_PARAM) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    blocks[y][x].reset();
                }
            }
        } else if (param == PhotronPanel::RESET_PARAM) {
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
    }

    int calculateCell(Vec blockPos) {
        float sum = 0.0;
        for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            float r = circles[i].radius * 0.9;
            float d = (blockPos.x - circles[i].pos.x) * (blockPos.x - circles[i].pos.x) + (blockPos.y - circles[i].pos.y) * (blockPos.y - circles[i].pos.y);
            d = std::fmax(d, 0.001); // to prevent dividing by zero
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

namespace PhotronPanelNS {
    struct HzModeValueItem : MenuItem {
        PhotronPanel *module;
        int hz;
        void onAction(const event::Action &e) override {
            module->setHz(hz);
        }
    };

    struct HzModeItem : MenuItem {
        PhotronPanel *module;
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

    struct ColorModeValueItem : MenuItem {
        PhotronPanel *module;
        bool isColor;
        void onAction(const event::Action &e) override {
            module->isColor = isColor;
        }
    };

    struct ColorModeItem : MenuItem {
        PhotronPanel *module;
        Menu *createChildMenu() override {
            Menu *menu = new Menu;
            std::vector<std::string> colorModes = {"color", "black & white"};
            for (int i = 0; i < 2; i++) {
                ColorModeValueItem *item = new ColorModeValueItem;
                item->text = colorModes[i];
                bool isColor = (i == 0) ? true : false;
                item->rightText = CHECKMARK(module->isColor == isColor);
                item->module = module;
                item->isColor = isColor;
                menu->addChild(item);
            }
            return menu;
        }
    };
}

struct PhotronPanelDisplay : Widget {
    PhotronPanel *module;

    void onButton(const event::Button &e) override {
        if (module == NULL) return;

        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if ((e.mods & RACK_MOD_MASK) == RACK_MOD_CTRL) {
                e.consume(this);
                module->isColor = !module->isColor;
            }
        }
    }

    void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        for (int y = 0; y <DISPLAY_SIZE_HEIGHT/CELL_SIZE; y++) {
            for (int x = 0; x < DISPLAY_SIZE_WIDTH/CELL_SIZE; x++) {
                Vec3 rgb = module->blocks[y][x].rgb;
                if (module->isColor) {
                    nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                } else {
                    // NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                    nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.x, rgb.x));
                }

                nvgBeginPath(args.vg);
                nvgRect(args.vg, module->blocks[y][x].pos.x, module->blocks[y][x].pos.y, CELL_SIZE, CELL_SIZE);
                nvgFill(args.vg);
            }
        }
    }

    void drawLayer(const DrawArgs &args, int layer) override {
        if (module == NULL) return;

        if (layer == 1) {
            //background
            // nvgFillColor(args.vg, nvgRGB(40, 40, 40));
            // nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            // nvgBeginPath(args.vg);
            // nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            // nvgFill(args.vg);

            for (int y = 0; y < DISPLAY_SIZE_HEIGHT/CELL_SIZE; y++) {
                for (int x = 0; x < DISPLAY_SIZE_WIDTH/CELL_SIZE; x++) {
                    Vec3 rgb = module->blocks[y][x].rgb;
                    bool isBlobs = module->darkRoomBlobs;
                    if (module->isColor) {
                        nvgFillColor(args.vg, nvgRGBA(rgb.x, rgb.y, rgb.z, isBlobs ? module->blockAlpha[y][x] : 255));
                    } else {
                        // NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                        // nvgFillColor(args.vg, nvgTransRGBA(color, rgb.y));
                        nvgFillColor(args.vg, nvgRGBA(rgb.x, rgb.x, rgb.x, isBlobs ? module->blockAlpha[y][x] : 255));
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
        Widget::drawLayer(args, layer);

    }
};

struct PhotronPanelWidget : ModuleWidget {
    PhotronPanelWidget(PhotronPanel *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PhotronPanel.svg")));

        PhotronPanelDisplay *display = new PhotronPanelDisplay();
        display->module = module;
        display->box.pos = Vec(0.0, 0.0);
        display->box.size = Vec(DISPLAY_SIZE_WIDTH, DISPLAY_SIZE_HEIGHT);
        addChild(display);

    }

    void appendContextMenu(Menu *menu) override {
        PhotronPanel *module = dynamic_cast<PhotronPanel*>(this->module);
        menu->addChild(new MenuEntry);

        PhotronPanelNS::HzModeItem *hzModeItem = new PhotronPanelNS::HzModeItem;
        hzModeItem->text = "Processing rate";
        hzModeItem->rightText = string::f("%d Hz ", module->internalHz) + RIGHT_ARROW; 
        hzModeItem->module = module;
        menu->addChild(hzModeItem);

        PhotronPanelNS::ColorModeItem *colorModeItem = new PhotronPanelNS::ColorModeItem;
        colorModeItem->text = "Mode";
        if (module->isColor) colorModeItem->rightText = std::string("color ") + " " + RIGHT_ARROW;
        else colorModeItem->rightText = std::string("black & white ") + " " + RIGHT_ARROW;
        colorModeItem->module = module;
        menu->addChild(colorModeItem);

        menu->addChild(createBoolPtrMenuItem("Dark Room Blobs", "", &module->darkRoomBlobs));
    }
};

Model *modelPhotronPanel = createModel<PhotronPanel, PhotronPanelWidget>("PhotronPanel");
