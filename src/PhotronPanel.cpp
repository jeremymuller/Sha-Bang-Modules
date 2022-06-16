#include "Photron.hpp"

#define DISPLAY_SIZE_WIDTH 75
#define DISPLAY_SIZE_HEIGHT 380
#define CELL_SIZE 5 // 5? or 10?
#define NUM_OF_MARCHING_CIRCLES 5

struct PhotronPanel : Module {
    enum ModeIds {
        COLOR,
        B_W,
        SINGLE_COLOR,
        STRIP_COLOR,
        NUM_MODES
    };
    enum ParamIds {
        RANDOMIZE_PARAM,
        RESET_PARAM,
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

    dsp::SchmittTrigger colorTrig, invertTrig, resetTrig;
    int width = 4;
    ModeIds colorMode = COLOR;
    bool isColor = true;
    bool darkRoomBlobs = true;
    float hue = 0.5;
    float pulsePhase = 0.0;
    // float pulseHz[8] = {0.0, 0.1, 0.2, 0.25, 0.33, 0.4, 0.5, 1.0};
    float pulseHz = 0.5;
    // int srIncrement = static_cast<int>(APP->engine->getSampleRate() / INTERNAL_HZ);
    int hertzIndex = 2;
    int hertz[7] = {60, 45, 30, 20, 15, 12, 10};
    int internalHz = 30;
    float srIncrement = APP->engine->getSampleTime() * internalHz;
    float sr = 0;
    float sep = 0.1;
    float ali = 0.0;
    float coh = 0.0;
    float cellScale = 1.0;
    static const int cols = DISPLAY_SIZE_WIDTH / CELL_SIZE;
    static const int rows = DISPLAY_SIZE_HEIGHT / CELL_SIZE;
    Block blocks[rows][cols];
    int blockAlpha[rows][cols];
    MarchingCircle circles[NUM_OF_MARCHING_CIRCLES];
    // expander stuff
    BlockMessage outputValues[rows];
    BlockMessage leftMessages[2][rows];
    Block rightOutputValues[rows];
    Block rightMessages[2][rows];

    PhotronPanel() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Block b(x*CELL_SIZE, y*CELL_SIZE, CELL_SIZE);
                blocks[y][x] = b;
                blocks[y][x].isSet = true;

