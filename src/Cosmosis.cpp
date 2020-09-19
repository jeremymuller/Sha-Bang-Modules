#include "plugin.hpp"

#define DISPLAY_SIZE 378

struct Cosmosis : Module {
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

    Cosmosis() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override {
        // TODO
    }
};

struct CosmosisDisplay : Widget {
    Cosmosis *module;

    CosmosisDisplay() {}

    void draw(const DrawArgs &args) override {

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
    }
};

Model *modelCosmosis = createModel<Cosmosis, CosmosisWidget>("Cosmosis");