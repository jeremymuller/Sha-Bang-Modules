#include "plugin.hpp"

#define NUM_OF_NODES 4
#define MAX_PARTICLES 16
#define DISPLAY_SIZE 378
#define INTERNAL_SAMP_TIME 10

struct Particle {
    Rect box;
    NVGcolor color = nvgRGB(255, 255, 255);
    float radius;
    bool locked;
    bool visible;

    Particle() {
        box.pos.x = 0;
        box.pos.y = 0;
        radius = randRange(5, 12);
        visible = false;
        locked = true;
    }

    void setPos(Vec pos) {
        box.pos.x = pos.x;
        box.pos.y = pos.y;
    }
};

struct Pulse {
    Rect box;
    float radius = 3;
    bool visible = false;
    bool isConnected = false;
    bool triggered = false;
    bool blipTrigger = false;
    int haloTime = 0;
    float haloRadius = radius;
    float haloAlpha = 0.0;
    float inc = 0.0;

    Pulse() {
        box.pos.x = 0;
        box.pos.y = 0;
    }

    void setPos(Vec pos) {
        box.pos.x = pos.x;
        box.pos.y = pos.y;
    }

    void blip() {
        if (haloTime > 22000) {
            haloTime = 0;
            haloRadius = radius;
            blipTrigger = false;
        }

        haloAlpha = rescale(haloTime, 0, 22000, 200, 0);

        haloRadius += 18.0 / (APP->engine->getSampleRate() / INTERNAL_SAMP_TIME);
        haloTime += INTERNAL_SAMP_TIME;
    }
};

struct Node {
    Rect box;
    Vec vel;
    Vec acc;
    float radius = 15.5;
    NVGcolor color;
    NVGcolor lineColor;
    Pulse *pulses = new Pulse[MAX_PARTICLES];
    // std::vector<Pulse> pulses;
    float lineAlpha;
    float lineWidth;
    int maxConnectedDist = 150;
    int currentConnects = 0;
    float phase;
    float tempoTime;
    int nodeTempo = maxConnectedDist * 2;
    // float pulseSpeed = 1.0 / 44100 * 60;
    bool locked = true;
    bool visible = true;
    bool start = true;
    bool triggered = false;
    int haloTime = 0;
    float haloRadius = 15.5;
    float haloAlpha = 0;
    float pitchVoltage[16];
    dsp::SchmittTrigger toggleTrig;
    dsp::PulseGenerator gatePulse[16];

    Node() {
        box.pos.x = 30;
        box.pos.y = 30;

        // for (unsigned int i = 0; i < 25; i++) {
        //     Pulse p;
        //     pulses.push_back(p);
        // }

        // tempoTime = 0;
        tempoTime = static_cast<int>(random::uniform() * maxConnectedDist);
        phase = random::uniform();
    }

    ~Node() {
        delete[] pulses;
    }

    bool connected(Vec p, int index) {
        float d = dist(box.pos, p);
        if (d < maxConnectedDist) {
            pulses[index].isConnected = true;
            lineAlpha = rescale(d, 0, maxConnectedDist, 255, 25);
            lineWidth = rescale(d, 0, maxConnectedDist, 3.0, 1.5);
            return true;
        } else {
            pulses[index].isConnected = false;
            pulses[index].visible = false;
            pulses[index].box.pos.x = box.pos.x;
            pulses[index].box.pos.y = box.pos.y;
            return false;
        }
    }

    void sendPulse(Vec particle, int index, float pulseSpeed) {
        if (start) {
            Pulse *pulse = &pulses[index];
            if (pulse == NULL) return;

            if (phase == 0) {
                // triggered = true;

                pulse->visible = true;
                pulse->inc = dist(particle, box.pos);
            }

            if (pulse->visible) {
                pulse->box.pos.x = particle.x;
                pulse->box.pos.y = particle.y;

                Vec posC = box.getCenter();
                Vec dir = posC.minus(particle);
                Vec dirNormal = dir.normalize();
                Vec dirMagSet = dirNormal.mult(pulse->inc);
                pulse->box.pos = dirMagSet.plus(pulse->box.pos);

                pulse->inc -= pulseSpeed;
                float d = dist(box.pos, particle);
                if (pulse->inc > d) pulse->inc = d;
                if (pulse->inc < 0) {
                    pulse->visible = false;
                    pulse->triggered = true;
                    pulse->blipTrigger = true;

                }
            }

        }
    }

    void blip() {
        if (haloTime > 22000) {
            haloTime = 0;
            haloRadius = radius;
            triggered = false;
        }

        haloAlpha = rescale(haloTime, 0, 22000, 200, 0);
        // haloAlpha = 255;

        haloRadius += 18.0 / (APP->engine->getSampleRate() / INTERNAL_SAMP_TIME);
        haloTime += INTERNAL_SAMP_TIME;
    }
};

