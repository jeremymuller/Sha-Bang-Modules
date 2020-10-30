#include "plugin.hpp"

#define DISPLAY_SIZE_WIDTH 427
#define DISPLAY_SIZE_HEIGHT 378
#define MAX_PARTICLES 16
#define INTERNAL_SAMP_TIME 60.0 // TODO: figure out 60Hz regardless of SR

struct Particle {
    Rect box;
    Vec vel;
    Vec acc;
    float mass;
    NVGcolor color = nvgRGB(255, 255, 255);
    float radius;
    bool visible;

    Particle() {
        box.pos.x = 100;
        box.pos.y = 100;
        Vec v = Vec(randRange(-1.0, 1.0), randRange(-1.0, 1.0));
        vel = v.normalize();
        acc = Vec(0.0, 0.0);
        radius = randRange(5, 12);
        mass = radius;
        visible = false;
    }

    void setPos(Vec _pos) {
        Vec v = Vec(randRange(-1.0, 1.0), randRange(-1.0, 1.0));
        vel = v.normalize();
        box.pos = _pos;
    }

    void applyForce(Vec force) {
        Vec f = force.div(mass);
        acc = acc.plus(f);
    }

    void update() {
        vel = vel.plus(acc);
        vel = limit(vel, 2.0);
        box.pos = box.pos.plus(vel);
        acc = acc.mult(0.0);
    }
};

struct Attractor {
    float mass;
    float G;
    Rect box;
    Vec vel;
    Vec acc;
    float radius = 15.5;
    NVGcolor color;
    bool locked = true;
    bool visible = true;


    Attractor() {
        box.pos.x = 30;
        box.pos.y = 30;

        mass = 15.5;
        G = 1.0;
    }

    Vec attract(Particle p) {
        Vec force = box.pos.minus(p.box.pos);
        float d = mag(force);

        d = clamp(d, 5.0, 25.0);

        force = force.normalize();

        float strength = (G * mass * p.mass) / (d * d);
        force = force.mult(strength);
        return force;
    }
};

struct Orbitones : Module {
    enum AttractorIds {
        PURPLE_ATTRACTOR,
        BLUE_ATTRACTOR,
        AQUA_ATTRACTOR,
        RED_ATTRACTOR,
        NUM_ATTRACTORS
    };
    enum ParamIds {
        REMOVE_PARTICLE_PARAM,
        CLEAR_PARTICLES_PARAM,
        ON_PARAMS = CLEAR_PARTICLES_PARAM + NUM_ATTRACTORS,
        GRAVITY_PARAMS = ON_PARAMS + NUM_ATTRACTORS,
		NUM_PARAMS = GRAVITY_PARAMS + NUM_ATTRACTORS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
        X_POLY_OUTPUT,
        Y_POLY_OUTPUT,
        NEG_X_POLY_OUTPUT,
        NEG_Y_POLY_OUTPUT,
        VEL_X_POLY_OUTPUT,
        VEL_Y_POLY_OUTPUT,
        AVG_X_OUTPUT,
        AVG_Y_OUTPUT,
        RND_X_OUTPUT,
        RND_Y_OUTPUT,
        MAX_X_OUTPUT,
        MAX_Y_OUTPUT,
        MIN_X_OUTPUT,
        MIN_Y_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    dsp::SchmittTrigger removeTrig, clearTrig;
    Attractor *attractors = new Attractor[NUM_ATTRACTORS];
    Particle *particles = new Particle[MAX_PARTICLES];
    int processOrbits = 0;
    int visibleParticles = 2;
    int currentParticle = 0;
    int channels = 1;

