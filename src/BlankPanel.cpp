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
    }

};

struct JeremyBlankPanelWidget : ModuleWidget {
    JeremyBlankPanelWidget(JeremyBlankPanel *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/JeremyBlankPanel.svg")));

        addChild(createWidget<JeremyScrew>(Vec(16, 2)));
        addChild(createWidget<JeremyScrew>(Vec(16, box.size.y - 14)));

        int randColor = static_cast<int>(random::uniform() * 4);
        switch(randColor) {
            case 0:
                addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(13.7, 187.8), module, JeremyBlankPanel::LEFTEYE));
                addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(19.8, 187.8), module, JeremyBlankPanel::RIGHTEYE));
                break;
            case 1:
                addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(13.7, 187.8), module, JeremyBlankPanel::LEFTEYE));
                addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(19.8, 187.8), module, JeremyBlankPanel::RIGHTEYE));
                break;
            case 2:
                addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(13.7, 187.8), module, JeremyBlankPanel::LEFTEYE));
                addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(19.8, 187.8), module, JeremyBlankPanel::RIGHTEYE));
                break;
            case 3:
                addChild(createLight<SmallLight<JeremyRedLight>>(Vec(13.7, 187.8), module, JeremyBlankPanel::LEFTEYE));
                addChild(createLight<SmallLight<JeremyRedLight>>(Vec(19.8, 187.8), module, JeremyBlankPanel::RIGHTEYE));
                break;
        }
    }
};

Model *modelJeremyBlankPanel = createModel<JeremyBlankPanel, JeremyBlankPanelWidget>("JeremyBlankPanel");
