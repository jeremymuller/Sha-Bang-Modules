#include "plugin.hpp"

struct Nodes : Module {
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

    Nodes() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override {
        // TODO
    }

};

struct NodesDisplay : Widget {
    Nodes *module;
    float currentX = 0;
    float currentY = 0;
    float posX = 0;
    float posY = 0;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;

    NodesDisplay() {}

    void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.consume(this);
			initX = e.pos.x;
			initY = e.pos.y;
            float d = dist(Vec(initX, initY), Vec(currentX, currentY));
            if (d < 12) {
                currentX = initX;
                currentY = initY;
            }
            // dragX = APP->scene->rack->mousePos.x;
            // dragY = APP->scene->rack->mousePos.y;
        }
    }

    void onDragStart(const event::DragStart &e) override {
        dragX = APP->scene->rack->mousePos.x;
        dragY = APP->scene->rack->mousePos.y;
    }

    void onDragMove(const event::DragMove &e) override {
        float newDragX = APP->scene->rack->mousePos.x;
        float newDragY = APP->scene->rack->mousePos.y;
        posX = initX + (newDragX - dragX);
        posY = initY + (newDragY - dragY);

        // float dist = std::sqrt(std::pow(posX-currentX, 2) + std::pow(posY-currentY, 2));
        float d = dist(Vec(posX, posY), Vec(currentX, currentY));
        if (d < 12) {
            currentX = posX;
            currentY = posY;
        }
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

        if (module == NULL) return;

        nvgStrokeColor(args.vg, nvgRGBA(128, 0, 219, 255));
        nvgStrokeWidth(args.vg, 2);
        nvgBeginPath(args.vg);
        nvgEllipse(args.vg, currentX, currentY, 11.5, 11.5);
        nvgStroke(args.vg);

        nvgFillColor(args.vg, nvgRGBA(128, 0, 219, 255));
        nvgBeginPath(args.vg);
        // nvgEllipse(args.vg, box.size.x / 2, box.size.y / 2, 8, 8);
        nvgEllipse(args.vg, currentX, currentY, 8, 8);
        nvgFill(args.vg);
    }

};

struct NodesWidget : ModuleWidget {
    NodesWidget(Nodes *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Nodes.svg")));

        NodesDisplay *display = new NodesDisplay();
        display->module = module;
        display->box.pos = Vec(66.5, 8.8);
        display->box.size = Vec(362, 362);
        addChild(display);

    }
};

Model *modelNodes = createModel<Nodes, NodesWidget>("Nodes");