                blockAlpha[y][x] = 0;
            }
        }

        for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            MarchingCircle c(randRange(DISPLAY_SIZE_WIDTH), randRange(DISPLAY_SIZE_HEIGHT), randRange(10.0, 35.0));
            c.setSize(DISPLAY_SIZE_WIDTH, DISPLAY_SIZE_HEIGHT);
            c.velLimit = 0.5;
            circles[i] = c;
        }

        resetBlocks(PhotronPanel::RESET_PARAM);

        leftExpander.producerMessage = leftMessages[0];
        leftExpander.consumerMessage = leftMessages[1];

        rightExpander.producerMessage = rightMessages[0];
        rightExpander.consumerMessage = rightMessages[1];
    }

    void onSampleRateChange() override {
        srIncrement = APP->engine->getSampleTime() * internalHz;
    }

    void onRandomize() override {
        resetBlocks(PhotronPanel::RANDOMIZE_PARAM);

        for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            circles[i].radius = randRange(10.0, 35.0);
        }
    }

    void onReset() override {
        resetBlocks(PhotronPanel::RESET_PARAM);
    }

    void setHz(int hz) {
        hertzIndex = hz;
        internalHz = hertz[hertzIndex];
        srIncrement = APP->engine->getSampleTime() * internalHz;
    }

    int getHz() {
        return hertzIndex;
    }

    void incrementColorMode() {
        switch (colorMode) {
        case COLOR:
            colorMode = B_W;
            break;
        case B_W:
            colorMode = SINGLE_COLOR;
            break;
        case SINGLE_COLOR:
            colorMode = STRIP_COLOR;
            break;
        default:
            colorMode = COLOR;
            break;
        }
    }

    json_t *dataToJson() override {
        // stores whether it's black and white
        // stores current RGB of each block
        json_t *rootJ = json_object();

        json_t *blocksJ = json_array();
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                json_t *rgbJ = json_array();
                Vec3 rgb = blocks[y][x].rgb;

                json_t *redJ = json_integer(rgb.x);
                json_t *greenJ = json_integer(rgb.y);
                json_t *blueJ = json_integer(rgb.z);

                json_array_append_new(rgbJ, redJ);
                json_array_append_new(rgbJ, greenJ);
                json_array_append_new(rgbJ, blueJ);

                json_array_append_new(blocksJ, rgbJ);
            }
        }

        // json_object_set_new(rootJ, "internalHz", json_integer(internalHz));
        json_object_set_new(rootJ, "hertzIndex", json_integer(getHz()));
        json_object_set_new(rootJ, "blobs", json_boolean(darkRoomBlobs));
        json_object_set_new(rootJ, "width", json_integer(width));
        json_object_set_new(rootJ, "color", json_integer(colorMode));
        json_object_set_new(rootJ, "hue", json_real(hue));
        json_object_set_new(rootJ, "pulsePhase", json_real(pulsePhase));
        json_object_set_new(rootJ, "pulseHz", json_real(pulseHz));
        json_object_set_new(rootJ, "blocks", blocksJ);
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        json_t *hertzIndexJ = json_object_get(rootJ, "hertzIndex");
        if (hertzIndexJ) setHz(json_integer_value(hertzIndexJ));

        json_t *pulsePhaseJ = json_object_get(rootJ, "pulsePhase");
        if (pulsePhaseJ) pulsePhase = json_real_value(pulsePhaseJ);

        json_t *pulseHzJ = json_object_get(rootJ, "pulseHz");
        if (pulseHzJ) pulseHz = json_real_value(pulseHzJ);

        json_t *colorJ = json_object_get(rootJ, "color");
        if (colorJ) colorMode = (ModeIds)json_integer_value(colorJ);

        json_t *blobsJ = json_object_get(rootJ, "blobs");
        if (blobsJ) darkRoomBlobs = json_boolean_value(blobsJ);

        json_t *hueJ = json_object_get(rootJ, "hue");
        if (hueJ) hue = json_real_value(hueJ);

        json_t *widthJ = json_object_get(rootJ, "width");
        if (widthJ) width = json_integer_value(widthJ);

        json_t *blocksJ = json_object_get(rootJ, "blocks");
        if (blocksJ) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    int i = x + y * cols;
                    json_t *rgbJ = json_array_get(blocksJ, i);
                    if (rgbJ) {
                        json_t *redJ = json_array_get(rgbJ, 0);
                        json_t *greenJ = json_array_get(rgbJ, 1);
                        json_t *blueJ = json_array_get(rgbJ, 2);
                        if (redJ) blocks[y][x].rgb.x = json_integer_value(redJ);
                        if (greenJ) blocks[y][x].rgb.y = json_integer_value(greenJ);
                        if (blueJ) blocks[y][x].rgb.z = json_integer_value(blueJ);
                    }
                }
            }
        }
    }

    void process(const ProcessArgs &args) override {

        if (sr == 0) {

            bool isParent = (leftExpander.module && (leftExpander.module->model == modelPhotronPanel || leftExpander.module->model == modelPhotronStrip));
            if (isParent) {
                BlockMessage *outputFromParent = (BlockMessage *)(leftExpander.consumerMessage);
                memcpy(outputValues, outputFromParent, sizeof(BlockMessage) * rows);

                setHz(outputValues[0].hertzIndex);
                colorMode = (ModeIds)outputValues[0].colorMode;
            }

            bool isRightExpander = (rightExpander.module && (rightExpander.module->model == modelPhotronPanel || rightExpander.module->model == modelPhotronStrip));
            if (isRightExpander) {
                Block *outputFromRight = (Block *)(rightExpander.consumerMessage);
                memcpy(rightOutputValues, outputFromRight, sizeof(Block) * rows);
            }

            int edge = width * RACK_GRID_WIDTH / CELL_SIZE;

            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    blocks[y][x].sepInput = PhotronPanel::sep;
                    // blocks[y][x].aliInput = PhotronPanel::ali;
                    // blocks[y][x].cohInput = PhotronPanel::coh;

                    // adjacents
                    Block west;
                    Block east;
                    Block north;
                    Block south;
                    if (isParent && x == 0)
                        west = outputValues[y].block;
                    else if (x > 0)
                        west = blocks[y][x - 1];

                    if (isRightExpander && x == edge-1)
                        east = rightOutputValues[y];
                    else if (x < cols-1) 
                        east = blocks[y][x+1];

                    if (y > 0) north = blocks[y-1][x];
                    if (y < rows-1) south = blocks[y+1][x];

                    // corners
                    Block northwest;
                    Block northeast;
                    Block southwest;
                    Block southeast;

                    if (isParent && (x == 0) && (y > 0))
                        northwest = outputValues[y - 1].block;
                    else if ((x > 0) && (y > 0))
                        northwest = blocks[y - 1][x - 1];

                    if (isRightExpander && (x == edge-1) && (y > 0))
                        northeast = rightOutputValues[y-1];
                    else if ((x < cols-1) && (y > 0)) 
                        northeast = blocks[y-1][x+1];

                    if (isParent && (x == 0) && (y < rows - 1))
                        southwest = outputValues[y + 1].block;
                    else if ((y < rows - 1) && (x > 0))
                        southwest = blocks[y + 1][x - 1];

                    if (isRightExpander && (x == edge-1) && (y < rows-1))
                        southeast = rightOutputValues[y+1];
                    else if ((y < rows-1) && (x < cols-1)) 
                        southeast = blocks[y+1][x+1];

                    Block b[8] = {west, east, north, south, northwest, northeast, southwest, southeast};
                    blocks[y][x].flock(b, 8);
                    blocks[y][x].update();

                    blockAlpha[y][x] = calculateCell(blocks[y][x].getCenter());
                }
            }

            for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
                circles[i].update();
            }

            // to expander (right side)
            if (rightExpander.module && (rightExpander.module->model == modelPhotronPanel || rightExpander.module->model == modelPhotronStrip)) {
                BlockMessage *messageToExpander = (BlockMessage *)(rightExpander.module->leftExpander.producerMessage);

                // int edge = width * RACK_GRID_WIDTH / CELL_SIZE;

                messageToExpander[0].hertzIndex = getHz();
                messageToExpander[0].colorMode = (int)colorMode;

                for (int y = 0; y < rows; y++) {
                    messageToExpander[y].block = blocks[y][edge-1];
                }

                rightExpander.module->leftExpander.messageFlipRequested = true;
            }

            // to expander (left side to parent)
            if (leftExpander.module && (leftExpander.module->model == modelPhotronPanel || leftExpander.module->model == modelPhotronStrip)) {
                Block *messageToExpander = (Block *)(leftExpander.module->rightExpander.producerMessage);

                for (int y = 0; y < rows; y++) {
                    messageToExpander[y] = blocks[y][0];
                }

                leftExpander.module->rightExpander.messageFlipRequested = true;
            }
        }
        sr += srIncrement;
        if (sr >= 1.0) {
            sr = 0.0;
        }

        pulsePhase += args.sampleTime * pulseHz;
        if (pulsePhase >= 1.0)
            pulsePhase -= 1.0;
    }

    float getPulsePhase() {
        if (pulseHz > 0) {
            float x = std::sin(2.0 * M_PI * pulsePhase);
            return rescale(x, -1.0, 1.0, 0.4, 1.0);
        } else
            return 1.0;
    }

    void resetBlocks(int param) {
        if (param == PhotronPanel::RANDOMIZE_PARAM) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    blocks[y][x].reset();
                }
            }
        } else if (param == PhotronPanel::RESET_PARAM) {
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    if (y < rows/4.0)
                        blocks[y][x].setColor(128, 0, 219); // purple
                    else if (y < rows/2.0)
                        blocks[y][x].setColor(38, 0, 255); // blue
                    else if (y < rows/2.0 + rows/4.0)
                        blocks[y][x].setColor(0, 238, 255); // aqua
                    else
                        blocks[y][x].setColor(255, 0, 0); // red
                }
            }          
        }
    }

    int calculateCell(Vec blockPos) {
        float sum = 0.0;
        for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            float r = circles[i].radius * 0.9;
            float d = (blockPos.x - circles[i].pos.x) * (blockPos.x - circles[i].pos.x) + (blockPos.y - circles[i].pos.y) * (blockPos.y - circles[i].pos.y);
            d = std::fmax(d, 0.001); // to prevent dividing by zero
            sum += (r * r) / d;
        }

        if (sum >= 1) {
            return 255;
        } else {
            sum = sum * sum;
            float newSum = rescale(sum, 0.2, 1, 0, 254);
            sum = clamp(newSum, 0.0, 255.0);
            return (int)sum;
        }
    }
};

