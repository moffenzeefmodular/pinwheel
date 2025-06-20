#include "plugin.hpp"

struct Pinwheel : Module {
    enum ParamId {
        SPEED_PARAM,
        MASS_PARAM,
        NUM_BLADES_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        SPEEDCVIN_INPUT,
        MASSCVIN_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        GATEOUT_OUTPUT,
        CVOUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    float angle = 0.f;

    Pinwheel() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(SPEED_PARAM, 0.f, 1.f, 0.f, "Speed");
        configParam(MASS_PARAM, 0.f, 1.f, 0.f, "Mass");
        configParam(NUM_BLADES_PARAM, 1.f, 8.f, 4.f, "Number of Blades");
        configInput(SPEEDCVIN_INPUT, "Speed CV In");
        configInput(MASSCVIN_INPUT, "Mass CV In");
        configOutput(GATEOUT_OUTPUT, "Gate Out");
        configOutput(CVOUT_OUTPUT, "CV Out");
    }

    void process(const ProcessArgs& args) override {
        float speed = params[SPEED_PARAM].getValue();
        float rotationRate = speed * 2.f * M_PI;
        angle += rotationRate * args.sampleTime;
        if (angle >= 2.f * M_PI)
            angle -= 2.f * M_PI;
    }
};

struct PinwheelDisplay : Widget {
    Pinwheel* module;

    PinwheelDisplay(Pinwheel* module) {
        this->module = module;
    }

    // HSV to RGB conversion helper, returns NVGcolor with full alpha
    NVGcolor hsvToRgb(float h, float s, float v) {
        float r, g, b;

        int i = (int)(h * 6.f);
        float f = h * 6.f - i;
        float p = v * (1.f - s);
        float q = v * (1.f - f * s);
        float t = v * (1.f - (1.f - f) * s);

        switch (i % 6) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }

        return nvgRGBf(r, g, b);
    }

    // Darken a color by factor (0..1)
    NVGcolor darkenColor(const NVGcolor& color, float factor = 0.85f) {
        return nvgRGBA(
            (uint8_t)(color.r * 255 * factor),
            (uint8_t)(color.g * 255 * factor),
            (uint8_t)(color.b * 255 * factor),
            255
        );
    }

    // Updated drawBlade to take a single color directly
    void drawBlade(const DrawArgs& args, NVGcolor bladeColor, float side, float flatHeight) {
        NVGcolor darkColor = darkenColor(bladeColor);

        // Draw square (left of origin) — flipped vertically
        nvgSave(args.vg);
        nvgScale(args.vg, 1.f, -1.f);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, -side, 0.f, side, side);
        nvgFillColor(args.vg, darkColor);
        nvgFill(args.vg);
        nvgRestore(args.vg);

        // Draw large triangle
        nvgSave(args.vg);
        nvgTranslate(args.vg, -side, 0.f);
        nvgRotate(args.vg, M_PI / 2.f);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0.f, 0.f);
        nvgLineTo(args.vg, -side, 0.f);
        nvgLineTo(args.vg, 0.f, flatHeight * 2.f);
        nvgClosePath(args.vg);
        nvgFillColor(args.vg, darkColor);
        nvgFill(args.vg);
        nvgRestore(args.vg);

        // Draw small triangle
        float smallBase = side;
        float smallHeight = side;

        nvgSave(args.vg);
        nvgTranslate(args.vg, -(side / 2), 0.f);
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0.f, 0.f);
        nvgLineTo(args.vg, smallBase / 2.f, -smallHeight);
        nvgLineTo(args.vg, -smallBase / 2.f, -smallHeight);
        nvgClosePath(args.vg);
        nvgFillColor(args.vg, bladeColor);
        nvgFill(args.vg);
        nvgRestore(args.vg);
    }

    void draw(const DrawArgs& args) override {
        if (!module)
            return;

        Vec center = box.size.div(2);
        nvgSave(args.vg);
        nvgTranslate(args.vg, center.x, center.y);

        // Stem (non-rotating)
        nvgBeginPath(args.vg);
        nvgRect(args.vg, -2.5f, 0.f, 5.f, 100.f);
        nvgFillColor(args.vg, nvgRGBA(60, 60, 60, 255));
        nvgFill(args.vg);

        nvgRotate(args.vg, module->angle);

        float side = 25.f * 0.7f;
        float flatHeight = side * 0.866f;

        int numberOfBlades = (int)std::round(module->params[Pinwheel::NUM_BLADES_PARAM].getValue());
        numberOfBlades = std::max(1, std::min(8, numberOfBlades));

        // Draw blades with evenly spaced hues on the full color wheel
        for (int i = 0; i < numberOfBlades; ++i) {
            float hue = (float)i / numberOfBlades; // [0,1]
            NVGcolor bladeColor = hsvToRgb(hue, 1.f, 1.f);

            nvgSave(args.vg);
            nvgRotate(args.vg, (2.f * M_PI / numberOfBlades) * i);
            drawBlade(args, bladeColor, side, flatHeight);
            nvgRestore(args.vg);
        }

        // Draw white center circle on top
        nvgBeginPath(args.vg);
        nvgCircle(args.vg, 0.f, 0.f, 4.f);
        nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 255));
        nvgFill(args.vg);

        nvgRestore(args.vg);
    }
};

struct PinwheelWidget : ModuleWidget {
    PinwheelWidget(Pinwheel* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Pinwheel.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        auto* display = new PinwheelDisplay(module);
        display->box.size = Vec(120, 120);
        display->box.pos = Vec(
            (box.size.x - 120) / 2.f,
            (box.size.y - 120) / 2.f - 50.f
        );
        addChild(display);

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.585, 112.669)), module, Pinwheel::SPEED_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(39.203, 112.669)), module, Pinwheel::MASS_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(67.5, 75)), module, Pinwheel::NUM_BLADES_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.55, 112.669)), module, Pinwheel::SPEEDCVIN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(49.168, 112.669)), module, Pinwheel::MASSCVIN_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(69.363, 112.669)), module, Pinwheel::GATEOUT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(86.956, 112.669)), module, Pinwheel::CVOUT_OUTPUT));
    }
};

Model* modelPinwheel = createModel<Pinwheel, PinwheelWidget>("Pinwheel");