    Orbitones() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(REMOVE_PARTICLE_PARAM, 0.0, 1.0, 0.0, "Remove previous particle");
        configParam(ON_PARAMS + PURPLE_ATTRACTOR, 0.0, 1.0, 0.0, "toggle purple attractor");
        configParam(ON_PARAMS + BLUE_ATTRACTOR, 0.0, 1.0, 0.0, "toggle blue attractor");
        configParam(ON_PARAMS + AQUA_ATTRACTOR, 0.0, 1.0, 0.0, "toggle aqua attractor");
        configParam(ON_PARAMS + RED_ATTRACTOR, 0.0, 1.0, 0.0, "toggle red attractor");
        configParam(GRAVITY_PARAMS + PURPLE_ATTRACTOR, -1.0, 2.0, 1.0, "Purple attractor gravity");
        configParam(GRAVITY_PARAMS + BLUE_ATTRACTOR, -1.0, 2.0, 1.0, "Blue attractor gravity");
        configParam(GRAVITY_PARAMS + AQUA_ATTRACTOR, -1.0, 2.0, 1.0, "Aqua attractor gravity");
        configParam(GRAVITY_PARAMS + RED_ATTRACTOR, -1.0, 2.0, 1.0, "Red attractor gravity");

        attractors[0].color = nvgRGBA(128, 0, 219, 255);
        attractors[1].color = nvgRGBA(38, 0, 255, 255);
        attractors[2].color = nvgRGBA(0, 238, 219, 255);
        attractors[3].color = nvgRGBA(255, 0, 0, 255);
        attractors[0].box.pos = Vec(randRange(16, DISPLAY_SIZE_WIDTH / 2.0 - 16), randRange(16, DISPLAY_SIZE_HEIGHT / 2.0 - 16));
        attractors[1].box.pos = Vec(randRange(DISPLAY_SIZE_WIDTH / 2.0 + 16, DISPLAY_SIZE_WIDTH - 16), randRange(16, DISPLAY_SIZE_HEIGHT / 2.0 - 16));
        attractors[2].box.pos = Vec(randRange(DISPLAY_SIZE_WIDTH / 2.0 + 16, DISPLAY_SIZE_WIDTH / 2.0 - 16), randRange(DISPLAY_SIZE_HEIGHT / 2.0 + 16, DISPLAY_SIZE_HEIGHT / 2.0 - 16));
        attractors[3].box.pos = Vec(randRange(16, DISPLAY_SIZE_WIDTH / 2.0 - 16), randRange(DISPLAY_SIZE_HEIGHT / 2.0 + 16, DISPLAY_SIZE_HEIGHT - 16));

        particles[0].visible = true;
        particles[1].visible = true;
    }

    ~Orbitones() {
        delete[] attractors;
        delete[] particles;
    }