struct Neutrinode : Module, Quantize {
    enum NodeIds {
        PURPLE_NODE,
        BLUE_NODE,
        AQUA_NODE,
        RED_NODE,
        NUM_NODES
    };
    enum ParamIds {
        BPM_PARAM,
        PLAY_PARAM,
        MOVE_PARAM,
        SPEED_PARAM,
        ROOT_NOTE_PARAM,
        SCALE_PARAM,
        PITCH_PARAM,
        RND_PARTICLES_PARAM,
        CLEAR_PARTICLES_PARAM,
        ON_PARAM = CLEAR_PARTICLES_PARAM + NUM_OF_NODES,
        OCTAVE_PARAMS = ON_PARAM + NUM_OF_NODES,
        NUM_PARAMS = OCTAVE_PARAMS + NUM_OF_NODES
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        GATES_ALL_OUTPUTS,
        VOLTS_ALL_OUTPUTS,
        GATE_OUTPUTS = VOLTS_ALL_OUTPUTS + NUM_OF_NODES,
        VOLT_OUTPUTS = GATE_OUTPUTS + NUM_OF_NODES,
        NUM_OUTPUTS = VOLT_OUTPUTS + NUM_OF_NODES
    };
    enum LightIds {
        PAUSE_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger rndTrig, clearTrig, pauseTrig;
    dsp::PulseGenerator gatePulsesAll[16];
    float allPitches[16] = {}; // <-- this is weird when two nodes collide on one particle
    Node *nodes = new Node[NUM_OF_NODES];
    Particle *particles = new Particle[MAX_PARTICLES];
    int visibleParticles = 0;
    // std::vector<Particle> particles;
    bool movement = false;
    bool pitchChoice = false;
    bool toggleStart = true;
    float clockStep;
    float maxConnectedDist = 150;
    float pulseSpeed = 1.0 / APP->engine->getSampleRate() * maxConnectedDist;
    int checkParams = 0;
    int processNodes = 0;
    int moveNodes = 0;
    int channels = 16;

    Neutrinode() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BPM_PARAM, 15, 120, 30, "Tempo", " bpm");
        configParam(PLAY_PARAM, 0.0, 1.0, 0.0, "Start nodes");
        configParam(MOVE_PARAM, 0.0, 1.0, 0.0, "Move nodes");
        configParam(SPEED_PARAM, -1.0, 1.0, 0.0, "Node speed");
        configParam(ROOT_NOTE_PARAM, 0.0, Quantize::NUM_OF_NOTES-1, 0.0, "Root note");
        configParam(SCALE_PARAM, 0.0, Quantize::NUM_OF_SCALES, 0.0, "Scale");
        configParam(PITCH_PARAM, 0.0, 1.0, 0.0);
        configParam(RND_PARTICLES_PARAM, 0.0, 1.0, 0.0, "Randomize particles");
        configParam(CLEAR_PARTICLES_PARAM, 0.0, 1.0, 0.0, "Clear particles");
        configParam(ON_PARAM + PURPLE_NODE, 0.0, 1.0, 0.0, "toggle purple node");
        configParam(ON_PARAM + BLUE_NODE, 0.0, 1.0, 0.0, "toggle blue node");
        configParam(ON_PARAM + AQUA_NODE, 0.0, 1.0, 0.0, "toggle aqua node");
        configParam(ON_PARAM + RED_NODE, 0.0, 1.0, 0.0, "toggle red node");
        configParam(OCTAVE_PARAMS + PURPLE_NODE, -5.0, 5.0, 0.0, "octave purple node");
        configParam(OCTAVE_PARAMS + BLUE_NODE, -5.0, 5.0, 0.0, "octave blue node");
        configParam(OCTAVE_PARAMS + AQUA_NODE, -5.0, 5.0, 0.0, "octave aqua node");
        configParam(OCTAVE_PARAMS + RED_NODE, -5.0, 5.0, 0.0, "octave red node");