struct PhotronPanelDisplay : Widget {
    PhotronPanel *module;
    float initX = 0;
    float initY = 0;
    float dragX = 0;
    float dragY = 0;
    bool shiftClick = false;

    void onButton(const event::Button &e) override {
        if (module == NULL) return;

        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
            if ((e.mods & RACK_MOD_MASK) == GLFW_MOD_SHIFT) {
                e.consume(this);
                shiftClick = true;
                // module->incrementColorMode();
                initX = e.pos.x;
                initY = e.pos.y;
            } else {
                shiftClick = false;
            }
        }
    }

    void onDragStart(const event::DragStart &e) override {
        dragX = APP->scene->rack->getMousePos().x;
        dragY = APP->scene->rack->getMousePos().y;
    }

    void onDragMove(const event::DragMove &e) override {
    	if (shiftClick) {
            float newDragX = APP->scene->rack->getMousePos().x;
            float newDragY = APP->scene->rack->getMousePos().y;
            // TODO
            drawRandoms(initX + (newDragX - dragX), initY + (newDragY - dragY));
        }
    }

    void drawRandoms(float mX, float mY) {
        int x = static_cast<int>(mX / CELL_SIZE);
        int y = static_cast<int>(mY / CELL_SIZE);

        module->blocks[y][x].reset();

        int edge = module->width * RACK_GRID_WIDTH / CELL_SIZE;

        if (x > 0) module->blocks[y][x-1].reset();
        if (x < edge-1) module->blocks[y][x+1].reset();
        if (y > 0) module->blocks[y-1][x].reset();
        if (x < module->cols-1) module->blocks[y+1][x].reset();
    }

    void drawSingleColor(const DrawArgs &args) {
        nvgFillColor(args.vg, nvgHSL(module->hue, 1.0, module->getPulsePhase() * 0.5));

        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFill(args.vg);
    }

    void drawStripColor(const DrawArgs &args) {
        nvgFillColor(args.vg, nvgRGB(0.0, 0.0, 0.0));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, box.pos.x, box.pos.y, box.size.x, box.size.y);
        nvgFill(args.vg);

        nvgFillColor(args.vg, nvgHSL(module->hue, 1.0, module->getPulsePhase() * 0.5));
        nvgBeginPath(args.vg);
        nvgRect(args.vg, box.pos.x + CELL_SIZE, box.pos.y + CELL_SIZE, box.size.x - CELL_SIZE*2, box.size.y - CELL_SIZE*2);
        nvgFill(args.vg);
    }

    void draw(const DrawArgs &args) override {
        if (module == NULL) return;

        if (module->colorMode == PhotronPanel::SINGLE_COLOR) {
            drawSingleColor(args);
        } else if (module->colorMode == PhotronPanel::STRIP_COLOR) {
            drawStripColor(args);
        } else {
            for (int y = 0; y < DISPLAY_SIZE_HEIGHT/CELL_SIZE; y++) {
                for (int x = 0; x < box.size.x/CELL_SIZE; x++) {
                    Vec3 rgb = module->blocks[y][x].rgb;
                    if (module->colorMode == PhotronPanel::COLOR) {
                        nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.y, rgb.z));
                    } else {
                        // NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                        nvgFillColor(args.vg, nvgRGB(rgb.x, rgb.x, rgb.x));
                    }

                    nvgBeginPath(args.vg);
                    nvgRect(args.vg, module->blocks[y][x].pos.x, module->blocks[y][x].pos.y, CELL_SIZE, CELL_SIZE);
                    nvgFill(args.vg);
                }
            }
        }

    }

    void drawLayer(const DrawArgs &args, int layer) override {
        if (module == NULL) return;

        if (layer == 1) {
            //background
            // nvgFillColor(args.vg, nvgRGB(40, 40, 40));
            // nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            // nvgBeginPath(args.vg);
            // nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
            // nvgFill(args.vg);

            if (module->colorMode == PhotronPanel::SINGLE_COLOR) {
                drawSingleColor(args);
            } else if (module->colorMode == PhotronPanel::STRIP_COLOR) {
                drawStripColor(args);
            } else {
                for (int y = 0; y < DISPLAY_SIZE_HEIGHT/CELL_SIZE; y++) {
                    for (int x = 0; x < box.size.x/CELL_SIZE; x++) {
                        Vec3 rgb = module->blocks[y][x].rgb;
                        bool isBlobs = module->darkRoomBlobs;
                        if (module->colorMode == PhotronPanel::COLOR) {
                            nvgFillColor(args.vg, nvgRGBA(rgb.x, rgb.y, rgb.z, isBlobs ? module->blockAlpha[y][x] : 255));
                        } else {
                            // NVGcolor color = nvgRGB(rgb.x, rgb.x, rgb.x);
                            // nvgFillColor(args.vg, nvgTransRGBA(color, rgb.y));
                            nvgFillColor(args.vg, nvgRGBA(rgb.x, rgb.x, rgb.x, isBlobs ? module->blockAlpha[y][x] : 255));
                        }
                        nvgBeginPath(args.vg);
                        nvgRect(args.vg, module->blocks[y][x].pos.x, module->blocks[y][x].pos.y, CELL_SIZE, CELL_SIZE);
                        nvgFill(args.vg);
                    }
                }
            }


            // // draw green circles for debugging
            // for (int i = 0; i < NUM_OF_MARCHING_CIRCLES; i++) {
            //     Vec circle = module->circles[i].getPos();
            //     float cRadius = module->circles[i].getRadius();

            //     nvgStrokeColor(args.vg, nvgRGB(0, 255, 0));
            //     nvgBeginPath(args.vg);
            //     nvgCircle(args.vg, circle.x, circle.y, cRadius);
            //     nvgStroke(args.vg);
            // }
        }
        Widget::drawLayer(args, layer);

    }
};