    void process(const ProcessArgs &args) override {
        // TODO
        if (processOrbits == 0) {
            if (removeTrig.process(params[REMOVE_PARTICLE_PARAM].getValue())) {
                if (visibleParticles > 0) removeParticle(visibleParticles-1);
            }
            if (clearTrig.process(params[CLEAR_PARTICLES_PARAM].getValue())) {
                if (visibleParticles > 0) clearParticles();
            }

            if (outputs[RND_X_OUTPUT].isConnected() || outputs[RND_Y_OUTPUT].isConnected()) {
                currentParticle = static_cast<int>(random::uniform() * visibleParticles);
            }

            for (int i = 0; i < NUM_ATTRACTORS; i++) {
                attractors[i].G = params[GRAVITY_PARAMS + i].getValue();
            }

            outputs[X_POLY_OUTPUT].setChannels(channels);
            outputs[Y_POLY_OUTPUT].setChannels(channels);
            outputs[NEG_X_POLY_OUTPUT].setChannels(channels);
            outputs[NEG_Y_POLY_OUTPUT].setChannels(channels);
            outputs[VEL_X_POLY_OUTPUT].setChannels(channels);
            outputs[VEL_Y_POLY_OUTPUT].setChannels(channels);

            float scl = 1.0 / visibleParticles;
            float currentAvgX = 0.0;
            float currentAvgY = 0.0;
            float maxX = -5.0;
            float maxY = -5.0;
            float minX = 5.0;
            float minY = 5.0;

            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (particles[i].visible) {
                    for (int j = 0; j < NUM_ATTRACTORS; j++) {
                        if (attractors[j].visible) {
                            Vec force = attractors[j].attract(particles[i]);
                            particles[i].applyForce(force);
                            particles[i].update();
                        }
                    }
                    float voltsX = rescale(particles[i].box.pos.x, 0, DISPLAY_SIZE_WIDTH, -5.0, 5.0);
                    float voltsY = rescale(particles[i].box.pos.y, DISPLAY_SIZE_HEIGHT, 0, -5.0, 5.0);
                    float voltsVelX = rescale(particles[i].vel.x, -2.0, 2.0, -5.0, 5.0);
                    float voltsVelY = rescale(particles[i].vel.y, 2.0, -2.0, -5.0, 5.0);
                    outputs[X_POLY_OUTPUT].setVoltage(voltsX, i);
                    outputs[Y_POLY_OUTPUT].setVoltage(voltsY, i);
                    outputs[NEG_X_POLY_OUTPUT].setVoltage(-voltsX, i);
                    outputs[NEG_Y_POLY_OUTPUT].setVoltage(-voltsY, i);
                    outputs[VEL_X_POLY_OUTPUT].setVoltage(voltsVelX, i);
                    outputs[VEL_Y_POLY_OUTPUT].setVoltage(voltsVelY, i);
                    currentAvgX += voltsX * scl;
                    currentAvgY += voltsY * scl;
                    maxX = std::max(maxX, voltsX);
                    maxY = std::max(maxY, voltsY);
                    minX = std::min(minX, voltsX);
                    minY = std::min(minY, voltsY);
                }
            }
            outputs[MAX_X_OUTPUT].setVoltage(maxX);
            outputs[MAX_Y_OUTPUT].setVoltage(maxY);
            outputs[MIN_X_OUTPUT].setVoltage(minX);
            outputs[MIN_Y_OUTPUT].setVoltage(minY);
            outputs[AVG_X_OUTPUT].setVoltage(currentAvgX);
            outputs[AVG_Y_OUTPUT].setVoltage(currentAvgY);

            // for (int i = 0; i < NUM_ATTRACTORS; i++) {
            //     if (attractors[i].visible) {
            //         for (int j = 0; j < MAX_PARTICLES; j++) {
            //             if (particles[j].visible) {
            //                 Vec force = attractors[i].attract(particles[j]);
            //                 particles[j].applyForce(force);
            //                 particles[j].update();
            //             }
            //             float margin = 7.0;
            //             float voltsX = rescale(particles[j].box.pos.x, 0, DISPLAY_SIZE_WIDTH, -5.0, 5.0);

            //             float voltsY = rescale(particles[j].box.pos.y, DISPLAY_SIZE_HEIGHT, 0, -5.0, 5.0);
            //             outputs[X_POLY_OUTPUT].setVoltage(voltsX, j);
            //             outputs[Y_POLY_OUTPUT].setVoltage(voltsY, j);
            //             outputs[NEG_X_POLY_OUTPUT].setVoltage(-voltsX, j);
            //             outputs[NEG_Y_POLY_OUTPUT].setVoltage(-voltsY, j);
            //         }
            //     }
            // }
        }
        processOrbits = (processOrbits + 1) % static_cast<int>(args.sampleRate / INTERNAL_SAMP_TIME); // check 60 hz;
    }

    void addParticle(Vec pos, int index) {
        visibleParticles++;
        particles[index].setPos(pos);
        particles[index].radius = randRange(5, 12);
        particles[index].mass = particles[index].radius;
        particles[index].visible = true;
    }

    void removeParticle(int index) {
        visibleParticles--;
        particles[index].box.pos = Vec(0, 0);
        particles[index].vel = Vec(0, 0);
        particles[index].visible = false;
    }

    void clearParticles() {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            particles[i].box.pos = Vec(0, 0);
            particles[i].vel = Vec(0, 0);
            particles[i].visible = false;
        }
        visibleParticles = 0;
    }
};

namespace OrbitonesNS {
    struct ChannelValueItem : MenuItem {
        Orbitones *module;
        int channels;
        void onAction(const event::Action &e) override {
            module->channels = channels;
        }
    };

