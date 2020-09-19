#include "plugin.hpp"

#define NUM_OF_NODES 4
#define MAX_PARTICLES 16
#define DISPLAY_SIZE 378

struct Particle {
    Rect box;
    NVGcolor color = nvgRGB(255, 255, 255);
    float radius;
    bool locked = true;
    bool visible = true;

    Particle(float _x, float _y) {
        box.pos.x = _x;
        box.pos.y = _y;
        radius = randRange(5, 10);
    }
};

struct Pulse {
    Rect box;
    float radius = 3;
    // float radius = 6;
    bool visible = false;
    bool isConnected = false;
    bool triggered = false;
    bool blipTrigger = false;
    int haloTime = 0;
    float haloRadius = radius;
    float haloAlpha = 0.0;
    float inc = 0.0;

    Pulse() {}

    void blip() {
        if (haloTime > 22000) {
            haloTime = 0;
            haloRadius = radius;
            blipTrigger = false;
        }

        haloAlpha = rescale(haloTime, 0, 22000, 200, 0);

        haloRadius += 18.0 / APP->engine->getSampleRate();
        haloTime++;
    }
};

struct Node {
    Rect box;
    Vec vel;
    Vec acc;
    float radius = 15.5;
    NVGcolor color;
    NVGcolor lineColor;
    std::vector<Pulse> pulses;
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

