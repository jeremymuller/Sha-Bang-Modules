#include "plugin.hpp"

struct AbsorptionSpectrum : Module {
    AbsorptionSpectrum() {
        config(0, 0, 0, 0);
    }
};

struct AbsorptionSpectrumWidget : ModuleWidget {
    AbsorptionSpectrumWidget(AbsorptionSpectrum *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AbsorptionSpectrum.svg")));
    }
};

Model *modelAbsorptionSpectrum = createModel<AbsorptionSpectrum, AbsorptionSpectrumWidget>("AbsorptionSpectrum");