    struct ChannelItem : MenuItem {
        Orbitones *module;
        Menu *createChildMenu() override {
            Menu *menu = new Menu;
            for (int channels = 1; channels <= 16; channels++) {
                ChannelValueItem *item = new ChannelValueItem;
                if (channels == 1)
                    item->text = "Monophonic";
                else
                    item->text = string::f("%d", channels);
                item->rightText = CHECKMARK(module->channels == channels);
                item->module = module;
                item->channels = channels;
                menu->addChild(item);
            }
            return menu;
        }
    };
}

struct OrbitonesDisplay : Widget {
    Orbitones *module;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;

    OrbitonesDisplay() {}

    void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            e.consume(this);
            initX = e.pos.x;
            initY = e.pos.y;
            Vec inits = Vec(initX, initY);
            bool clickedOnObj = false;
            for (int i = 0; i < Orbitones::NUM_ATTRACTORS; i++) {
                if (module->attractors[i].visible) {
                    Vec attPos = module->attractors[i].box.getCenter();
                    float d = dist(inits, attPos);
                    if (d < 16 && !clickedOnObj) {
                        module->attractors[i].box.pos.x = initX;
                        module->attractors[i].box.pos.y = initY;
                        module->attractors[i].locked = false;
                        clickedOnObj = true;
                    } else {
                        module->attractors[i].locked = true;
                    }
                }
            }

            if (!clickedOnObj && (module->visibleParticles < MAX_PARTICLES)) {
                module->addParticle(inits, module->visibleParticles);
            }
        }
    }

    void onDragStart(const event::DragStart &e) override {
        dragX = APP->scene->rack->mousePos.x;
        dragY = APP->scene->rack->mousePos.y;
    }

    void onDragMove(const event::DragMove &e) override {
        float newDragX = APP->scene->rack->mousePos.x;
        float newDragY = APP->scene->rack->mousePos.y;

        for (int i = 0; i < Orbitones::NUM_ATTRACTORS; i++) {
            if (!module->attractors[i].locked) {
                module->attractors[i].box.pos.x = initX + (newDragX - dragX);
                module->attractors[i].box.pos.y = initY + (newDragY - dragY);
                checkEdges(i);
            }
        }
    }

    void checkEdges(int index) {
        if (module != NULL) {
            // x's
            if (module->attractors[index].box.pos.x < 16) {
                module->attractors[index].box.pos.x = 16;
                module->attractors[index].vel.x *= -1;
            } else if (module->attractors[index].box.pos.x > box.size.x-16) {
                module->attractors[index].box.pos.x = box.size.x-16;
                module->attractors[index].vel.x *= -1;
            }
            // y's
            if (module->attractors[index].box.pos.y < 16) {
                module->attractors[index].box.pos.y = 16;
                module->attractors[index].vel.y *= -1;
            } else if (module->attractors[index].box.pos.y > box.size.y - 16) {
                module->attractors[index].box.pos.y = box.size.y - 16;
                module->attractors[index].vel.y *= -1;
            }
        }
    }

    void draw(const DrawArgs &args) override {
        // background
        nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

        if (module == NULL) return;

        for (int i = 0; i < Orbitones::NUM_ATTRACTORS; i++) {
            if (module->attractors[i].visible) {
                // display attractors
                Vec pos = module->attractors[i].box.getCenter();
                nvgStrokeColor(args.vg, module->attractors[i].color);
                nvgStrokeWidth(args.vg, 2);
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, module->attractors[i].radius);
                nvgStroke(args.vg);

                nvgFillColor(args.vg, module->attractors[i].color);
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, module->attractors[i].radius - 3.5);
                nvgFill(args.vg);
            }
        }
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (module->particles[i].visible && module->particles[i].box.pos.x > 0) {
                Vec pos = module->particles[i].box.getCenter();
                nvgFillColor(args.vg, nvgTransRGBA(module->particles[i].color, 90));
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, module->particles[i].radius);
                nvgFill(args.vg);

                nvgFillColor(args.vg, module->particles[i].color);
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, 2.5);
                nvgFill(args.vg);
            }
        }
    }
};

