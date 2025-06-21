#include "plugin.hpp"

struct Pinwheel : Module {
	enum ParamId {
		NUMBLADES_PARAM,
		SPEED_PARAM,
		MASS_PARAM,
		BLADEANGLEMOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SPEEDCVIN_INPUT,
		NUMBLADESCVIN_INPUT,
		MASSCVIN_INPUT,
		BLADEANGLEMODCVIN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		GATE1OUT_OUTPUT,
		GATE2OUT_OUTPUT,
		GATE3OUT_OUTPUT,
		GATE4OUT_OUTPUT,
		GATE5OUT_OUTPUT,
		GATE6OUT_OUTPUT,
		GATE7OUT_OUTPUT,
		GATE8OUT_OUTPUT,
		CV1OUT_OUTPUT,
		CV2OUT_OUTPUT,
		CV3OUT_OUTPUT,
		CV4OUT_OUTPUT,
		CV5OUT_OUTPUT,
		CV6OUT_OUTPUT,
		CV7OUT_OUTPUT,
		CV8OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		GATE1LED_LIGHT,
		GATE2LED_LIGHT,
		GATE3LED_LIGHT,
		GATE4LED_LIGHT,
		GATE6LED_LIGHT,
		GATE5LED_LIGHT,
		GATE7LED_LIGHT,
		GATE8LED_LIGHT,
		CV1GREENLED_LIGHT,
		CV1REDLED_LIGHT,
		CV2GREENLED_LIGHT,
		CV2REDLED_LIGHT,
		CV3GREENLED_LIGHT,
		CV3REDLED_LIGHT,
		CV4GREENLED_LIGHT,
		CV4REDLED_LIGHT,
		CV5GREENLED_LIGHT,
		CV5REDLED_LIGHT,
		CV6GREENLED_LIGHT,
		CV6REDLED_LIGHT,
		CV7GREENLED_LIGHT,
		CV7REDLED_LIGHT,
		CV8GREENLED_LIGHT,
		CV8REDLED_LIGHT,
		LIGHTS_LEN
	};

    float angle = 0.f;
    float slewedSpeed = 0.f;

    Pinwheel() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(SPEED_PARAM, 0.f, 1.f, 0.5f, "Speed");
        configParam(MASS_PARAM, 0.f, 1.f, 0.f, "Mass");
        configSwitch(NUMBLADES_PARAM, 1.f, 8.f, 4.f, "Number of Blades", {"1", "2", "3", "4", "5", "6", "7", "8"});
        configParam(BLADEANGLEMOD_PARAM, -1.f, 1.f, 0.f, "Blade Angle Mod");
        configInput(SPEEDCVIN_INPUT, "Speed CV In");
        configInput(MASSCVIN_INPUT, "Mass CV In");
        configInput(NUMBLADESCVIN_INPUT, "Number of Blades CV In");
        configInput(BLADEANGLEMODCVIN_INPUT, "Blade Angle Mod CV In");

