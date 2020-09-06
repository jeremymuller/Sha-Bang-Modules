#include "plugin.hpp"

struct Node {
    Rect box;
    NVGcolor color;
    bool locked = true;
    bool visible = true;
    Node() {
        box.pos.x = 30;
        box.pos.y = 30;
    }
};

struct Neutrinode : Module {
    enum ParamIds {
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

    Node *nodes = new Node[4];

    Neutrinode() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        nodes[0].color = nvgRGBA(128, 0, 219, 255);
        nodes[1].color = nvgRGBA(38, 0, 255, 255);
        nodes[2].color = nvgRGBA(0, 238, 219, 255);
        nodes[3].color = nvgRGBA(255, 0, 0, 255);
        nodes[0].box.pos = Vec(30, 30);
        nodes[1].box.pos = Vec(50, 50);
        nodes[2].box.pos = Vec(70, 70);
        nodes[3].box.pos = Vec(90, 90);
    }

    ~Neutrinode() {
        delete[] nodes;
    }

    void process(const ProcessArgs &args) override {
        // TODO
    }

};

struct NeutrinodeDisplay : Widget {
    Neutrinode *module;
    float currentX = 0;
    float currentY = 0;
    float posX = 0;
    float posY = 0;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;

    NeutrinodeDisplay() {}

    void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.consume(this);
			initX = e.pos.x;
			initY = e.pos.y;
            Vec inits = Vec(initX, initY);
            for (int i = 0; i < 4; i++) {
                Vec nodePos = module->nodes[i].box.getCenter();
                float d = dist(inits, nodePos);
                if (d < 12) {
                    module->nodes[i].box.pos.x = initX;
                    module->nodes[i].box.pos.y = initY;
                    module->nodes[i].locked = false;
                } else {
                    module->nodes[i].locked = true;
                }
            }
            // dragX = APP->scene->rack->mousePos.x;
            // dragY = APP->scene->rack->mousePos.y;
        }
    }

    void onDragStart(const event::DragStart &e) override {
        // if (!module->nodes[0].locked) {
        //     module->nodes[0].box.pos.x = e.pos.x;
        //     module->nodes[0].box.pos.y = e.pos.y;
        // }
        dragX = APP->scene->rack->mousePos.x;
        dragY = APP->scene->rack->mousePos.y;
    }

    void onDragMove(const event::DragMove &e) override {
        float newDragX = APP->scene->rack->mousePos.x;
        float newDragY = APP->scene->rack->mousePos.y;

        for (int i = 0; i < 4; i++) {
            if (!module->nodes[i].locked) {
                module->nodes[i].box.pos.x = initX + (newDragX-dragX);
                module->nodes[i].box.pos.y = initY + (newDragY-dragY);
            }
        }
        // posX = initX + (newDragX - dragX);
        // posY = initY + (newDragY - dragY);

        // // float dist = std::sqrt(std::pow(posX-currentX, 2) + std::pow(posY-currentY, 2));
        // Vec inits = Vec(initX, initY);
        // Vec nodePos = module->nodes[0].box.getCenter();
        // float d = dist(inits, nodePos);
        // if (d < 12) {
        //     module->nodes[0].box.pos.x = initX;
        //     module->nodes[0].box.pos.y = initY;
        //     // currentX = posX;
        //     // currentY = posY;
        // }
    }

    void onDragEnd(const event::DragEnd &e) override {

    }

    float dist(Vec a, Vec b) {
        // returns distance between two points
        return std::sqrt(std::pow(a.x-b.x, 2) + std::pow(a.y-b.y, 2));
    }

    void draw(const DrawArgs &args) override {
        //background
        nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

        if (module != NULL) {
            for (int i = 0; i < 4; i++) {
                if (module->nodes[i].visible) {
                    nvgStrokeColor(args.vg, module->nodes[i].color);
                    nvgStrokeWidth(args.vg, 2);
                    nvgBeginPath(args.vg);
                    Vec pos = module->nodes[i].box.getCenter();
                    nvgCircle(args.vg, pos.x, pos.y, 11.5);
                    nvgStroke(args.vg);

                    nvgFillColor(args.vg, module->nodes[i].color);
                    nvgBeginPath(args.vg);
                    nvgCircle(args.vg, pos.x, pos.y, 8);
                    nvgFill(args.vg);
                }
            }

        }

    }

};

struct NeutrinodeWidget : ModuleWidget {
    NeutrinodeWidget(Neutrinode *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Neutrinode.svg")));

        NeutrinodeDisplay *display = new NeutrinodeDisplay();
        display->module = module;
        display->box.pos = Vec(125.4, 8.8);
        display->box.size = Vec(362, 362);
        addChild(display);

    }
};

Model *modelNeutrinode = createModel<Neutrinode, NeutrinodeWidget>("Neutrinode");