        nodes[0].color = nvgRGBA(128, 0, 219, 255);
        nodes[1].color = nvgRGBA(38, 0, 255, 255);
        nodes[2].color = nvgRGBA(0, 238, 219, 255);
        nodes[3].color = nvgRGBA(255, 0, 0, 255);
        nodes[0].lineColor = nvgRGB(213, 153, 255);
        nodes[1].lineColor = nvgRGB(165, 152, 255);
        nodes[2].lineColor = nvgRGB(104, 245, 255);
        nodes[3].lineColor = nvgRGB(255, 101, 101);
        nodes[0].box.pos = Vec(randRange(16, DISPLAY_SIZE/2.0-16), randRange(16, DISPLAY_SIZE/2.0-16));
        nodes[1].box.pos = Vec(randRange(DISPLAY_SIZE/2.0+16, DISPLAY_SIZE-16), randRange(16, DISPLAY_SIZE/2.0-16));
        nodes[2].box.pos = Vec(randRange(DISPLAY_SIZE/2.0+16, DISPLAY_SIZE/2.0-16), randRange(DISPLAY_SIZE/2.0+16, DISPLAY_SIZE/2.0-16));
        nodes[3].box.pos = Vec(randRange(16, DISPLAY_SIZE/2.0-16), randRange(DISPLAY_SIZE/2.0+16, DISPLAY_SIZE-16));
    }

    ~Neutrinode() {
        delete[] nodes;
        delete[] particles;
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        json_t *nodesJ = json_array();
        json_t *particlesJ = json_array();
        for (int i = 0; i < NUM_OF_NODES; i++) {
            json_t *dataJ = json_array();
            json_t *nodeVisibleJ = json_boolean(nodes[i].visible);
            json_t *nodePosXJ = json_real(nodes[i].box.pos.x);
            json_t *nodePosYJ = json_real(nodes[i].box.pos.y);
            json_t *nodeTempoTimeJ = json_real(nodes[i].tempoTime);
            json_t *nodePhaseJ = json_real(nodes[i].phase);
            // other data?
            json_array_append_new(dataJ, nodeVisibleJ);
            json_array_append_new(dataJ, nodePosXJ);
            json_array_append_new(dataJ, nodePosYJ);
            json_array_append_new(dataJ, nodeTempoTimeJ);
            json_array_append_new(dataJ, nodePhaseJ);

            // append node
            json_array_append_new(nodesJ, dataJ);
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {
            json_t *pDataJ = json_array();
            json_t *particleVisibleJ = json_boolean(particles[i].visible);
            json_t *particlePosXJ = json_real(particles[i].box.pos.x);
            json_t *particlePosYJ = json_real(particles[i].box.pos.y);
            json_t *particleRadJ = json_real(particles[i].radius);

            json_array_append_new(pDataJ, particleVisibleJ);
            json_array_append_new(pDataJ, particlePosXJ);
            json_array_append_new(pDataJ, particlePosYJ);
            json_array_append_new(pDataJ, particleRadJ);

            json_array_append_new(particlesJ, pDataJ);
        }

        // json_object_set_new(rootJ, "start", json_boolean(toggleStart));
        json_object_set_new(rootJ, "channels", json_integer(channels));
        json_object_set_new(rootJ, "nodes", nodesJ);
        json_object_set_new(rootJ, "particles", particlesJ);

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *channelsJ = json_object_get(rootJ, "channels");
        if (channelsJ) channels = json_integer_value(channelsJ);

        // json_t *startJ = json_object_get(rootJ, "start");
        // if (startJ) toggleStart = json_boolean_value(startJ);


        // data from nodes
        json_t *nodesJ = json_object_get(rootJ, "nodes");
        if (nodesJ) {
            for (int i = 0; i < NUM_OF_NODES; i++) {
                json_t *dataJ = json_array_get(nodesJ, i);
                if (dataJ) {
                    json_t *nodeVisibleJ = json_array_get(dataJ, 0);
                    json_t *nodePosXJ = json_array_get(dataJ, 1);
                    json_t *nodePosYJ = json_array_get(dataJ, 2);
                    json_t *nodeTempoTimeJ = json_array_get(dataJ, 3);
                    json_t *nodePhaseJ = json_array_get(dataJ, 4);
                    if (nodeVisibleJ) nodes[i].visible = json_boolean_value(nodeVisibleJ);
                    if (nodePosXJ) nodes[i].box.pos.x = json_real_value(nodePosXJ);
                    if (nodePosYJ) nodes[i].box.pos.y = json_real_value(nodePosYJ);
                    if (nodeTempoTimeJ) nodes[i].tempoTime = json_real_value(nodeTempoTimeJ);
                    if (nodePhaseJ) nodes[i].phase = json_real_value(nodePhaseJ);
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
                    if (particleVisibleJ) {
                        if (json_boolean_value(particleVisibleJ)) {
                            float x = 0;
                            float y = 0;
                            float r = 0;
                            if (particlePosXJ) x = json_real_value(particlePosXJ);
                            if (particlePosYJ) y = json_real_value(particlePosYJ);
                            if (particleRadJ) r = json_real_value(particleRadJ);
                            addParticle(Vec(x, y), i, r);
                        }
                    }
                }
            }
        }
    }

    void process(const ProcessArgs &args) override {
        // outputs[GATE_OUTPUTS + PURPLE_NODE].setChannels(channels);
        // outputs[GATE_OUTPUTS + BLUE_NODE].setChannels(channels);
        // outputs[GATE_OUTPUTS + AQUA_NODE].setChannels(channels);
        // outputs[GATE_OUTPUTS + RED_NODE].setChannels(channels);
        // outputs[VOLT_OUTPUTS + PURPLE_NODE].setChannels(channels);
        // outputs[VOLT_OUTPUTS + BLUE_NODE].setChannels(channels);
        // outputs[VOLT_OUTPUTS + AQUA_NODE].setChannels(channels);
        // outputs[VOLT_OUTPUTS + RED_NODE].setChannels(channels);
        // outputs[GATES_ALL_OUTPUTS].setChannels(channels);
        // outputs[VOLTS_ALL_OUTPUTS].setChannels(channels);

        // checks param knobs every 4th sample
        if (checkParams == 0) {
            // if (rndTrig.process(params[RND_PARTICLES_PARAM].getValue())) {
            //     randomizeParticles();
            // }
            if (clearTrig.process(params[CLEAR_PARTICLES_PARAM].getValue())) {
                clearParticles();
            }
            if (pauseTrig.process(params[PLAY_PARAM].getValue())) {
                toggleStart = !toggleStart;
            }
            movement = params[MOVE_PARAM].getValue();
            pitchChoice = params[PITCH_PARAM].getValue();
            
            clockStep = params[BPM_PARAM].getValue() / 60.0;
            clockStep = (clockStep / (args.sampleRate / INTERNAL_SAMP_TIME)) / 2;
            pulseSpeed = clockStep * maxConnectedDist * 2;
        }
        checkParams = (checkParams+1) % 4;

        // TODO: This doesn't need to happen every sample, might try every 700 samples

        if (processNodes == 0) {
            lights[PAUSE_LIGHT].setBrightness(toggleStart ? 1.0 : 0.0);

            if (movement) {
                if (moveNodes == 0) {
                    updateNodePos();
                }
                moveNodes = (moveNodes+1) % static_cast<int>(args.sampleRate/60.0/INTERNAL_SAMP_TIME); // check 60 hz
            }

            int polyChannelIndex = 0;
            int rootNote = params[ROOT_NOTE_PARAM].getValue();
            int scale = params[SCALE_PARAM].getValue();
            outputs[GATES_ALL_OUTPUTS].setChannels(channels);
            outputs[VOLTS_ALL_OUTPUTS].setChannels(channels);
            for (int i = 0; i < NUM_OF_NODES; i++) {
                nodes[i].start = toggleStart;


                if (nodes[i].toggleTrig.process(params[ON_PARAM+i].getValue())) {
                    nodes[i].start = !nodes[i].start;
                    nodes[i].visible = !nodes[i].visible;
                }

                if (nodes[i].visible && nodes[i].start) {
                    int oct = params[OCTAVE_PARAMS + i].getValue();

                    // when using 16 channels it doesn't work for All outputs
                    for (int j = 0; j < MAX_PARTICLES; j++) {
                        if (particles[j].visible) {
                            Vec p = particles[j].box.getCenter();
                            nodes[i].sendPulse(p, j, pulseSpeed);
                            if (nodes[i].pulses[j].triggered) {
                                nodes[i].pulses[j].triggered = false;
                                nodes[i].gatePulse[j % channels].trigger(1e-3f);
                                // gatePulsesAll[j % channels].trigger(1e-3f);
                                gatePulsesAll[polyChannelIndex].trigger(1e-3f);

                                float volts;
                                float margin = 7.0;
                                if (pitchChoice) volts = rescale(particles[j].box.pos.y, DISPLAY_SIZE-margin, margin, 0.0, 2.0);
                                else volts = rescale(particles[j].radius, 5.0, 12.0, 2.0, 0.0);
                                float pitch = Quantize::quantizeRawVoltage(volts, rootNote, scale) + oct;
                                // allPitches[polyChannelIndex] = pitch;
                                // outs
                                outputs[VOLT_OUTPUTS + i].setVoltage(pitch, (j % channels));
                                outputs[VOLTS_ALL_OUTPUTS].setVoltage(pitch, polyChannelIndex);
                                // outputs[VOLTS_ALL_OUTPUTS].setVoltage(pitch, (j % channels));
                            }
                            if (nodes[i].pulses[j].blipTrigger) nodes[i].pulses[j].blip();

                            bool pulse = nodes[i].gatePulse[j % channels].process(1.0 / args.sampleRate);
                            outputs[GATE_OUTPUTS + i].setVoltage(pulse ? 10.0 : 0.0, (j % channels));
                            bool pulseAll = gatePulsesAll[polyChannelIndex].process(1.0 / args.sampleRate);
                            outputs[GATES_ALL_OUTPUTS].setVoltage(pulseAll ? 10.0 : 0.0, polyChannelIndex);
                            polyChannelIndex = (polyChannelIndex+1) % channels;
                        }
                    }

                    nodes[i].phase += clockStep;
                    if (nodes[i].triggered) nodes[i].blip();
                    if (nodes[i].phase > 1.0) {
                        nodes[i].phase = 0;
                        nodes[i].triggered = true;
                    }

                }
                outputs[GATE_OUTPUTS + i].setChannels(channels);
                outputs[VOLT_OUTPUTS + i].setChannels(channels);
            }

        }
        processNodes = (processNodes+1) % INTERNAL_SAMP_TIME;

    }

    void setChannels(int channels) {
		if (channels == this->channels)
			return;
		this->channels = channels;
	}

    // TODO: probably won't use this
    // void randomizeParticles() {
    //     if (particles.size() < 1) {
    //         for (int i = 0; i < 4; i++) {
    //             Particle p(randRange(15, DISPLAY_SIZE - 15), randRange(15, DISPLAY_SIZE - 15));
    //             particles.push_back(p);
    //             for (int i = 0; i < NUM_OF_NODES; i++) {
    //                 Pulse pulse;
    //                 nodes[i].pulses.push_back(pulse);
    //             }
    //         }
    //     } else {
    //         for (unsigned int i = 0; i < particles.size(); i++) {
    //             particles[i].box.pos.x = randRange(15, DISPLAY_SIZE - 15);
    //             particles[i].box.pos.y = randRange(15, DISPLAY_SIZE - 15);
    //         }
    //     }
    // }

    void addParticle(Vec pos, int index) {
        visibleParticles++;
        particles[index].setPos(pos);
        particles[index].radius = randRange(5, 12);
        particles[index].visible = true;
        particles[index].locked = false;

        for (int i = 0; i < NUM_OF_NODES; i++) {
            nodes[i].pulses[index].setPos(nodes[i].box.getCenter());
        }
    }

    void addParticle(Vec pos, int index, float _radius) {
        visibleParticles++;
        particles[index].setPos(pos);
        particles[index].radius = _radius;
        particles[index].visible = true;
        particles[index].locked = false;

        for (int i = 0; i < NUM_OF_NODES; i++) {
            nodes[i].pulses[index].setPos(nodes[i].box.getCenter());
        }
    }

    void removeParticle(int index) {
        visibleParticles--;
        particles[index].visible = false;
        particles[index].locked = true;

        for (int i = 0; i < NUM_OF_NODES; i++) {
            nodes[i].pulses[index].visible = false;

            // nodes[i].pulses.erase(nodes[i].pulses.begin() + index);
        }
    }

    void clearParticles() {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            particles[i].visible = false;
            particles[i].locked = true;
            for (int j = 0; j < NUM_OF_NODES; j++) {
                nodes[j].pulses[i].visible = false;
            }
        }
        visibleParticles = 0;
        // particles.clear();
        // nodes[i].pulses.clear();
    }

    void updateNodePos() {
        float speed = std::pow(2.0, params[SPEED_PARAM].getValue());
        for (int i = 0; i < NUM_OF_NODES; i++) {
            // nodes[i].acc.x = randRange(-speed, speed);
            // nodes[i].acc.y = randRange(-speed, speed);
            nodes[i].acc.x = randRange(-0.075, 0.075);
            nodes[i].acc.y = randRange(-0.075, 0.075);

            nodes[i].vel = nodes[i].vel.plus(nodes[i].acc);
            float magSq = nodes[i].vel.x * nodes[i].vel.x + nodes[i].vel.y * nodes[i].vel.y;
            if (magSq > 1) {
                nodes[i].vel = nodes[i].vel.normalize();
                nodes[i].vel = nodes[i].vel.mult(speed);
            }
            nodes[i].box.pos = nodes[i].box.pos.plus(nodes[i].vel);
        }
    }

};

