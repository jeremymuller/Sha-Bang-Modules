#include "plugin.hpp"

#define DISPLAY_SIZE_WIDTH 427
#define DISPLAY_SIZE_HEIGHT 378
#define MAX_PARTICLES 16
#define INTERNAL_SAMP_TIME 60.0

struct Trail {
    float x;
    float y;
    int alpha;
    Trail() {}
    Trail(float _x, float _y, int _alpha) {
        x = _x;
        y = _y;
        alpha = _alpha;
    }
};

struct Particle {
    Rect box;
    Vec vel;
    Vec acc;
    float mass;
    NVGcolor color = nvgRGB(255, 255, 255);
    NVGcolor trailColor;
    float radius;
    bool visible;
    bool whiteTrails = true;
    std::vector<Trail> history;

    Particle() {
        box.pos.x = randRange(0, DISPLAY_SIZE_WIDTH);
        box.pos.y = randRange(0, DISPLAY_SIZE_HEIGHT);
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
        box.pos = box.pos.plus(vel);
        vel = limit(vel, 12.0);
        acc = acc.mult(0.0);

        if (whiteTrails) {
            trailColor = nvgRGB(255, 255, 255);
        } else {
            NVGcolor red = nvgRGB(255, 0, 0);
            NVGcolor blue = nvgRGB(0, 0, 255);
            float _u = rescale(mag(vel), 0.0, 12.0, 0.0, 1.0);
            trailColor = nvgLerpRGBA(red, blue, _u);
        }
    }

    bool updateHistory() {
        // trail
        Vec v = box.getCenter();
        Trail t = Trail(v.x, v.y, 255);
        history.push_back(t);
        if (history.size() > 20) {
            history.erase(history.begin());
        }
        return true;
    }

    void clearHistory() {
        history.clear();
    }
};

struct Attractor {
    float mass;
    float G;
    float gravityScl;
    Rect box;
    Vec vel;
    Vec acc;
    float radius = 15.5;
    NVGcolor color;
    bool locked = true;
    bool visible = true;
    dsp::SchmittTrigger toggleTrig;

    Attractor() {
        box.pos.x = 30;
        box.pos.y = 30;

        mass = 15.5;
        G = 30.0;
        gravityScl = 1.0;
    }

    Vec attract(Particle p) {
        Vec force = box.pos.minus(p.box.pos);
        float d = mag(force);

        d = clamp(d, 10.0, 50.0);

        force = force.normalize();

        float strength = (G * gravityScl) * (mass * p.mass) / (d * d);
        force = force.mult(strength);
        // if (d < 15) force = force.mult(0.1);
        if (d < 15) force = Vec(0.0, 0.0); // weaken forces so particle doesn't get sucked into blackhole
        return force;
    }
};

