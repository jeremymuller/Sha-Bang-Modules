#include "plugin.hpp"

#define NUM_OF_NODES 4
#define MAX_PARTICLES 20

struct Particle {
    Rect box;
    NVGcolor color = nvgRGB(255, 255, 255);
    float radius;
    bool locked = true;
    bool visible = true;

    Particle(float _x, float _y) {
        box.pos.x = _x;
        box.pos.y = _y;
        radius = random::uniform() * 5 + 5;
    }
};

struct Pulse {
    Rect box;
    float radius = 3;
    // float radius = 6;
    bool visible = false;
    bool isConnected = false;
    float inc = 0.0;

    Pulse() {}
};

struct Node {
    Rect box;
    NVGcolor color;
    NVGcolor particleColor;
    std::vector<Pulse> pulses;
    float lineAlpha;
    float lineWidth;
    int maxConnectedDist = 150;
    int currentConnects = 0;
    float phase;
    float tempoTime;
    int nodeTempo = maxConnectedDist * 2;
    float pulseSpeed = 1.0 / 44100 * 60;
    bool locked = true;
    bool visible = true;
    bool start = true;
    bool triggered = false;

    Node() {
        box.pos.x = 30;
        box.pos.y = 30;

        // for (unsigned int i = 0; i < 25; i++) {
        //     Pulse p;
        //     pulses.push_back(p);
        // }

        // tempoTime = 0;
        tempoTime = static_cast<int>(random::uniform() * maxConnectedDist);
        phase = static_cast<int>(random::uniform() * maxConnectedDist);
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

    void sendPulse(Vec particle, int index) {
        if (start) {
            Pulse *pulse = &pulses[index];
            if (pulse == NULL) return;

            // if (triggered) {
            //     triggered = false;

            //     pulse->visible = true;
            //     pulse->inc = dist(particle, box.pos);
            // }

            if (tempoTime == 0) {
                triggered = true;

                pulse->visible = true;
                pulse->inc = dist(particle, box.pos);
            }

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
            }

        }
    }
};

struct Neutrinode : Module {
    enum ParamIds {
        BPM_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    Node *nodes = new Node[NUM_OF_NODES];
    std::vector<Particle> particles;

    Neutrinode() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BPM_PARAM, -2.0, 6.0, 0.0, "Tempo", " bpm", 2.0, 60.0);

        nodes[0].color = nvgRGBA(128, 0, 219, 255);
        nodes[1].color = nvgRGBA(38, 0, 255, 255);
        nodes[2].color = nvgRGBA(0, 238, 219, 255);
        nodes[3].color = nvgRGBA(255, 0, 0, 255);
        nodes[0].box.pos = Vec(30, 30);
        nodes[1].box.pos = Vec(50, 50);
        nodes[2].box.pos = Vec(70, 70);
        nodes[3].box.pos = Vec(90, 90);
    }

    ~Neutrinode() {
        delete[] nodes;
    }

    // void addParticle(float x, float y) {
    //     Particle 
    // }

    void process(const ProcessArgs &args) override {
        // TODO: gotta put all the vector logic here...framerate messes everything up


        // float clockTime = std::pow(2.0, params[BPM_PARAM].getValue());
        // float clockTime = 1;
        // clockTime /= 2.0;
        // // phase += clockTime * 0.5 * args.sampleTime;

        // // reset stuff here
        // for (int i = 0; i < NUM_OF_NODES; i++) {
        //     nodes[i].phase += clockTime * args.sampleTime;

        //     if (nodes[i].phase >= 1.0) {
        //         nodes[i].phase = 0;
        //         nodes[i].triggered = true;
        //     }

        // }

        // TODO: figure out how to calculatepulsespeed

        for (int i = 0; i < NUM_OF_NODES; i++) {
            if (nodes[i].visible) {
                for (unsigned int j = 0; j < particles.size(); j++) {
                    Vec p = particles[j].box.getCenter();
                    nodes[i].sendPulse(p, j);
                }

                if (nodes[i].start) {
                    nodes[i].tempoTime += nodes[i].pulseSpeed;
                    if (nodes[i].tempoTime > nodes[i].nodeTempo)
                        nodes[i].tempoTime = 0;
                }
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
            if (module->nodes[index].box.pos.x < 15.5) 
                module->nodes[index].box.pos.x = 15.5;
            else if (module->nodes[index].box.pos.x > box.size.x-15.5) 
                module->nodes[index].box.pos.x = box.size.x-15.5;
            // y's
            if (module->nodes[index].box.pos.y < 15.5)
                module->nodes[index].box.pos.y = 15.5;
            else if (module->nodes[index].box.pos.y > box.size.y - 15.5)
                module->nodes[index].box.pos.y = box.size.y - 15.5;
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

                    // display node
                    nvgStrokeColor(args.vg, module->nodes[i].color);
                    nvgStrokeWidth(args.vg, 2);
                    nvgBeginPath(args.vg);
                    Vec pos = module->nodes[i].box.getCenter();
                    nvgCircle(args.vg, pos.x, pos.y, 15.5);
                    nvgStroke(args.vg);

                    nvgFillColor(args.vg, module->nodes[i].color);
                    nvgBeginPath(args.vg);
                    nvgCircle(args.vg, pos.x, pos.y, 12);
                    nvgFill(args.vg);


                    Node n = module->nodes[i];
                    for (unsigned int j = 0; j < module->particles.size(); j++) {
                        Vec p = module->particles[j].box.getCenter();
                        if (module->nodes[i].connected(p, j)) {
                            nvgStrokeColor(args.vg, nvgTransRGBA(module->nodes[i].color, module->nodes[i].lineAlpha));
                            nvgStrokeWidth(args.vg, module->nodes[i].lineWidth);
                            nvgBeginPath(args.vg);
                            nvgMoveTo(args.vg, module->nodes[i].box.pos.x, module->nodes[i].box.pos.y);
                            nvgLineTo(args.vg, p.x, p.y);
                            nvgStroke(args.vg);
                        }

                        if (module->nodes[i].start) {
                            Pulse p = module->nodes[i].pulses[j];
                            if (p.visible && p.isConnected) {
                                nvgFillColor(args.vg, nvgTransRGBA(module->nodes[i].color, 200));
                                nvgBeginPath(args.vg);
                                // Vec pulsePos = module->nodes[i].pulses[j].box.getCenter();
                                nvgCircle(args.vg, p.box.pos.x, p.box.pos.y, p.radius);
                                nvgFill(args.vg);
                            }
                        }
                    }

                    // if (module->nodes[i].start) {
                    //     module->nodes[i].tempoTime += module->nodes[i].pulseSpeed;
                    //     if (module->nodes[i].tempoTime > module->nodes[i].nodeTempo)
                    //         module->nodes[i].tempoTime = 0;
                    // }
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
        display->box.pos = Vec(125.4, 8.8);
        display->box.size = Vec(362, 362);
        addChild(display);

    }
};

Model *modelNeutrinode = createModel<Neutrinode, NeutrinodeWidget>("Neutrinode");