struct ChannelValueItem : MenuItem {
    Neutrinode *module;
    int channels;
    void onAction(const event::Action &e) override {
        module->setChannels(channels);
    }
};

struct ChannelItem : MenuItem {
    Neutrinode *module;
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

struct NeutrinodeDisplay : Widget {
    Neutrinode *module;
    float currentX = 0;
    float currentY = 0;
    float posX = 0;
    float posY = 0;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;

    NeutrinodeDisplay() {}

    void onButton(const event::Button &e) override {
        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			e.consume(this);
			initX = e.pos.x;
			initY = e.pos.y;
            Vec inits = Vec(initX, initY);
            bool clickedOnObj = false;
            int nextAvailableIndex = 0;
            for (int i = 0; i < NUM_OF_NODES; i++) {
                if (module->nodes[i].visible) {
                    Vec nodePos = module->nodes[i].box.getCenter();
                    float d = dist(inits, nodePos);
                    if (d < 16 && !clickedOnObj) {
                        module->nodes[i].box.pos.x = initX;
                        module->nodes[i].box.pos.y = initY;
                        module->nodes[i].locked = false;
                        clickedOnObj = true;
                    } else {
                        module->nodes[i].locked = true;
                    }
                } else {
                    module->nodes[i].locked = true;
                }
            }
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (module->particles[i].visible) {
                    Vec partPos = module->particles[i].box.getCenter();
                    float d = dist(inits, partPos);
                    float r = module->particles[i].radius;
                    if (d < r && !clickedOnObj) {
                        // module->particles.erase(module->particles.begin()+i);
                        module->particles[i].box.pos.x = initX;
                        module->particles[i].box.pos.y = initY;
                        module->particles[i].locked = false;
                        clickedOnObj = true;
                    } else {
                        module->particles[i].locked = true;
                    }
                } else {
                    nextAvailableIndex = i;
                }
            }