struct LightMenuItem : MenuItem {
    PhotronPanel *module;
    float *pulseHz = NULL;
    float *hue = NULL;

    Menu *createChildMenu() override {
        Menu *menu = new Menu;

        struct LightPulseQuantity : Quantity {
            float *pulseHz;

            LightPulseQuantity(float *_pulseHz) {
                pulseHz = _pulseHz;
            }

            void setValue(float value) override {
                *pulseHz = clamp(value, getMinValue(), getMaxValue());
            }

            float getValue() override {
                return *pulseHz;
            }

            float getDefaultValue() override { return 0.5; }
            float getMinValue() override { return 0.0; }
            float getMaxValue() override { return 2.0; }

            std::string getDisplayValueString() override {
                return string::f("%.1f Hz", clamp(*pulseHz, getMinValue(), getMaxValue()));
            }

            void setDisplayValue(float value) override {
                setValue(value);
            }

            std::string getLabel() override {
                return "Pulse";
            }
        };
        struct LightPulseSlider : ui::Slider {
            LightPulseSlider(float *pulseHz) {
                quantity = new LightPulseQuantity(pulseHz);
            }
            ~LightPulseSlider() {
                delete quantity;
            }
        };

        struct LightColorQuantity : Quantity {
            float *hue;

            LightColorQuantity(float *_hue) {
                hue = _hue;
            }

            void setValue(float value) override {
                *hue = clamp(value, getMinValue(), getMaxValue());
            }

            float getValue() override {
                return *hue;
            }

            float getDefaultValue() override { return 0.5; }
            float getMinValue() override { return 0.0; }
            float getMaxValue() override { return 1.0; }

            std::string getDisplayValueString() override {
                return string::f("%.1f°", rescale(*hue, getMinValue(), getMaxValue(), 0.0, 360.0));
            }

            void setDisplayValue(float value) override {
                setValue(value);
            }

            std::string getLabel() override {
                return "Hue";
            }
        };
        struct LightColorSlider : ui::Slider {
            LightColorSlider(float *hue) {
                quantity = new LightColorQuantity(hue);
            }
            ~LightColorSlider() {
                delete quantity;
            }
        };

        LightPulseSlider *pSlider = new LightPulseSlider(pulseHz);
        pSlider->box.size.x = 200.0;
        menu->addChild(pSlider);

        menu->addChild(new MenuSeparator);

        LightColorSlider *hueSlider = new LightColorSlider(hue);
        hueSlider->box.size.x = 150.0;
        menu->addChild(hueSlider);

        return menu;
    }
};

