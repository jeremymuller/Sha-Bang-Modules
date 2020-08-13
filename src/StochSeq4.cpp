#include "plugin.hpp"

struct StochSeq4 : Module {

};

struct StochSeq4Widget : ModuleWidget {
    StochSeq4Widget(StochSeq4 *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/StochSeq4.svg")));
    }
};