            if (!clickedOnObj && (module->visibleParticles < MAX_PARTICLES)) {
                module->addParticle(inits, nextAvailableIndex);

                // for (int i = 0; i < NUM_OF_NODES; i++) {
                //     module->nodes[i].pulses[nextAvailableIndex].setPos(module->nodes[i].box.getCenter());
                //     module->nodes[i].pulses[nextAvailableIndex].visible = true;

                //     // Pulse pulse;
                //     // module->nodes[i].pulses.push_back(pulse);
                // }

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

        for (int i = 0; i < NUM_OF_NODES; i++) {
            if (!module->nodes[i].locked) {
                module->nodes[i].box.pos.x = initX + (newDragX-dragX);
                module->nodes[i].box.pos.y = initY + (newDragY-dragY);
                checkEdges(i);
            }
        }
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!module->particles[i].locked && module->particles[i].visible) {
                module->particles[i].box.pos.x = initX + (newDragX - dragX);
                module->particles[i].box.pos.y = initY + (newDragY - dragY);
                checkEdgesForDelete(i);
            }
        }
        // posX = initX + (newDragX - dragX);
        // posY = initY + (newDragY - dragY);

        // // float dist = std::sqrt(std::pow(posX-currentX, 2) + std::pow(posY-currentY, 2));
        // Vec inits = Vec(initX, initY);
        // Vec nodePos = module->nodes[0].box.getCenter();
        // float d = dist(inits, nodePos);
        // if (d < 12) {
        //     module->nodes[0].box.pos.x = initX;
        //     module->nodes[0].box.pos.y = initY;
        //     // currentX = posX;
        //     // currentY = posY;
        // }
    }

    void checkEdges(int index) {
        if (module != NULL) {
            // x's
            if (module->nodes[index].box.pos.x < 16) {
                module->nodes[index].box.pos.x = 16;
                module->nodes[index].vel.x *= -1;
            } else if (module->nodes[index].box.pos.x > box.size.x-16) {
                module->nodes[index].box.pos.x = box.size.x-16;
                module->nodes[index].vel.x *= -1;
            }
            // y's
            if (module->nodes[index].box.pos.y < 16) {
                module->nodes[index].box.pos.y = 16;
                module->nodes[index].vel.y *= -1;
            } else if (module->nodes[index].box.pos.y > box.size.y - 16) {
                module->nodes[index].box.pos.y = box.size.y - 16;
                module->nodes[index].vel.y *= -1;
            }
        }
    }

    void checkEdgesForDelete(int index) {
        if (module != NULL) {
            bool eraseParticle = false;
            float r = module->particles[index].radius;

            if (module->particles[index].box.pos.x < r) eraseParticle = true;
            else if (module->particles[index].box.pos.x > box.size.x - r) eraseParticle = true;
            else if (module->particles[index].box.pos.y < r) eraseParticle = true;
            else if (module->particles[index].box.pos.y > box.size.y - r) eraseParticle = true;

            if (eraseParticle) {
                module->removeParticle(index);
                // for (int i = 0; i < NUM_OF_NODES; i++) {
                //     // module->nodes[i].pulses[index].visible = false;
                //     // module->nodes[i].pulses.erase(module->nodes[i].pulses.begin() + index);
                // }
            }
        }
    }

    void draw(const DrawArgs &args) override {
        //background
        nvgFillColor(args.vg, nvgRGB(40, 40, 40));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);

        if (module != NULL) {
            // draw nodes
            for (int i = 0; i < NUM_OF_NODES; i++) {
                if (module->nodes[i].visible) {
                    // draw lines and pulses
                    // Node n = module->nodes[i];
                    for (int j = 0; j < MAX_PARTICLES; j++) {
                        if (module->particles[j].visible) {
                            Vec particle = module->particles[j].box.getCenter();
                            if (module->nodes[i].connected(particle, j)) {
                                nvgStrokeColor(args.vg, nvgTransRGBA(module->nodes[i].lineColor, module->nodes[i].lineAlpha));
                                nvgStrokeWidth(args.vg, module->nodes[i].lineWidth);
                                nvgBeginPath(args.vg);
                                nvgMoveTo(args.vg, module->nodes[i].box.pos.x, module->nodes[i].box.pos.y);
                                nvgLineTo(args.vg, particle.x, particle.y);
                                nvgStroke(args.vg);
                            }

                            if (module->nodes[i].start) {
                                Pulse *pulse = &module->nodes[i].pulses[j];
                                if (pulse->visible && pulse->isConnected) {


                                    nvgFillColor(args.vg, nvgTransRGBA(module->nodes[i].color, 200));
                                    nvgBeginPath(args.vg);
                                    // Vec pulsePos = module->nodes[i].pulses[j].box.getCenter();
                                    nvgCircle(args.vg, pulse->box.pos.x, pulse->box.pos.y, pulse->radius);
                                    nvgFill(args.vg);

                                }
                                if (pulse->blipTrigger) {
                                    // pulse blip
                                    nvgFillColor(args.vg, nvgTransRGBA(module->nodes[i].lineColor, pulse->haloAlpha));
                                    nvgBeginPath(args.vg);
                                    nvgCircle(args.vg, particle.x, particle.y, pulse->haloRadius);
                                    nvgFill(args.vg);
                                }
                            }
                        }
                    }

                    Vec pos = module->nodes[i].box.getCenter();
                    // display halos
                    if (module->nodes[i].triggered) {
                        nvgStrokeColor(args.vg, nvgTransRGBA(module->nodes[i].color, module->nodes[i].haloAlpha));
                        nvgStrokeWidth(args.vg, 2);
                        nvgBeginPath(args.vg);
                        nvgCircle(args.vg, pos.x, pos.y, module->nodes[i].haloRadius);
                        nvgStroke(args.vg);
                    }
                    // display nodes
                    nvgStrokeColor(args.vg, module->nodes[i].color);
                    nvgStrokeWidth(args.vg, 2);
                    nvgBeginPath(args.vg);
                    nvgCircle(args.vg, pos.x, pos.y, module->nodes[i].radius);
                    nvgStroke(args.vg);

                    nvgFillColor(args.vg, module->nodes[i].color);
                    nvgBeginPath(args.vg);
                    nvgCircle(args.vg, pos.x, pos.y, module->nodes[i].radius-3.5);
                    nvgFill(args.vg);

                    checkEdges(i);
                }
            }

            // draw particles
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (module->particles[i].visible) {
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

    }

};

