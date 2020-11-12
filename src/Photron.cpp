#include "plugin.hpp"

#define DISPLAY_SIZE_WIDTH 690
#define DISPLAY_SIZE_HEIGHT 380
#define CELL_SIZE 10
#define INTERNAL_HZ 60

struct Vec3 : Vec {
    // allows for 3d vectors and math
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;

    Vec3() {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    // TODO math when needed
    Vec3 plus(Vec3 b) const {
        return Vec3(x+b.x, y+b.y, z+b.z);
    }

    Vec3 minus(Vec3 b) const {
        return Vec3(x-b.x, y-b.y, z-b.z);
    }

    Vec3 mult(float s) const {
        return Vec3(x*s, y*s, z*s);
    }

    Vec3 div(float s) const {
        return Vec3(x/s, y/s, z/s);
    }

    Vec3 limit(float max) const {
        if (magSq() > max*max) {
            Vec3 n = normalize();
            return n.mult(max);
        }
        return Vec3(x, y, z);
    }

    float mag() const {
        return sqrt(x*x + y*y + z*z);
    }

    float magSq() const {
        return (x*x + y*y + z*z);
    }

    Vec3 normalize() const {
        float m = mag();
        if (m > 0) 
            return Vec3(x/m, y/m, z/m);
        else
            return Vec3(x, y, z);
    }

    Vec3 setMag(float len) const {
        Vec3 norm = normalize();
        norm = norm.mult(len);
        return norm;
    }

};

struct Block {
    bool isSet = false;
    Vec pos;
    Vec3 rgb;
    Vec3 rgbVel = Vec3();
    Vec3 rgbAcc = Vec3(); 

    float size;
    float maxspeed;
    float maxforce;
    float radius;

    float sepInput = 0.0;
    float aliInput = 0.0;
    float cohInput = 0.0;

    Block() {}

    Block(float x, float y, float s) {
        isSet = true;
        pos = Vec(x, y);

        int r = floor(randRange(256));
        int g = floor(randRange(256));
        int b = floor(randRange(256));

        rgb = Vec3(r, g, b);
        size = s;

        maxspeed = 1;
        maxforce = 0.01;
        radius = 15;
    }

    void reset() {
        int r = floor(randRange(256));
        int g = floor(randRange(256));
        int b = floor(randRange(256));

        rgb = Vec3(r, g, b);
    }

    void setColor(int r, int g, int b) {
        rgb = Vec3(r, g, b);
    }

    void flock(Block blocks[], int size) {
        Vec3 sep = separate(blocks, size);
        Vec3 ali = align(blocks, size);
        Vec3 coh = cohesion(blocks, size);

        sep = sep.mult(1.1 + sepInput);
        ali = ali.mult(1.0 + aliInput);
        coh = coh.mult(1.8 + cohInput);

        applyForce(sep);
        applyForce(ali);
        applyForce(coh);
    }

    Vec3 seek(Vec3 target) {
        Vec3 desired = target.minus(rgb);
        desired = desired.setMag(maxspeed);
        Vec3 steer = desired.minus(rgbVel);
        steer = steer.limit(maxforce);
        return steer;
    }

    Vec3 separate(Block blocks[], int size) {
        Vec3 sum = Vec3();
        int count = 0;
        Vec3 steer = Vec3();

        for (int i = 0; i < size; i++) {
            if (blocks[i].isSet) {
                Vec3 diff = rgb.minus(blocks[i].rgb);
                diff = diff.normalize();
                sum = sum.plus(diff);
                count++;
            }
        }

        if (count > 0) {
            sum = sum.div(count);
            sum = sum.normalize();
            sum = sum.mult(maxspeed);

            steer = sum.minus(rgbVel);
            steer = steer.limit(maxforce);
        }
        return steer;
    }

    Vec3 align(Block blocks[], int size) {
        Vec3 sum = Vec3();
        int count = 0;
        
        for (int i = 0; i < size; i++) {
            if (blocks[i].isSet) {
                sum = sum.plus(blocks[i].rgbVel);
                count++;
            }
        }

        if (count > 0) {
            sum = sum.div((float)count);
            sum = sum.normalize();
            sum = sum.mult(maxspeed);
            Vec3 steer = sum.minus(rgbVel);
            steer = steer.limit(maxforce);
            return steer;
        } else {
            return Vec3();
        }
    }

    Vec3 cohesion(Block blocks[], int size) {
        Vec3 sum = Vec3();
        int count = 0;

        for (int i = 0; i < size; i++) {
            if (blocks[i].isSet) {
                sum = sum.plus(blocks[i].rgb);
                count++;
            }
        }

        if (count > 0) {
            sum = sum.div((float)count);
            return seek(sum);
        } else {
            return Vec3();
        }
    }

    void update() {
        rgbVel = rgbVel.plus(rgbAcc);
        rgbVel = rgbVel.limit(maxspeed);
        rgb = rgb.plus(rgbVel);

        edges();

        rgbAcc = rgbAcc.mult(0.0); // reset acceleration
    }

    void applyForce(Vec3 force) {
        rgbAcc = rgbAcc.plus(force);
    }

    void edges() {
        if (rgb.x > 255) {
            rgb.x = 255;
            rgbVel.x *= -1;
        } else if (rgb.x < 0) {
            rgb.x = 0;
            rgbVel.x *= -1;
        }

        if (rgb.y > 255) {
            rgb.y = 255;
            rgbVel.y *= -1;
        } else if (rgb.y < 0) {
            rgb.y = 0;
            rgbVel.y *= -1;
        }

        if (rgb.z > 255) {
            rgb.z = 255;
            rgbVel.z *= -1;
        } else if (rgb.z < 0) {
            rgb.z = 0;
            rgbVel.z *= -1;
        }
    }
};

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
        configParam(COLOR_PARAM, 0.0, 1.0, 0.0, "color or b&w");

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
        hzModeItem->rightText = string::f("%d", module->internalHz) + " Hz " + RIGHT_ARROW; 
        hzModeItem->module = module;
        menu->addChild(hzModeItem);
    }
};

Model *modelPhotron = createModel<Photron, PhotronWidget>("Photron");