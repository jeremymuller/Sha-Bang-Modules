#include "plugin.hpp"

#define NUM_OF_NODES 4
#define MAX_PARTICLES 20
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
        // radius = random::uniform() * 5 + 5;
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
    float inc = 0.0;

    Pulse() {}
};

struct Node {
    Rect box;
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
    float pitchVoltage = 0.0;
    dsp::SchmittTrigger toggleTrig;
    dsp::PulseGenerator gatePulse;

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

        haloRadius += 18.0 / 44100; // TODO: figure out using sample rate
        haloTime++;

    }
};

struct Neutrinode : Module {
    enum NodeIds {
        PURPLE_NODE,
        BLUE_NODE,
        AQUA_NODE,
        RED_NODE,
        NUM_NODES
    };
    enum ParamIds {
        BPM_PARAM,
        PAUSE_PARAM,
        ON_PARAM = NUM_OF_NODES,
        NUM_PARAMS = ON_PARAM + NUM_OF_NODES
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        GATE_OUTPUTS = NUM_OF_NODES,
        VOLT_OUTPUTS = GATE_OUTPUTS + NUM_OF_NODES,
        NUM_OUTPUTS = VOLT_OUTPUTS + NUM_OF_NODES
    };
    enum LightIds {
        NUM_LIGHTS
    };

    Node *nodes = new Node[NUM_OF_NODES];
    std::vector<Particle> particles;
    float clockStep;
    float maxConnectedDist = 150;
    float pulseSpeed = 1.0 / 44100 * maxConnectedDist;
    int checkParams = 0;

