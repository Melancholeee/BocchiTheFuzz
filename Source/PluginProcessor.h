#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"

class BocchiProcessor : public juce::AudioProcessor
{
public:
    BocchiProcessor();
    ~BocchiProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Bocchi the Fuzz"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    void processChunk (juce::dsp::AudioBlock<float> chunk);
    float readChorus (int ch, float delaySamples) const;

    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pTone  = nullptr;
    std::atomic<float>* pWide  = nullptr;
    std::atomic<float>* pOut   = nullptr;

    double sr = 44100.0;
    int maxBlock = 512;

    juce::dsp::Oversampling<float> oversampler;

    // предфильтр (работает на оверсэмплированной частоте, только на "грязной" ветке)
    std::array<juce::dsp::IIR::Filter<float>, 2> preHp, preMid;

    juce::SmoothedValue<float> driveSm, toneSm, wideSm, outSm;

    // DC-блокер
    float dcR = 0.995f;
    std::array<float, 2> dcX { 0.f, 0.f }, dcY { 0.f, 0.f };

    // Tone (однополюсник для наклона)
    float toneA1 = 0.1f;
    std::array<float, 2> toneLow { 0.f, 0.f };

    // Chorus
    std::array<std::vector<float>, 2> chorusBuf;
    int chorusMask = 0;
    int chorusWrite = 0;
    float phaseA = 0.f, phaseB = 0.f;

    float biasTanh = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BocchiProcessor)
};
