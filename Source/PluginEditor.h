#pragma once
#include "PluginProcessor.h"
#include "GrungeLookAndFeel.h"

class BocchiEditor : public juce::AudioProcessorEditor
{
public:
    explicit BocchiEditor (BocchiProcessor&);
    ~BocchiEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        GrungeSlider* slider;
        juce::Point<float> centre;   // в "логических" координатах 640x480
        float body;                  // диаметр корпуса
        float margin;                // запас под дугу и деления
        juce::String name;
    };

    void setupKnob (Knob& k, int ticks, bool reactive, int seed, double defaultValue);
    void drawOutlinedText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour fill, float outline);

    BocchiProcessor& proc;
    GrungeLookAndFeel lnf;

    GrungeSlider drive, tone, wide, out;
    std::vector<Knob> knobs;

    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attach> aDrive, aTone, aWide, aOut;

    juce::Image bg, logo, calm, fuzz;
    float scale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BocchiEditor)
};