struct Orbitones : Module {
    enum TrailIds {
        TRAILS_OFF,
        TRAILS_WHITE,
        TRAILS_REDSHIFT_BLUESHIFT,
        NUM_TRAIL
    };
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
        MOVE_ATTRACTORS_PARAM,
        GLOBAL_GRAVITY_PARAM,
        ON_PARAMS = GLOBAL_GRAVITY_PARAM + NUM_ATTRACTORS,
        GRAVITY_PARAMS = ON_PARAMS + NUM_ATTRACTORS,
		NUM_PARAMS = GRAVITY_PARAMS + NUM_ATTRACTORS
	};
	enum InputIds {
        MOVE_ATTRACTORS_INPUT,
        GLOBAL_GRAVITY_INPUT,
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
        MAX_X_OUTPUT,
        MAX_Y_OUTPUT,
        MIN_X_OUTPUT,
        MIN_Y_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

    dsp::SchmittTrigger removeTrig, clearTrig, moveTrig;
    Attractor *attractors = new Attractor[NUM_ATTRACTORS];
    Particle *particles = new Particle[MAX_PARTICLES];
    bool movement = false;
    bool drawTrails = true;
    bool particleBoundary = false;
    int currentTrailId = 1;
    std::string trails[NUM_TRAIL] = {"off ", "white ", "red/blue shift "};
    int processOrbits = 0;
    int visibleParticles = 2;
    int channels = 1;

    Orbitones() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(REMOVE_PARTICLE_PARAM, 0.0, 1.0, 0.0, "Remove previous particle");
        configParam(CLEAR_PARTICLES_PARAM, 0.0, 1.0, 0.0, "Clear particles");
        configParam(MOVE_ATTRACTORS_PARAM, 0.0, 1.0, 0.0, "Move attractors");
        configParam(GLOBAL_GRAVITY_PARAM, 1.0, 50.0, 30.0, "Global gravity");
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

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        // attractors data
        json_t *attractorsJ = json_array();
        for (int i = 0; i < NUM_ATTRACTORS; i++) {
            json_t *dataJ = json_array();
            json_t *attractorVisibleJ = json_boolean(attractors[i].visible);
            json_t *attractorPosXJ = json_real(attractors[i].box.pos.x);
            json_t *attractorPosYJ = json_real(attractors[i].box.pos.y);

            json_array_append_new(dataJ, attractorVisibleJ);
            json_array_append_new(dataJ, attractorPosXJ);
            json_array_append_new(dataJ, attractorPosYJ);

            // append
            json_array_append_new(attractorsJ, dataJ);
        }

        json_t *particlesJ = json_array();
        for (int i = 0; i < MAX_PARTICLES; i++) {
            json_t *pDataJ = json_array();
            json_t *particleVisibleJ = json_boolean(particles[i].visible);
            json_t *particlePosXJ = json_real(particles[i].box.pos.x);
            json_t *particlePosYJ = json_real(particles[i].box.pos.y);
            json_t *particleRadJ = json_real(particles[i].radius);
            json_t *particleMassJ = json_real(particles[i].mass);

            json_array_append_new(pDataJ, particleVisibleJ);
            json_array_append_new(pDataJ, particlePosXJ);
            json_array_append_new(pDataJ, particlePosYJ);
            json_array_append_new(pDataJ, particleRadJ);
            json_array_append_new(pDataJ, particleMassJ);

            json_array_append_new(particlesJ, pDataJ);
        }

        json_object_set_new(rootJ, "move", json_boolean(movement));
        json_object_set_new(rootJ, "trails", json_integer(currentTrailId));
        json_object_set_new(rootJ, "boundary", json_boolean(particleBoundary));
        json_object_set_new(rootJ, "channels", json_integer(channels));
        json_object_set_new(rootJ, "visibleParticles", json_integer(visibleParticles));
        json_object_set_new(rootJ, "attractors", attractorsJ);
        json_object_set_new(rootJ, "particles", particlesJ);

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *channelsJ = json_object_get(rootJ, "channels");
        if (channelsJ) channels = json_integer_value(channelsJ);

        json_t *moveJ = json_object_get(rootJ, "move");
        if (moveJ) movement = json_boolean_value(moveJ);

        json_t *trailsJ = json_object_get(rootJ, "trails");
        if (trailsJ) setTrails(json_integer_value(trailsJ));

        json_t *particleBoundaryJ = json_object_get(rootJ, "boundary");
        if (particleBoundaryJ) particleBoundary = json_boolean_value(particleBoundaryJ);

        json_t *visibleParticlesJ = json_object_get(rootJ, "visibleParticles");
        if (visibleParticlesJ) visibleParticles = json_integer_value(visibleParticlesJ);

        // data from attractors
        json_t *attractorsJ = json_object_get(rootJ, "attractors");
        if (attractorsJ) {
            for (int i = 0; i < NUM_ATTRACTORS; i++) {
                json_t *dataJ = json_array_get(attractorsJ, i);
                if (dataJ) {
                    json_t *attractorsVisibleJ = json_array_get(dataJ, 0);
                    json_t *attractorsPosXJ = json_array_get(dataJ, 1);
                    json_t *attractorsPosYJ = json_array_get(dataJ, 2);
                    if (attractorsVisibleJ) attractors[i].visible = json_boolean_value(attractorsVisibleJ);
                    if (attractorsPosXJ) attractors[i].box.pos.x = json_real_value(attractorsPosXJ);
                    if (attractorsPosYJ) attractors[i].box.pos.y = json_real_value(attractorsPosYJ);
                }
            }
        }
        // data from particles
        json_t *particlesJ = json_object_get(rootJ, "particles");
        if (particlesJ) {
            for (int i = 0; i < MAX_PARTICLES; i++) {
                json_t *pDataJ = json_array_get(particlesJ, i);
                if (pDataJ) {
                    json_t *particleVisibleJ = json_array_get(pDataJ, 0);
                    json_t *particlePosXJ = json_array_get(pDataJ, 1);
                    json_t *particlePosYJ = json_array_get(pDataJ, 2);
                    json_t *particleRadJ = json_array_get(pDataJ, 3);
                    json_t *particleMassJ = json_array_get(pDataJ, 4);
                    if (particleVisibleJ) {
                        particles[i].visible = json_boolean_value(particleVisibleJ);
                        if (particles[i].visible) {
                            if (particlePosXJ) particles[i].box.pos.x = json_real_value(particlePosXJ);
                            if (particlePosYJ) particles[i].box.pos.y = json_real_value(particlePosYJ);
                            if (particleRadJ) particles[i].radius = json_real_value(particleRadJ);
                            if (particleMassJ) particles[i].mass = json_real_value(particleMassJ);
                        }
                    }
                }
            }
        }
    }

    void process(const ProcessArgs &args) override {
        if (removeTrig.process(params[REMOVE_PARTICLE_PARAM].getValue())) {
            if (visibleParticles > 0) removeParticle(visibleParticles-1);
        }
        if (clearTrig.process(params[CLEAR_PARTICLES_PARAM].getValue())) {
            if (visibleParticles > 0) clearParticles();
        }
        if (moveTrig.process(params[MOVE_ATTRACTORS_PARAM].getValue() + inputs[MOVE_ATTRACTORS_INPUT].getVoltage())) {
            movement = !movement;
        }
        
        if (processOrbits == 0) {
            for (int i = 0; i < NUM_ATTRACTORS; i++) {
                if (inputs[GLOBAL_GRAVITY_INPUT].isConnected()) {
                    attractors[i].G = params[GLOBAL_GRAVITY_PARAM].getValue() * std::pow(2.0, inputs[GLOBAL_GRAVITY_INPUT].getVoltage());
                } else {
                    attractors[i].G = params[GLOBAL_GRAVITY_PARAM].getValue();
                }
                attractors[i].gravityScl = params[GRAVITY_PARAMS + i].getValue();
                if (attractors[i].toggleTrig.process(params[ON_PARAMS+i].getValue())) {
                    attractors[i].visible = !attractors[i].visible;
                }
            }

            if (movement) {
                updateAttractorPos();
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
                if (!drawTrails) particles[i].clearHistory();
                if (particles[i].visible) {
                    for (int j = 0; j < NUM_ATTRACTORS; j++) {
                        if (attractors[j].visible) {
                            Vec force = attractors[j].attract(particles[i]);
                            particles[i].applyForce(force);
                        }
                    }
                    particles[i].update();
                    float voltsX = rescale(particles[i].box.pos.x, 0, DISPLAY_SIZE_WIDTH, -5.0, 5.0);
                    float voltsY = rescale(particles[i].box.pos.y, DISPLAY_SIZE_HEIGHT, 0, -5.0, 5.0);
                    float voltsVelX = rescale(particles[i].vel.x, -12.0, 12.0, -5.0, 5.0);
                    float voltsVelY = rescale(particles[i].vel.y, 12.0, -12.0, -5.0, 5.0);
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
        particles[index].clearHistory();
    }

    void clearParticles() {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            particles[i].box.pos = Vec(0, 0);
            particles[i].vel = Vec(0, 0);
            particles[i].visible = false;
            particles[i].clearHistory();
        }
        visibleParticles = 0;
    }

    void setTrails(int trailId) {
        if (trailId == Orbitones::TRAILS_OFF) {
            currentTrailId = Orbitones::TRAILS_OFF;
            drawTrails = false;
        } else if (trailId == Orbitones::TRAILS_WHITE) {
            currentTrailId = Orbitones::TRAILS_WHITE;
            drawTrails = true;
            for (int i = 0; i < MAX_PARTICLES; i++) {
                particles[i].whiteTrails = true;
            }
        } else {
            currentTrailId = Orbitones::TRAILS_REDSHIFT_BLUESHIFT;
            drawTrails = true;
            for (int i = 0; i < MAX_PARTICLES; i++) {
                particles[i].whiteTrails = false;
            }
        }
    }

    std::string getTrailString() {
        return trails[currentTrailId];
    }

    void updateAttractorPos() {
        for (int i = 0; i < NUM_ATTRACTORS; i++) {
            attractors[i].acc.x = randRange(-0.075, 0.075);
            attractors[i].acc.y = randRange(-0.075, 0.075);

            attractors[i].vel = attractors[i].vel.plus(attractors[i].acc);
            float magSq = attractors[i].vel.x * attractors[i].vel.x + attractors[i].vel.y * attractors[i].vel.y;
            if (magSq > 1) {
                attractors[i].vel = attractors[i].vel.normalize();
                attractors[i].vel = attractors[i].vel.mult(0.9);
            }
            attractors[i].box.pos = attractors[i].box.pos.plus(attractors[i].vel);
        }
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

    struct DrawTrailsValueItem : MenuItem {
        Orbitones *module;
        int trailId;
        void onAction(const event::Action &e) override {
            module->setTrails(trailId);
        }
    };

    struct DrawTrailsItem : MenuItem {
        Orbitones *module;
        Menu *createChildMenu() override {
            Menu *menu = new Menu;
            for (int i = 0; i < Orbitones::NUM_TRAIL; i++) {
                DrawTrailsValueItem *item = new DrawTrailsValueItem;
                item->text = module->trails[i];
                item->rightText = CHECKMARK(module->currentTrailId == i);
                item->module = module;
                item->trailId = i;
                menu->addChild(item);
            }
            return menu;
        }
    };

    struct ParticleBoundaryValueItem : MenuItem {
        Orbitones *module;
        bool boundary;
        void onAction(const event::Action &e) override {
            module->particleBoundary = boundary;
        }
    };

    struct ParticleBoundaryItem : MenuItem {
        Orbitones *module;
        Menu *createChildMenu() override {
            Menu *menu = new Menu;
            for (int i = 0; i < 2; i++) {
                bool boundary = (i == 0);
                ParticleBoundaryValueItem *item = new ParticleBoundaryValueItem;
                item->text = i == 0 ? "on" : "off";
                item->rightText = CHECKMARK(module->particleBoundary == boundary);
                item->module = module;
                item->boundary = boundary;
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

    void checkEdgesParticle(int index) {
        if (module != NULL) {
            // x's
            float radius = module->particles[index].radius;
            if (module->particles[index].box.pos.x < radius) {
                module->particles[index].box.pos.x = radius;
                module->particles[index].vel.x *= -1;
            } else if (module->particles[index].box.pos.x > box.size.x-radius) {
                module->particles[index].box.pos.x = box.size.x-radius;
                module->particles[index].vel.x *= -1;
            }
            // y's
            if (module->particles[index].box.pos.y < radius) {
                module->particles[index].box.pos.y = radius;
                module->particles[index].vel.y *= -1;
            } else if (module->particles[index].box.pos.y > box.size.y - radius) {
                module->particles[index].box.pos.y = box.size.y - radius;
                module->particles[index].vel.y *= -1;
            }
        }
    }

    void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        // background
        nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

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

                checkEdges(i);
            }
        }
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (module->particles[i].visible) {
                // trails
                nvgScissor(args.vg, 0, 0, DISPLAY_SIZE_WIDTH, DISPLAY_SIZE_HEIGHT); // clip trails to display area
                if (module->drawTrails) {
                    bool updated = module->particles[i].updateHistory();
                    if (!updated) break;
                    for (unsigned int j = 1; j < module->particles[i].history.size(); j++) {
                        Trail trailPos = module->particles[i].history[j];
                        Trail trailPosPrev = module->particles[i].history[j-1];
                        nvgBeginPath(args.vg);
                        nvgMoveTo(args.vg, trailPos.x, trailPos.y);
                        nvgLineTo(args.vg, trailPosPrev.x, trailPosPrev.y);
                        int _alpha = module->particles[i].history[j].alpha;
                        module->particles[i].history[j].alpha -= 7;
                        if (_alpha < 0) _alpha = 0;
                        float trailWidth = rescale(_alpha, 255, 0, 2.0, 0.5);
                        nvgStrokeColor(args.vg, nvgTransRGBA(module->particles[i].trailColor, _alpha));
                        nvgStrokeWidth(args.vg, trailWidth);
                        nvgStroke(args.vg);
                    }
                }

                // particles
                Vec pos = module->particles[i].box.getCenter();
                nvgFillColor(args.vg, nvgTransRGBA(module->particles[i].color, 90));
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, module->particles[i].radius);
                nvgFill(args.vg);

                nvgFillColor(args.vg, module->particles[i].color);
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, 2.5);
                nvgFill(args.vg);
                if (module->particleBoundary)
                    checkEdgesParticle(i);
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

        addParam(createParamCentered<TinyBlueButton>(Vec(42, 56), module, Orbitones::REMOVE_PARTICLE_PARAM));
        addParam(createParamCentered<TinyBlueButton>(Vec(42, 93.4), module, Orbitones::CLEAR_PARTICLES_PARAM));
        addParam(createParamCentered<TinyBlueButton>(Vec(42, 130.9), module, Orbitones::MOVE_ATTRACTORS_PARAM));
        addInput(createInputCentered<TinyPJ301M>(Vec(42, 151.3), module, Orbitones::MOVE_ATTRACTORS_INPUT));
        


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
        // global gravity
        addParam(createParamCentered<BlueKnob>(Vec(16, 307.9), module, Orbitones::GLOBAL_GRAVITY_PARAM));
        addInput(createInputCentered<TinyPJ301M>(Vec(39.9, 307.9), module, Orbitones::GLOBAL_GRAVITY_INPUT));

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

        menu->addChild(new MenuEntry);

        OrbitonesNS::DrawTrailsItem *drawTrailsItem = new OrbitonesNS::DrawTrailsItem;
        drawTrailsItem->text = "Particle trails";
        std::string rightText = module->getTrailString();
        drawTrailsItem->rightText = rightText + RIGHT_ARROW;
        drawTrailsItem->module = module;
        menu->addChild(drawTrailsItem);

        OrbitonesNS::ParticleBoundaryItem *particleBoundaryItem = new OrbitonesNS::ParticleBoundaryItem;
        particleBoundaryItem->text = "Particle boundaries";
        std::string boundaryText = module->particleBoundary ? "on " : "off ";
        particleBoundaryItem->rightText = boundaryText + RIGHT_ARROW;
        particleBoundaryItem->module = module;
        menu->addChild(particleBoundaryItem);

    }
};

Model *modelOrbitones = createModel<Orbitones, OrbitonesWidget>("Orbitones");