    Neutrinode() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BPM_PARAM, -2.0, 6.0, 0.0, "Tempo", " bpm", 2.0, 60.0);
        configParam(PAUSE_PARAM, 0.0, 1.0, 1.0);
        configParam(ON_PARAM + PURPLE_NODE, 0.0, 1.0, 0.0, "toggle purple node");
        configParam(ON_PARAM + BLUE_NODE, 0.0, 1.0, 0.0, "toggle blue node");
        configParam(ON_PARAM + AQUA_NODE, 0.0, 1.0, 0.0, "toggle aqua node");
        configParam(ON_PARAM + RED_NODE, 0.0, 1.0, 0.0, "toggle red node");

        nodes[0].color = nvgRGBA(128, 0, 219, 255);
        nodes[1].color = nvgRGBA(38, 0, 255, 255);
        nodes[2].color = nvgRGBA(0, 238, 219, 255);
        nodes[3].color = nvgRGBA(255, 0, 0, 255);
        nodes[0].lineColor = nvgRGB(213, 153, 255);
        nodes[1].lineColor = nvgRGB(165, 152, 255);
        nodes[2].lineColor = nvgRGB(104, 245, 255);
        nodes[3].lineColor = nvgRGB(255, 101, 101);
        // TODO: this is not that great
        nodes[0].box.pos = Vec(randRange(DISPLAY_SIZE/2.0), randRange(DISPLAY_SIZE/2.0));
        nodes[1].box.pos = Vec(randRange(DISPLAY_SIZE/2.0, DISPLAY_SIZE), randRange(DISPLAY_SIZE/2.0));
        nodes[2].box.pos = Vec(randRange(DISPLAY_SIZE / 2.0, DISPLAY_SIZE / 2.0), randRange(DISPLAY_SIZE / 2.0, DISPLAY_SIZE / 2.0));
        nodes[3].box.pos = Vec(randRange(DISPLAY_SIZE/2.0), randRange(DISPLAY_SIZE/2.0, DISPLAY_SIZE));
    }

    ~Neutrinode() {
        delete[] nodes;
    }

    void process(const ProcessArgs &args) override {
        // checks BPM param knob every 4th sample
        if (checkParams == 0) {
            clockStep = std::pow(2.0, params[BPM_PARAM].getValue());
            clockStep = (clockStep / args.sampleRate) / 2;
            pulseSpeed = clockStep * maxConnectedDist * 2;
        }
        checkParams = (checkParams+1) % 4;

        // TODO: This doesn't need to happen every sample, might try every few samples

        for (int i = 0; i < NUM_OF_NODES; i++) {
            bool s = params[PAUSE_PARAM].getValue();
            nodes[i].start = s;


            if (nodes[i].toggleTrig.process(params[ON_PARAM+i].getValue())) {
                nodes[i].start = !nodes[i].start;
                nodes[i].visible = !nodes[i].visible;
            }

            // TODO: clean this up
            if (nodes[i].visible && nodes[i].start) {

                for (unsigned int j = 0; j < particles.size(); j++) {
                    Vec p = particles[j].box.getCenter();
                    nodes[i].sendPulse(p, j, pulseSpeed);
                    if (nodes[i].pulses[j].triggered) {
                        nodes[i].pulses[j].triggered = false;
                        nodes[i].gatePulse.trigger(1e-3f);
                        nodes[i].pitchVoltage = rescale(particles[j].radius, 5, 10, 5, -5);
                    }
                }

                if (nodes[i].start) {
                    nodes[i].phase += clockStep;
                    if (nodes[i].triggered) nodes[i].blip();
                    if (nodes[i].phase > 1.0) {
                        nodes[i].phase = 0;
                        nodes[i].triggered = true;
                    }
                }

                bool pulse = nodes[i].gatePulse.process(1.0 / args.sampleRate);
                outputs[GATE_OUTPUTS + i].setVoltage(pulse ? 10.0 : 0.0);
                outputs[VOLT_OUTPUTS + i].setVoltage(nodes[i].pitchVoltage);
            }
        }
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
                Vec nodePos = module->nodes[i].box.getCenter();
                float d = dist(inits, nodePos);
                if (d < 16) {
                    module->nodes[i].box.pos.x = initX;
                    module->nodes[i].box.pos.y = initY;
                    module->nodes[i].locked = false;
                    clickedOnObj = true;
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
            if (module->nodes[index].box.pos.x < 16) 
                module->nodes[index].box.pos.x = 16;
            else if (module->nodes[index].box.pos.x > box.size.x-16) 
                module->nodes[index].box.pos.x = box.size.x-16;
            // y's
            if (module->nodes[index].box.pos.y < 16)
                module->nodes[index].box.pos.y = 16;
            else if (module->nodes[index].box.pos.y > box.size.y - 16)
                module->nodes[index].box.pos.y = box.size.y - 16;
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

            // TODO: erase pulses?
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
            // draw particles
            for (unsigned int i = 0; i < module->particles.size(); i++) {
                Vec pos = module->particles[i].box.getCenter();
                nvgFillColor(args.vg, module->particles[i].color);
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, module->particles[i].radius);
                nvgFill(args.vg);

                nvgFillColor(args.vg, nvgRGB(0, 0, 0));
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, pos.x, pos.y, 2.5);
                nvgFill(args.vg);
            }

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
                            Pulse pulse = module->nodes[i].pulses[j];
                            if (pulse.visible && pulse.isConnected) {
                                nvgFillColor(args.vg, nvgTransRGBA(module->nodes[i].color, 200));
                                nvgBeginPath(args.vg);
                                // Vec pulsePos = module->nodes[i].pulses[j].box.getCenter();
                                nvgCircle(args.vg, pulse.box.pos.x, pulse.box.pos.y, pulse.radius);
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


        addChild(createWidget<JeremyScrew>(Vec(74.6, 2)));
        addChild(createWidget<JeremyScrew>(Vec(74.6, box.size.y - 14)));
        // addChild(createWidget<JeremyScrew>(Vec(431, 2)));
        // addChild(createWidget<JeremyScrew>(Vec(431, box.size.y - 14)));

        addParam(createParamCentered<PurpleKnob>(Vec(80.6, 54.9), module, Neutrinode::BPM_PARAM));
        addParam(createParamCentered<PauseButton>(Vec(134, 54.9), module, Neutrinode::PAUSE_PARAM));

        // toggles
        addParam(createParamCentered<TinyPurpleButton>(Vec(41.6, 104.8), module, Neutrinode::ON_PARAM + Neutrinode::PURPLE_NODE));
        addParam(createParamCentered<TinyBlueButton>(Vec(73.9, 104.8), module, Neutrinode::ON_PARAM + Neutrinode::BLUE_NODE));
        addParam(createParamCentered<TinyAquaButton>(Vec(106.2, 104.8), module, Neutrinode::ON_PARAM + Neutrinode::AQUA_NODE));
        addParam(createParamCentered<TinyRedButton>(Vec(138.5, 104.8), module, Neutrinode::ON_PARAM + Neutrinode::RED_NODE));

        // gate outputs
        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(41.6, 292.8), module, Neutrinode::GATE_OUTPUTS + Neutrinode::PURPLE_NODE));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(73.9, 292.8), module, Neutrinode::GATE_OUTPUTS + Neutrinode::BLUE_NODE));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(106.2, 292.8), module, Neutrinode::GATE_OUTPUTS + Neutrinode::AQUA_NODE));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(138.5, 292.8), module, Neutrinode::GATE_OUTPUTS + Neutrinode::RED_NODE));
        // volt outputs
        addOutput(createOutputCentered<TinyPJ301MPurple>(Vec(41.6, 314.1), module, Neutrinode::VOLT_OUTPUTS + Neutrinode::PURPLE_NODE));
        addOutput(createOutputCentered<TinyPJ301MBlue>(Vec(73.9, 314.1), module, Neutrinode::VOLT_OUTPUTS + Neutrinode::BLUE_NODE));
        addOutput(createOutputCentered<TinyPJ301MAqua>(Vec(106.2, 314.1), module, Neutrinode::VOLT_OUTPUTS + Neutrinode::AQUA_NODE));
        addOutput(createOutputCentered<TinyPJ301MRed>(Vec(138.5, 314.1), module, Neutrinode::VOLT_OUTPUTS + Neutrinode::RED_NODE));
    }
};

Model *modelNeutrinode = createModel<Neutrinode, NeutrinodeWidget>("Neutrinode");