#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	p->addModel(modelStochSeq);
	p->addModel(modelStochSeq4);
	p->addModel(modelPolyrhythmClock);
	p->addModel(modelRandGates);
	p->addModel(modelRandRoute);
	p->addModel(modelNeutrinode);
	p->addModel(modelCosmosis);
	p->addModel(modelJeremyBlankPanel);
	p->addModel(modelQubitCrusher);
	p->addModel(modelPhotron);
    p->addModel(modelPhotronPanel);
    p->addModel(modelPhotronStrip);
    p->addModel(modelOrbitones);
	p->addModel(modelAbsorptionSpectrum);
	p->addModel(modelTalea);
	p->addModel(modelCollider);
	p->addModel(modelStochSeq4X);
	p->addModel(modelStochSeqGrid);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