struct NeutrinodeWidget : ModuleWidget {
    NeutrinodeWidget(Neutrinode *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Neutrinode.svg")));

        NeutrinodeDisplay *display = new NeutrinodeDisplay();
        display->module = module;
        display->box.pos = Vec(161.2, 0.8);
        display->box.size = Vec(DISPLAY_SIZE, DISPLAY_SIZE);
        addChild(display);

        // screws
        addChild(createWidget<JeremyScrew>(Vec(74.6, 2)));
        addChild(createWidget<JeremyScrew>(Vec(74.6, box.size.y - 14)));
        // addChild(createWidget<JeremyScrew>(Vec(431, 2)));
        // addChild(createWidget<JeremyScrew>(Vec(431, box.size.y - 14)));

        addParam(createParamCentered<PurpleButton>(Vec(26.4, 78.1), module, Neutrinode::PLAY_PARAM));
        addParam(createParamCentered<PurpleInvertKnob>(Vec(58.8, 78.1), module, Neutrinode::BPM_PARAM));
        addParam(createParamCentered<Jeremy_HSwitch>(Vec(91.5, 78.1), module, Neutrinode::MOVE_PARAM));
        addParam(createParamCentered<PurpleKnob>(Vec(130.7, 78.1), module, Neutrinode::SPEED_PARAM));

        // note and scale knobs
        PurpleNoteKnob *noteKnob = dynamic_cast<PurpleNoteKnob *>(createParamCentered<PurpleNoteKnob>(Vec(26.4, 122.3), module, Neutrinode::ROOT_NOTE_PARAM));
        LeftAlignedLabel* const noteLabel = new LeftAlignedLabel;
        noteLabel->box.pos = Vec(42.6, 125.8);
        noteLabel->text = "";
        noteKnob->connectLabel(noteLabel, module);
        addChild(noteLabel);
        addParam(noteKnob);

        PurpleScaleKnob *scaleKnob = dynamic_cast<PurpleScaleKnob *>(createParamCentered<PurpleScaleKnob>(Vec(26.4, 153.4), module, Neutrinode::SCALE_PARAM));
        LeftAlignedLabel* const scaleLabel = new LeftAlignedLabel;
        scaleLabel->box.pos = Vec(42.6, 157.7);
        scaleLabel->text = "";
        scaleKnob->connectLabel(scaleLabel, module);
        addChild(scaleLabel);
        addParam(scaleKnob);

        // addParam(createParamCentered<PurpleInvertKnob>(Vec(26.4, 136.7), module, Neutrinode::ROOT_NOTE_PARAM));
        // addParam(createParamCentered<PurpleInvertKnob>(Vec(58.8, 136.7), module, Neutrinode::SCALE_PARAM));
        addParam(createParamCentered<Jeremy_HSwitch>(Vec(91.5, 122.8), module, Neutrinode::PITCH_PARAM));

        addParam(createParamCentered<PurpleButton>(Vec(130.7, 122.3), module, Neutrinode::CLEAR_PARTICLES_PARAM));
        // addParam(createParamCentered<PurpleButton>(Vec(130.7, 78.1), module, Neutrinode::RND_PARTICLES_PARAM));
        // addParam(createParamCentered<PurpleButton>(Vec(130.7, 121.9), module, Neutrinode::CLEAR_PARTICLES_PARAM));


        addChild(createLight<SmallLight<JeremyAquaLight>>(Vec(110.5 - 3.21, 24.3 - 3.21), module, Neutrinode::PAUSE_LIGHT));

        // toggles
        addParam(createParamCentered<TinyPurpleButton>(Vec(96.8, 201.8), module, Neutrinode::ON_PARAM + Neutrinode::PURPLE_NODE));
        addParam(createParamCentered<TinyBlueButton>(Vec(96.8, 233.8), module, Neutrinode::ON_PARAM + Neutrinode::BLUE_NODE));
        addParam(createParamCentered<TinyAquaButton>(Vec(96.8, 265.8), module, Neutrinode::ON_PARAM + Neutrinode::AQUA_NODE));
        addParam(createParamCentered<TinyRedButton>(Vec(96.8, 297.8), module, Neutrinode::ON_PARAM + Neutrinode::RED_NODE));
        // octaves
        addParam(createParamCentered<PurpleInvertKnob>(Vec(129.1, 201.8), module, Neutrinode::OCTAVE_PARAMS + Neutrinode::PURPLE_NODE));
        addParam(createParamCentered<BlueInvertKnob>(Vec(129.1, 233.8), module, Neutrinode::OCTAVE_PARAMS + Neutrinode::BLUE_NODE));
        addParam(createParamCentered<AquaInvertKnob>(Vec(129.1, 265.8), module, Neutrinode::OCTAVE_PARAMS + Neutrinode::AQUA_NODE));
        addParam(createParamCentered<RedInvertKnob>(Vec(129.1, 297.8), module, Neutrinode::OCTAVE_PARAMS + Neutrinode::RED_NODE));
        // gate outputs
        addOutput(createOutputCentered<PJ301MPurple>(Vec(32.1, 201.8), module, Neutrinode::GATE_OUTPUTS + Neutrinode::PURPLE_NODE));
        addOutput(createOutputCentered<PJ301MBlue>(Vec(32.1, 233.8), module, Neutrinode::GATE_OUTPUTS + Neutrinode::BLUE_NODE));
        addOutput(createOutputCentered<PJ301MAqua>(Vec(32.1, 265.8), module, Neutrinode::GATE_OUTPUTS + Neutrinode::AQUA_NODE));
        addOutput(createOutputCentered<PJ301MRed>(Vec(32.1, 297.8), module, Neutrinode::GATE_OUTPUTS + Neutrinode::RED_NODE));
        // all
        addOutput(createOutputCentered<PJ301MPort>(Vec(32.1, 343.2), module, Neutrinode::GATES_ALL_OUTPUTS));
        // volt outputs
        addOutput(createOutputCentered<PJ301MPurple>(Vec(64.4, 201.8), module, Neutrinode::VOLT_OUTPUTS + Neutrinode::PURPLE_NODE));
        addOutput(createOutputCentered<PJ301MBlue>(Vec(64.4, 233.8), module, Neutrinode::VOLT_OUTPUTS + Neutrinode::BLUE_NODE));
        addOutput(createOutputCentered<PJ301MAqua>(Vec(64.4, 265.8), module, Neutrinode::VOLT_OUTPUTS + Neutrinode::AQUA_NODE));
        addOutput(createOutputCentered<PJ301MRed>(Vec(64.4, 297.8), module, Neutrinode::VOLT_OUTPUTS + Neutrinode::RED_NODE));
        // all volts
        addOutput(createOutputCentered<PJ301MPort>(Vec(64.4, 343.2), module, Neutrinode::VOLTS_ALL_OUTPUTS));
    }

    // TODO: issue with polyphony menu
    // void appendContextMenu(Menu *menu) override {
    //     Neutrinode *module = dynamic_cast<Neutrinode*>(this->module);
    //     MenuEntry *spacerLabel = new MenuEntry();
    //     menu->addChild(spacerLabel);

    //     ChannelItem *channelItem = new ChannelItem;
    //     channelItem->text = "Polyphony channels";
    //     channelItem->rightText = string::f("%d", module->channels) + " " + RIGHT_ARROW;
    //     channelItem->module = module;
    //     menu->addChild(channelItem);
    // }
};

Model *modelNeutrinode = createModel<Neutrinode, NeutrinodeWidget>("Neutrinode");
