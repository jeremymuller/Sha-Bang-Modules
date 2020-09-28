#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	p->addModel(modelStochSeq);
	p->addModel(modelStochSeq4);
	p->addModel(modelPolyrhythmClock);
	p->addModel(modelRandGates);
	p->addModel(modelNeutrinode);
	p->addModel(modelCosmosis);
	p->addModel(modelJeremyBlankPanel);
	p->addModel(modelNtest);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
