#include "plugin.hpp"

struct AbsorptionSpectrum : Module {
    AbsorptionSpectrum() {
        config(0, 0, 0, 0);
    }
};

struct AbsorptionSpectrumWidget : ModuleWidget {
    AbsorptionSpectrumWidget(AbsorptionSpectrum *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/AbsorptionSpectrum.svg"), asset::plugin(pluginInstance, "res/AbsorptionSpectrum-dark.svg")));
    }
};

Model *modelAbsorptionSpectrum = createModel<AbsorptionSpectrum, AbsorptionSpectrumWidget>("AbsorptionSpectrum");