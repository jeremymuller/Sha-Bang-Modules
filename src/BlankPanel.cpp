#include "plugin.hpp"

float blinkPhase = 0.0;
float hz = 0.25;

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
        blinkPhase += args.sampleTime * hz;
        if (blinkPhase >= 1.0) {
            blinkPhase -= 1.0;
        }
        // if (blinkPhase >= 1.0 || blinkPhase <= 0) {
        //     // blinkPhase -= 1.0;
        //     hz *= -1;
        // }
        lights[LEFTEYE].setSmoothBrightness(blinkPhase < 0.5 ? 1.0 : 0.0, args.sampleTime * 0.25);
        lights[RIGHTEYE].setSmoothBrightness(blinkPhase < 0.5 ? 1.0 : 0.0, args.sampleTime * 0.25);
    }

};

struct JeremyBlankPanelWidget : ModuleWidget {
    JeremyBlankPanelWidget(JeremyBlankPanel *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/JeremyBlankPanel.svg")));

        addChild(createWidget<JeremyScrew>(Vec(24, 2)));
        addChild(createWidget<JeremyScrew>(Vec(24, box.size.y - 14)));

        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(19.8 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::LEFTEYE));
        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(28.1 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::RIGHTEYE));

        // Old stuff
        // int randColor = static_cast<int>(random::uniform() * 4);
        // switch(randColor) { // 1.606
        //     case 0:
        //         addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(19.8 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::LEFTEYE));
        //         addChild(createLight<SmallLight<JeremyPurpleLight>>(Vec(28.1 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::RIGHTEYE));
        //         break;
        //     case 1:
        //         addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(19.8 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::LEFTEYE));
        //         addChild(createLight<SmallLight<JeremyBlueLight>>(Vec(28.1 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::RIGHTEYE));
        //         break;
        //     case 2:
        //         addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(19.8 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::LEFTEYE));
        //         addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(28.1 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::RIGHTEYE));
        //         break;
        //     case 3:
        //         addChild(createLight<SmallLight<JeremyRedLight>>(Vec(19.8 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::LEFTEYE));
        //         addChild(createLight<SmallLight<JeremyRedLight>>(Vec(28.1 - 3.21, 180.9 - 3.21), module, JeremyBlankPanel::RIGHTEYE));
        //         break;
        // }
    }
};

Model *modelJeremyBlankPanel = createModel<JeremyBlankPanel, JeremyBlankPanelWidget>("JeremyBlankPanel");