        for (int i = 0; i < 8; i++) {
            configOutput(GATE1OUT_OUTPUT + i, "Gate Out");
            configOutput(CV1OUT_OUTPUT + i, "CV Out");
        }
    }

    void process(const ProcessArgs& args) override {
        float speedKnobVoltage = rescale(params[SPEED_PARAM].getValue(), 0.f, 1.f, -5.f, 5.f);
        float speedCVVoltage = inputs[SPEEDCVIN_INPUT].isConnected() ? clamp(inputs[SPEEDCVIN_INPUT].getVoltage(), -5.f, 5.f) : 0.f;
        float combinedSpeedVoltage = clamp(speedKnobVoltage + speedCVVoltage, -5.f, 5.f);
        float speedParam = rescale(combinedSpeedVoltage, -5.f, 5.f, 0.f, 1.f);
        float targetSpeed = (speedParam - 0.5f) * 2.f;

        float massKnobVoltage = rescale(params[MASS_PARAM].getValue(), 0.f, 1.f, -5.f, 5.f);
        float massCVVoltage = inputs[MASSCVIN_INPUT].isConnected() ? clamp(inputs[MASSCVIN_INPUT].getVoltage(), -5.f, 5.f) : 0.f;
        float combinedMassVoltage = clamp(massKnobVoltage + massCVVoltage, -5.f, 5.f);
        float combinedMass = rescale(combinedMassVoltage, -5.f, 5.f, 0.f, 1.f);

        float maxSlewTime = 1.f;
        float minSlewTime = 0.001f;
        float slewTime = rescale(combinedMass, 0.f, 1.f, minSlewTime, maxSlewTime);
        float slewAmount = clamp(args.sampleTime / slewTime, 0.f, 1.f);
        slewedSpeed += (targetSpeed - slewedSpeed) * slewAmount;

        float rotationRate = slewedSpeed * 8.f * M_PI;
        angle += rotationRate * args.sampleTime;
        if (angle >= 2.f * M_PI) angle -= 2.f * M_PI;
        else if (angle < 0.f) angle += 2.f * M_PI;

        float numBladesKnob = rescale(params[NUMBLADES_PARAM].getValue(), 1.f, 8.f, -5.f, 5.f);
        float numBladesCV = inputs[NUMBLADESCVIN_INPUT].isConnected() ? clamp(inputs[NUMBLADESCVIN_INPUT].getVoltage(), -5.f, 5.f) : 0.f;
        float combinedNumBladesVoltage = clamp(numBladesKnob + numBladesCV, -5.f, 5.f);
        float combinedNumBlades = rescale(combinedNumBladesVoltage, -5.f, 5.f, 1.f, 8.f);
        int numberOfBlades = clamp((int)std::round(combinedNumBlades), 1, 8);

        float angleModKnob = params[BLADEANGLEMOD_PARAM].getValue();
        float angleModCV = inputs[BLADEANGLEMODCVIN_INPUT].isConnected() ? clamp(inputs[BLADEANGLEMODCVIN_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
        float totalAngleMod = clamp(angleModKnob + angleModCV, -1.f, 1.f);

        for (int i = 0; i < 8; ++i) {
            if (i < numberOfBlades) {
                float baseSpacing = (2.f * M_PI / numberOfBlades);
                float modulatedOffset = baseSpacing * i * (1.f + totalAngleMod);
                float bladeAngle = angle + modulatedOffset;
                if (bladeAngle >= 2.f * M_PI) bladeAngle -= 2.f * M_PI;

                float shiftedAngle = bladeAngle - (M_PI / 2.f);
                if (shiftedAngle < 0.f) shiftedAngle += 2.f * M_PI;

                float CVout = 0.f;
                if (shiftedAngle <= M_PI)
                    CVout = rescale(shiftedAngle, 0.f, M_PI, 5.f, -5.f);
                else
                    CVout = rescale(shiftedAngle, M_PI, 2.f * M_PI, -5.f, 5.f);

                outputs[CV1OUT_OUTPUT + i].setVoltage(CVout);

                const float side = 25.f * 0.7f;
                const float flatHeight = side * 0.866f;
                float tipRadius = side + flatHeight;

                float tipX = tipRadius * cos(bladeAngle);
                float tipY = -tipRadius * sin(bladeAngle);

                const float stemWidth = 5.f;
                bool gateActive = (fabs(tipX) <= (stemWidth / 2.f)) && (tipY >= 0.f);

                outputs[GATE1OUT_OUTPUT + i].setVoltage(gateActive ? 5.f : 0.f);
                lights[GATE1LED_LIGHT + i].setBrightnessSmooth(gateActive ? 1.f : 0.f, args.sampleTime);

                if (CVout >= 0.f) {
                    lights[CV1GREENLED_LIGHT + i * 2].setBrightnessSmooth(clamp(CVout / 10.f, 0.f, 1.f), args.sampleTime);
                    lights[CV1REDLED_LIGHT + i * 2].setBrightnessSmooth(0.f, args.sampleTime);
                } else {
                    lights[CV1GREENLED_LIGHT + i * 2].setBrightnessSmooth(0.f, args.sampleTime);
                    lights[CV1REDLED_LIGHT + i * 2].setBrightnessSmooth(clamp(-CVout / 10.f, 0.f, 1.f), args.sampleTime);
                }
            } else {
                outputs[GATE1OUT_OUTPUT + i].setVoltage(0.f);
                outputs[CV1OUT_OUTPUT + i].setVoltage(0.f);
                lights[GATE1LED_LIGHT + i].setBrightnessSmooth(0.f, args.sampleTime);
                lights[CV1GREENLED_LIGHT + i * 2].setBrightnessSmooth(0.f, args.sampleTime);
                lights[CV1REDLED_LIGHT + i * 2].setBrightnessSmooth(0.f, args.sampleTime);
            }
        }
    }
};

struct PinwheelDisplay : Widget {
    Pinwheel* module;

    PinwheelDisplay(Pinwheel* module) {
        this->module = module;
    }

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

    NVGcolor darkenColor(const NVGcolor& color, float factor = 0.85f) {
        return nvgRGBA(
            (uint8_t)(color.r * 255 * factor),
            (uint8_t)(color.g * 255 * factor),
            (uint8_t)(color.b * 255 * factor),
            255
        );
    }

    void drawBlade(const DrawArgs& args, NVGcolor bladeColor, float side, float flatHeight) {
        NVGcolor darkColor = darkenColor(bladeColor);

        nvgSave(args.vg);
        nvgScale(args.vg, 1.f, -1.f);
        nvgBeginPath(args.vg);
        nvgRect(args.vg, -side, 0.f, side, side);
        nvgFillColor(args.vg, darkColor);
        nvgFill(args.vg);
        nvgRestore(args.vg);

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
    if (!module) return;

    Vec center = box.size.div(2);
    nvgSave(args.vg);
    nvgTranslate(args.vg, center.x, center.y);

    nvgBeginPath(args.vg);
    nvgRect(args.vg, -2.5f, 0.f, 5.f, 100.f);
    nvgFillColor(args.vg, nvgRGBA(60, 60, 60, 255));
    nvgFill(args.vg);

    nvgRotate(args.vg, module->angle);

    float side = 25.f * 0.7f;
    float flatHeight = side * 0.866f;

    float numBladesKnob = rescale(module->params[Pinwheel::NUMBLADES_PARAM].getValue(), 1.f, 8.f, -5.f, 5.f);
    float numBladesCV = module->inputs[Pinwheel::NUMBLADESCVIN_INPUT].isConnected() ? clamp(module->inputs[Pinwheel::NUMBLADESCVIN_INPUT].getVoltage(), -5.f, 5.f) : 0.f;
    float combinedNumBladesVoltage = clamp(numBladesKnob + numBladesCV, -5.f, 5.f);
    float combinedNumBlades = rescale(combinedNumBladesVoltage, -5.f, 5.f, 1.f, 8.f);
    int numberOfBlades = clamp((int)std::round(combinedNumBlades), 1, 8);

    // Calculate angle mod just like in process()
    float angleModKnob = module->params[Pinwheel::BLADEANGLEMOD_PARAM].getValue();
    float angleModCV = module->inputs[Pinwheel::BLADEANGLEMODCVIN_INPUT].isConnected() ? clamp(module->inputs[Pinwheel::BLADEANGLEMODCVIN_INPUT].getVoltage() / 5.f, -1.f, 1.f) : 0.f;
    float totalAngleMod = clamp(angleModKnob + angleModCV, -1.f, 1.f);

    float baseSpacing = (2.f * M_PI / numberOfBlades);

    for (int i = 0; i < numberOfBlades; ++i) {
        float hue = (float)i / numberOfBlades;
        NVGcolor bladeColor = hsvToRgb(hue, 1.f, 1.f);

        nvgSave(args.vg);
        nvgRotate(args.vg, baseSpacing * i * (1.f + totalAngleMod));  // <--- here
        drawBlade(args, bladeColor, side, flatHeight);
        nvgRestore(args.vg);
    }

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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.242, 94.016)), module, Pinwheel::NUMBLADES_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.15, 106.186)), module, Pinwheel::SPEED_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(27.039, 106.186)), module, Pinwheel::MASS_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.15, 119.858)), module, Pinwheel::SPEEDCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.242, 119.858)), module, Pinwheel::NUMBLADESCVIN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(27.039, 119.858)), module, Pinwheel::MASSCVIN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.372, 101.159)), module, Pinwheel::GATE1OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(46.501, 101.159)), module, Pinwheel::GATE2OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(54.967, 101.159)), module, Pinwheel::GATE3OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(63.434, 101.159)), module, Pinwheel::GATE4OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(71.901, 101.159)), module, Pinwheel::GATE5OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(80.103, 101.159)), module, Pinwheel::GATE6OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.569, 101.159)), module, Pinwheel::GATE7OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(96.771, 101.159)), module, Pinwheel::GATE8OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(37.372, 117.829)), module, Pinwheel::CV1OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(46.501, 117.829)), module, Pinwheel::CV2OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(54.967, 117.829)), module, Pinwheel::CV3OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(63.434, 117.829)), module, Pinwheel::CV4OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(71.901, 117.829)), module, Pinwheel::CV5OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(80.103, 117.829)), module, Pinwheel::CV6OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(88.569, 117.829)), module, Pinwheel::CV7OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(96.771, 117.829)), module, Pinwheel::CV8OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.372, 93.046)), module, Pinwheel::GATE1LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(46.501, 93.046)), module, Pinwheel::GATE2LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(54.967, 93.046)), module, Pinwheel::GATE3LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(63.434, 93.046)), module, Pinwheel::GATE4LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(80.114, 92.947)), module, Pinwheel::GATE5LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(71.901, 93.046)), module, Pinwheel::GATE6LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(88.569, 93.046)), module, Pinwheel::GATE7LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(96.771, 93.046)), module, Pinwheel::GATE8LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(37.372, 109.891)), module, Pinwheel::CV1GREENLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.372, 109.891)), module, Pinwheel::CV1REDLED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(46.501, 109.891)), module, Pinwheel::CV2GREENLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(46.501, 109.891)), module, Pinwheel::CV2REDLED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(54.967, 109.891)), module, Pinwheel::CV3GREENLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(54.967, 109.891)), module, Pinwheel::CV3REDLED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(63.434, 109.891)), module, Pinwheel::CV4GREENLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(63.434, 109.891)), module, Pinwheel::CV4REDLED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(71.901, 109.891)), module, Pinwheel::CV5GREENLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(71.901, 109.891)), module, Pinwheel::CV5REDLED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(80.103, 109.891)), module, Pinwheel::CV6GREENLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(80.103, 109.891)), module, Pinwheel::CV6REDLED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(88.569, 109.891)), module, Pinwheel::CV7GREENLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(88.569, 109.891)), module, Pinwheel::CV7REDLED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(96.771, 109.891)), module, Pinwheel::CV8GREENLED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(96.771, 109.891)), module, Pinwheel::CV8REDLED_LIGHT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(17.242, 80.0)), module, Pinwheel::BLADEANGLEMOD_PARAM));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(27.039, 80.0)), module, Pinwheel::BLADEANGLEMODCVIN_INPUT));

	}
};

Model* modelPinwheel = createModel<Pinwheel, PinwheelWidget>("Pinwheel");