// code modified from https://github.com/VCVRack/Rack/blob/9ad53329fff74989daf3365600f9fccc0b6f5266/src/core/Blank.cpp#L57
struct PhotronPanelResizeHandle : OpaqueWidget {
	bool right = false;
	Vec dragPos;
	Rect originalBox;
	PhotronPanel *module;

	PhotronPanelResizeHandle() {
		box.size = Vec(RACK_GRID_WIDTH * 1, RACK_GRID_HEIGHT);
	}

	void onDragStart(const DragStartEvent& e) override {
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;

		dragPos = APP->scene->rack->getMousePos();
		ModuleWidget* mw = getAncestorOfType<ModuleWidget>();
		assert(mw);
		originalBox = mw->box;
	}

	void onDragMove(const DragMoveEvent& e) override {
		ModuleWidget* mw = getAncestorOfType<ModuleWidget>();
		assert(mw);

		Vec newDragPos = APP->scene->rack->getMousePos();
		float deltaX = newDragPos.x - dragPos.x;

		Rect newBox = originalBox;
		Rect oldBox = mw->box;
		const float minWidth = 3 * RACK_GRID_WIDTH;
        const float maxWidth = DISPLAY_SIZE_WIDTH;
		if (right) {
			newBox.size.x += deltaX;
			// newBox.size.x = std::fmax(newBox.size.x, minWidth);
            newBox.size.x = clamp(newBox.size.x, minWidth, maxWidth);
			newBox.size.x = std::round(newBox.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
		}
		else {
			newBox.size.x -= deltaX;
			// newBox.size.x = std::fmax(newBox.size.x, minWidth);
            newBox.size.x = clamp(newBox.size.x, minWidth, maxWidth);
            newBox.size.x = std::round(newBox.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH;
			newBox.pos.x = originalBox.pos.x + originalBox.size.x - newBox.size.x;
		}

		// Set box and test whether it's valid
		mw->box = newBox;
		if (!APP->scene->rack->requestModulePos(mw, newBox.pos)) {
			mw->box = oldBox;
		}
		module->width = std::round(mw->box.size.x / RACK_GRID_WIDTH);
	}
};

struct PhotronPanelWidget : ModuleWidget {
    Widget *rightHandle;
    PhotronPanelDisplay *display;

    PhotronPanelWidget(PhotronPanel *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PhotronPanel.svg")));

        display = new PhotronPanelDisplay();
        display->module = module;
        display->box.pos = Vec(0.0, 0.0);
        // display->box.size = Vec(DISPLAY_SIZE_WIDTH, DISPLAY_SIZE_HEIGHT);
        addChild(display);

        PhotronPanelResizeHandle *leftHandle = new PhotronPanelResizeHandle;
        leftHandle->module = module;
        addChild(leftHandle);

        PhotronPanelResizeHandle *rightHandle = new PhotronPanelResizeHandle;
        rightHandle->right = true;
        this->rightHandle = rightHandle;
        rightHandle->module = module;
        addChild(rightHandle);
    }

    void step() override {
        PhotronPanel *module = dynamic_cast<PhotronPanel *>(this->module);
        if (module) {
            box.size.x = module->width * RACK_GRID_WIDTH;
        }

        display->box.size = box.size;
        rightHandle->box.pos.x = box.size.x - rightHandle->box.size.x;
        ModuleWidget::step();
    }

    void appendContextMenu(Menu *menu) override {
        PhotronPanel *module = dynamic_cast<PhotronPanel*>(this->module);
        // menu->addChild(new MenuEntry);
        menu->addChild(new MenuSeparator);

        menu->addChild(createIndexSubmenuItem(
            "Processing rate",
            {"60 Hz", "45 Hz", "30 Hz", "20 Hz", "15 Hz", "12 Hz", "10 Hz"},
            [=]() {
                return module->getHz();
            },
            [=](int hz) {
                module->setHz(hz);
            }));

        menu->addChild(createIndexPtrSubmenuItem("Mode", {"color", "black & white", "solid color", "strip"}, &module->colorMode));

        // menu->addChild(createIndexPtrSubmenuItem("Light pulse", {"Off", "0.1 Hz", "0.2 Hz", "0.25 Hz", "0.33 Hz", "0.4 Hz", "0.5 Hz", "1 Hz"}, &module->pulseHzIndex));

        LightMenuItem *lightPulse = createMenuItem<LightMenuItem>("Light", RIGHT_ARROW);
        lightPulse->module = module;
        lightPulse->pulseHz = &module->pulseHz;
        lightPulse->hue = &module->hue;
        menu->addChild(lightPulse);

        menu->addChild(createBoolPtrMenuItem("Dark Room Blobs", "", &module->darkRoomBlobs));
    }
};

Model *modelPhotronPanel = createModel<PhotronPanel, PhotronPanelWidget>("PhotronPanel");