        haloRadius += 18.0 / APP->engine->getSampleRate();
        haloTime++;
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
        GATE_OUTPUTS = NUM_OF_NODES,
        VOLT_OUTPUTS = GATE_OUTPUTS + NUM_OF_NODES,
        NUM_OUTPUTS = VOLT_OUTPUTS + NUM_OF_NODES
    };
    enum LightIds {
        PAUSE_LIGHT,
        NUM_LIGHTS
    };

    dsp::SchmittTrigger rndTrig, clearTrig, pauseTrig;
    Node *nodes = new Node[NUM_OF_NODES];
    std::vector<Particle> particles;
    int channels = 1;
    bool movement = false;
    bool pitchChoice = false;
    bool toggleStart = true;
    float clockStep;
    float maxConnectedDist = 150;
    float pulseSpeed = 1.0 / APP->engine->getSampleRate() * maxConnectedDist;
    int checkParams = 0;
    int moveNodes = 0;


    Neutrinode() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BPM_PARAM, 15, 120, 30, "Tempo", " bpm");
        configParam(PLAY_PARAM, 0.0, 1.0, 0.0, "Start nodes");
        configParam(MOVE_PARAM, 0.0, 1.0, 0.0, "Move nodes");
        configParam(SPEED_PARAM, -1.0, 1.0, 0.0, "Node speed");
        configParam(ROOT_NOTE_PARAM, 0.0, Quantize::NUM_OF_NOTES-1, 0.0, "Root note");
        configParam(SCALE_PARAM, 0.0, Quantize::NUM_OF_SCALES-1, 0.0, "Scale");
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
    }

    void process(const ProcessArgs &args) override {
        // checks BPM param knob every 4th sample
        if (checkParams == 0) {
            if (rndTrig.process(params[RND_PARTICLES_PARAM].getValue())) {
                randomizeParticles();
            }
            if (clearTrig.process(params[CLEAR_PARTICLES_PARAM].getValue())) {
                clearParticles();
            }
            if (pauseTrig.process(params[PLAY_PARAM].getValue())) {
                toggleStart = !toggleStart;
            }
            movement = params[MOVE_PARAM].getValue();
            pitchChoice = params[PITCH_PARAM].getValue();
            


            // clockStep = std::pow(2.0, params[BPM_PARAM].getValue());
            clockStep = params[BPM_PARAM].getValue() / 60.0;
            clockStep = (clockStep / args.sampleRate) / 2;
            pulseSpeed = clockStep * maxConnectedDist * 2;
        }
        checkParams = (checkParams+1) % 4;

        // TODO: This doesn't need to happen every sample, might try every 700 samples

        lights[PAUSE_LIGHT].value = toggleStart ? 1.0 : 0.0;

        if (movement) {
            if (moveNodes == 0) {
                updateNodePos();
            }
            moveNodes = (moveNodes+1) % static_cast<int>(args.sampleRate/60.0); // check 60 hz
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

                for (unsigned int j = 0; j < particles.size(); j++) {
                    Vec p = particles[j].box.getCenter();
                    nodes[i].sendPulse(p, j, pulseSpeed);
                    if (nodes[i].pulses[j].triggered) {
                        nodes[i].pulses[j].triggered = false;
                        nodes[i].gatePulse[j % channels].trigger(1e-3f);

                        float volts;
                        if (pitchChoice) volts = rescale(particles[j].box.pos.y, DISPLAY_SIZE, 0, 0, 1);
                        else volts = rescale(particles[j].radius, 5, 10, 1, 0);
                        float pitch = Quantize::quantizeRawVoltage(volts, rootNote, scale) + oct;
                        // outs
                        outputs[VOLT_OUTPUTS + i].setVoltage(pitch, (j % channels));
                        outputs[VOLTS_ALL_OUTPUTS].setVoltage(pitch, polyChannelIndex);
                    }
                    if (nodes[i].pulses[j].blipTrigger) nodes[i].pulses[j].blip();

                    bool pulse = nodes[i].gatePulse[j % channels].process(1.0 / args.sampleRate);
                    outputs[GATE_OUTPUTS + i].setVoltage(pulse ? 10.0 : 0.0, (j % channels));
                    outputs[GATES_ALL_OUTPUTS].setVoltage(pulse ? 10.0 : 0.0, polyChannelIndex);
                    polyChannelIndex = (polyChannelIndex+1) % channels;
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
        // loop all channels
        // for (int c = 0; c < channels; c++) {
        //     bool pulse = nodes[PURPLE_NODE].gatePulse[c].process(1.0 / args.sampleRate);
        //     outputs[VOLT_OUTPUTS + PURPLE_NODE].setVoltage(nodes[PURPLE_NODE].pitchVoltage[c], c);
        //     outputs[GATE_OUTPUTS + PURPLE_NODE].setVoltage(pulse ? 10.0 : 0.0, c);

        //     pulse = nodes[BLUE_NODE].gatePulse[c].process(1.0 / args.sampleRate);
        //     outputs[VOLT_OUTPUTS + BLUE_NODE].setVoltage(nodes[BLUE_NODE].pitchVoltage[c], c);
        //     outputs[GATE_OUTPUTS + BLUE_NODE].setVoltage(pulse ? 10.0 : 0.0, c);

        //     pulse = nodes[AQUA_NODE].gatePulse[c].process(1.0 / args.sampleRate);
        //     outputs[VOLT_OUTPUTS + AQUA_NODE].setVoltage(nodes[AQUA_NODE].pitchVoltage[c], c);
        //     outputs[GATE_OUTPUTS + AQUA_NODE].setVoltage(pulse ? 10.0 : 0.0, c);

        //     pulse = nodes[RED_NODE].gatePulse[c].process(1.0 / args.sampleRate);
        //     outputs[VOLT_OUTPUTS + RED_NODE].setVoltage(nodes[RED_NODE].pitchVoltage[c], c);
        //     outputs[GATE_OUTPUTS + RED_NODE].setVoltage(pulse ? 10.0 : 0.0, c);
        // }
    }

    void randomizeParticles() {
        if (particles.size() < 1) {
            for (int i = 0; i < 4; i++) {
                Particle p(randRange(15, DISPLAY_SIZE - 15), randRange(15, DISPLAY_SIZE - 15));
                particles.push_back(p);
                for (int i = 0; i < NUM_OF_NODES; i++) {
                    Pulse pulse;
                    nodes[i].pulses.push_back(pulse);
                }
            }
        } else {
            for (unsigned int i = 0; i < particles.size(); i++) {
                particles[i].box.pos.x = randRange(15, DISPLAY_SIZE - 15);
                particles[i].box.pos.y = randRange(15, DISPLAY_SIZE - 15);
            }
        }
    }

    void clearParticles() {
        particles.clear();
        for (int i = 0; i < NUM_OF_NODES; i++) 
            nodes[i].pulses.clear();
    }

    void updateNodePos() {
        // TODO
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
        module->channels = channels;
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
            for (unsigned int i = 0; i < module->particles.size(); i++) {
                Vec partPos = module->particles[i].box.getCenter();
                float d = dist(inits, partPos);
                float r = module->particles[i].radius;
                if (d < r) {
                    // module->particles.erase(module->particles.begin()+i);
                    module->particles[i].box.pos.x = initX;
                    module->particles[i].box.pos.y = initY;
                    module->particles[i].locked = false;
                    clickedOnObj = true;
                } else {
                    module->particles[i].locked = true;
                }
            }

            if (!clickedOnObj) {
                if (module->particles.size() < MAX_PARTICLES) {
                    Particle p(initX, initY);
                    p.locked = false;
                    module->particles.push_back(p);
                    for (int i = 0; i < NUM_OF_NODES; i++) {
                        Pulse pulse;
                        module->nodes[i].pulses.push_back(pulse);
                    }
                }
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
        for (unsigned int i = 0; i < module->particles.size(); i++) {
            if (!module->particles[i].locked) {
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
                module->particles.erase(module->particles.begin()+index);
                for (int i = 0; i < NUM_OF_NODES; i++) 
                    module->nodes[i].pulses.erase(module->nodes[i].pulses.begin() + index);
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
                    Node n = module->nodes[i];
                    for (unsigned int j = 0; j < module->particles.size(); j++) {
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
            for (unsigned int i = 0; i < module->particles.size(); i++) {
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
        NoteKnob *noteKnob = dynamic_cast<NoteKnob *>(createParamCentered<NoteKnob>(Vec(26.4, 117.5), module, Neutrinode::ROOT_NOTE_PARAM));
        LeftAlignedLabel* const noteLabel = new LeftAlignedLabel;
        noteLabel->box.pos = Vec(42.6, 121);
        noteLabel->text = "";
        noteKnob->connectLabel(noteLabel, module);
        addChild(noteLabel);
        addParam(noteKnob);

        ScaleKnob *scaleKnob = dynamic_cast<ScaleKnob *>(createParamCentered<ScaleKnob>(Vec(26.4, 146), module, Neutrinode::SCALE_PARAM));
        LeftAlignedLabel* const scaleLabel = new LeftAlignedLabel;
        scaleLabel->box.pos = Vec(42.6, 149.5);
        scaleLabel->text = "";
        scaleKnob->connectLabel(scaleLabel, module);
        addChild(scaleLabel);
        addParam(scaleKnob);

        // addParam(createParamCentered<PurpleInvertKnob>(Vec(26.4, 136.7), module, Neutrinode::ROOT_NOTE_PARAM));
        // addParam(createParamCentered<PurpleInvertKnob>(Vec(58.8, 136.7), module, Neutrinode::SCALE_PARAM));
        addParam(createParamCentered<Jeremy_HSwitch>(Vec(91.5, 130.7), module, Neutrinode::PITCH_PARAM));

        addParam(createParamCentered<PurpleButton>(Vec(130.7, 130.7), module, Neutrinode::CLEAR_PARTICLES_PARAM));
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

    void appendContextMenu(Menu *menu) override {
        Neutrinode *module = dynamic_cast<Neutrinode*>(this->module);

        ChannelItem *channelItem = new ChannelItem;
        channelItem->text = "Polyphony channels";
        channelItem->rightText = string::f("%d", module->channels) + " " + RIGHT_ARROW;
        channelItem->module = module;
        menu->addChild(channelItem);
    }
};

Model *modelNeutrinode = createModel<Neutrinode, NeutrinodeWidget>("Neutrinode");