struct OrbitonesWidget : ModuleWidget {
    OrbitonesWidget(Orbitones *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Orbitones.svg")));

        OrbitonesDisplay *display = new OrbitonesDisplay();
        display->module = module;
        display->box.pos = Vec(55.9, 0.8);
        display->box.size = Vec(DISPLAY_SIZE_WIDTH, DISPLAY_SIZE_HEIGHT);
        addChild(display);

        // screws
        addChild(createWidget<JeremyScrew>(Vec(22, 2)));
        addChild(createWidget<JeremyScrew>(Vec(22, box.size.y - 14)));
        addChild(createWidget<JeremyScrew>(Vec(505.5, 2)));
        addChild(createWidget<JeremyScrew>(Vec(505.5, box.size.y - 14)));

        addParam(createParamCentered<TinyPurpleButton>(Vec(42, 56), module, Orbitones::REMOVE_PARTICLE_PARAM));
        addParam(createParamCentered<TinyPurpleButton>(Vec(42, 93.4), module, Orbitones::CLEAR_PARTICLES_PARAM));

        // on/off
        addParam(createParamCentered<TinyPurpleButton>(Vec(28, 196.8), module, Orbitones::ON_PARAMS + Orbitones::PURPLE_ATTRACTOR));
        addParam(createParamCentered<TinyBlueButton>(Vec(44.9, 213.7), module, Orbitones::ON_PARAMS + Orbitones::BLUE_ATTRACTOR));
        addParam(createParamCentered<TinyAquaButton>(Vec(11, 213.7), module, Orbitones::ON_PARAMS + Orbitones::AQUA_ATTRACTOR));
        addParam(createParamCentered<TinyRedButton>(Vec(28, 230.6), module, Orbitones::ON_PARAMS + Orbitones::RED_ATTRACTOR));

        // gravities
        addParam(createParamCentered<TinyPurpleKnob>(Vec(16, 250.4), module, Orbitones::GRAVITY_PARAMS + Orbitones::PURPLE_ATTRACTOR));
        addParam(createParamCentered<TinyBlueKnob>(Vec(39.9, 250.4), module, Orbitones::GRAVITY_PARAMS + Orbitones::BLUE_ATTRACTOR));
        addParam(createParamCentered<TinyAquaKnob>(Vec(16, 274.3), module, Orbitones::GRAVITY_PARAMS + Orbitones::AQUA_ATTRACTOR));
        addParam(createParamCentered<TinyRedKnob>(Vec(39.9, 274.3), module, Orbitones::GRAVITY_PARAMS + Orbitones::RED_ATTRACTOR));

        // poly outs
        addOutput(createOutputCentered<PJ301MPort>(Vec(498.1, 266.8), module, Orbitones::X_POLY_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(524.8, 266.8), module, Orbitones::Y_POLY_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(498.1, 306), module, Orbitones::NEG_X_POLY_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(524.8, 306), module, Orbitones::NEG_Y_POLY_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(498.1, 345.2), module, Orbitones::VEL_X_POLY_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(524.8, 345.2), module, Orbitones::VEL_Y_POLY_OUTPUT));

        // mono outs
        addOutput(createOutputCentered<TinyPJ301M>(Vec(499.1, 90.5), module, Orbitones::AVG_X_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(523.8, 90.5), module, Orbitones::AVG_Y_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(499.1, 122.5), module, Orbitones::MAX_X_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(523.8, 122.5), module, Orbitones::MAX_Y_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(499.1, 154.5), module, Orbitones::MIN_X_OUTPUT));
        addOutput(createOutputCentered<TinyPJ301M>(Vec(523.8, 154.5), module, Orbitones::MIN_Y_OUTPUT));
    }

    void appendContextMenu(Menu *menu) override {
        Orbitones *module = dynamic_cast<Orbitones*>(this->module);

        menu->addChild(new MenuEntry);

        OrbitonesNS::ChannelItem *channelItem = new OrbitonesNS::ChannelItem;
        channelItem->text = "Polyphony channels";
        channelItem->rightText = string::f("%d", module->channels) + " " + RIGHT_ARROW;
        channelItem->module = module;
        menu->addChild(channelItem);
    }
};

Model *modelOrbitones = createModel<Orbitones, OrbitonesWidget>("Orbitones");