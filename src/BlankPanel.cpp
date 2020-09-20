#include "plugin.hpp"

struct JeremyBlankPanel : Module {
    enum LightIds {
        LEFTEYE,
        RIGHTEYE,
        NUM_LIGHTS
    };

    JeremyBlankPanel() {
        config(0, 0, 0, NUM_LIGHTS);
    }

    void process(const ProcessArgs &args) override {
        // TODO
        lights[LEFTEYE].setBrightness(1.0);
        lights[RIGHTEYE].setBrightness(1.0);
    }

};

struct JeremyBlankPanelWidget : ModuleWidget {
    JeremyBlankPanelWidget(JeremyBlankPanel *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/JeremyBlankPanel.svg")));

        addChild(createWidget<JeremyScrew>(Vec(24, 2)));
        addChild(createWidget<JeremyScrew>(Vec(24, box.size.y - 14)));

        int randColor = static_cast<int>(random::uniform() * 4);
        switch(randColor) {
            case 0:
                addChild(createLight<TinyLight<JeremyPurpleLight>>(Vec(19.8 - 1.606, 180.9 - 1.606), module, JeremyBlankPanel::LEFTEYE));
                addChild(createLight<TinyLight<JeremyPurpleLight>>(Vec(28.1 - 1.606, 180.9 - 1.606), module, JeremyBlankPanel::RIGHTEYE));
                break;
            case 1:
                addChild(createLight<TinyLight<JeremyBlueLight>>(Vec(19.8 - 1.606, 180.9 - 1.606), module, JeremyBlankPanel::LEFTEYE));
                addChild(createLight<TinyLight<JeremyBlueLight>>(Vec(28.1 - 1.606, 180.9 - 1.606), module, JeremyBlankPanel::RIGHTEYE));
                break;
            case 2:
                addChild(createLight<TinyLight<JeremyAquaLight>>(Vec(19.8 - 1.606, 180.9 - 1.606), module, JeremyBlankPanel::LEFTEYE));
                addChild(createLight<TinyLight<JeremyAquaLight>>(Vec(28.1 - 1.606, 180.9 - 1.606), module, JeremyBlankPanel::RIGHTEYE));
                break;
            case 3:
                addChild(createLight<TinyLight<JeremyRedLight>>(Vec(19.8 - 1.606, 180.9 - 1.606), module, JeremyBlankPanel::LEFTEYE));
                addChild(createLight<TinyLight<JeremyRedLight>>(Vec(28.1 - 1.606, 180.9 - 1.606), module, JeremyBlankPanel::RIGHTEYE));
                break;
        }
    }
};

Model *modelJeremyBlankPanel = createModel<JeremyBlankPanel, JeremyBlankPanelWidget>("JeremyBlankPanel");
