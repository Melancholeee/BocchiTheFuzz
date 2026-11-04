#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

BocchiProcessor::BocchiProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout()),
      oversampler (2, (size_t) Cfg::kOversampleLog2,
                   juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true)
{
    pDrive = apvts.getRawParameterValue (Cfg::kIdDrive);
    pTone  = apvts.getRawParameterValue (Cfg::kIdTone);
    pWide  = apvts.getRawParameterValue (Cfg::kIdWide);
    pOut   = apvts.getRawParameterValue (Cfg::kIdOut);
}

APVTS::ParameterLayout BocchiProcessor::createLayout()
{
    using P    = juce::AudioParameterFloat;
    using Attr = juce::AudioParameterFloatAttributes;

    APVTS::ParameterLayout layout;

    layout.add (std::make_unique<P> (
        juce::ParameterID { Cfg::kIdDrive, 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), Cfg::kDefDrive,
        Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v * 10.0f, 1); })
              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue() / 10.0f; })));

    layout.add (std::make_unique<P> (
        juce::ParameterID { Cfg::kIdTone, 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), Cfg::kDefTone,
        Attr().withStringFromValueFunction ([] (float v, int)
              {
                  int t = (int) std::round ((v - 0.5f) * 200.0f);
                  return (t > 0 ? "+" : "") + juce::String (t);
              })
              .withValueFromStringFunction ([] (const juce::String& s) { return 0.5f + s.getFloatValue() / 200.0f; })));

    layout.add (std::make_unique<P> (
        juce::ParameterID { Cfg::kIdWide, 1 }, "Wide",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), Cfg::kDefWide,
        Attr().withStringFromValueFunction ([] (float v, int) { return juce::String ((int) std::round (v * 100.0f)) + " %"; })
              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue() / 100.0f; })));

    layout.add (std::make_unique<P> (
        juce::ParameterID { Cfg::kIdOut, 1 }, "Out",
        juce::NormalisableRange<float> (Cfg::kOutMinDb, Cfg::kOutMaxDb, 0.1f), Cfg::kDefOutDb,
        Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })
              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));

    return layout;
}

bool BocchiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // выход всегда стерео (хорусу нужно стерео), вход моно или стерео
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono();
}

void BocchiProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (1, samplesPerBlock);
    const double osSr = sr * (double) (1 << Cfg::kOversampleLog2);

    oversampler.initProcessing ((size_t) maxBlock);
    oversampler.reset();
    setLatencySamples ((int) std::ceil (oversampler.getLatencyInSamples()));

    for (int ch = 0; ch < 2; ++ch)
    {
        preHp[(size_t) ch].coefficients  = juce::dsp::IIR::Coefficients<float>::makeHighPass (osSr, Cfg::kPreHpHz, 0.707f);
        preMid[(size_t) ch].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (osSr, Cfg::kPreMidHz, Cfg::kPreMidQ, Cfg::kPreMidGain);
        preHp[(size_t) ch].reset();
        preMid[(size_t) ch].reset();
    }

    driveSm.reset (osSr, Cfg::kSmoothSec);
    toneSm.reset  (sr,   Cfg::kSmoothSec);
    wideSm.reset  (sr,   Cfg::kSmoothSec);
    outSm.reset   (sr,   Cfg::kSmoothSec);

    driveSm.setCurrentAndTargetValue (pDrive->load());
    toneSm.setCurrentAndTargetValue  (pTone->load());
    wideSm.setCurrentAndTargetValue  (pWide->load());
    outSm.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain (pOut->load()));

    dcR    = (float) std::exp (-juce::MathConstants<double>::twoPi * 20.0 / sr);
    toneA1 = 1.0f - (float) std::exp (-juce::MathConstants<double>::twoPi * (double) Cfg::kTonePivotHz / sr);
    biasTanh = std::tanh (Cfg::kBias);

    dcX.fill (0.f); dcY.fill (0.f); toneLow.fill (0.f);

    const int size = juce::nextPowerOfTwo ((int) (sr * 0.06) + 8);
    for (auto& b : chorusBuf) b.assign ((size_t) size, 0.0f);
    chorusMask  = size - 1;
    chorusWrite = 0;
    phaseA = 0.0f;
    phaseB = 1.3f;
}

float BocchiProcessor::readChorus (int ch, float delaySamples) const
{
    const auto& buf = chorusBuf[(size_t) ch];
    float rp = (float) chorusWrite - delaySamples;
    while (rp < 0.0f) rp += (float) (chorusMask + 1);
    const int   i0   = (int) rp;
    const float frac = rp - (float) i0;
    const float a = buf[(size_t) (i0 & chorusMask)];
    const float b = buf[(size_t) ((i0 + 1) & chorusMask)];
    return a + frac * (b - a);
}

void BocchiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0)
        return;

    // моно-вход -> копируем во второй канал
    if (getTotalNumInputChannels() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    driveSm.setTargetValue (pDrive->load());
    toneSm.setTargetValue  (pTone->load());
    wideSm.setTargetValue  (pWide->load());
    outSm.setTargetValue   (juce::Decibels::decibelsToGain (pOut->load()));

    juce::dsp::AudioBlock<float> full (buffer);
    for (int start = 0; start < numSamples; start += maxBlock)
    {
        const int len = juce::jmin (maxBlock, numSamples - start);
        processChunk (full.getSubBlock ((size_t) start, (size_t) len));
    }
}

void BocchiProcessor::processChunk (juce::dsp::AudioBlock<float> chunk)
{
    // ---------- 1. DRIVE на 4x оверсэмплинге ----------
    auto osBlock = oversampler.processSamplesUp (chunk);
    const size_t osN = osBlock.getNumSamples();
    float* p0 = osBlock.getChannelPointer (0);
    float* p1 = osBlock.getChannelPointer (1);
    float* ptr[2] = { p0, p1 };

    for (size_t i = 0; i < osN; ++i)
    {
        const float d    = driveSm.getNextValue();
        const float gain = juce::Decibels::decibelsToGain (d * Cfg::kMaxDriveDb);
        const float mix  = juce::jmin (1.0f, d * Cfg::kMixSpeed);
        const float comp = 1.0f / std::pow (gain, Cfg::kCompExp);

        for (size_t ch = 0; ch < 2; ++ch)
        {
            const float dry = ptr[ch][i];
            float x = preHp[ch].processSample (dry);
            x = preMid[ch].processSample (x);

            float wet = std::tanh (x * gain + Cfg::kBias) - biasTanh;   // сама сатурация
            wet *= comp;                                                 // компенсация громкости

            ptr[ch][i] = dry * (1.0f - mix) + wet * mix;
        }
    }

    oversampler.processSamplesDown (chunk);

    // ---------- 2. TONE -> 3. WIDE (хорус) -> 4. OUT, на обычной частоте ----------
    float* data[2] = { chunk.getChannelPointer (0), chunk.getChannelPointer (1) };
    const size_t n = chunk.getNumSamples();
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float msToSamples = (float) sr / 1000.0f;

    for (size_t i = 0; i < n; ++i)
    {
        const float t  = toneSm.getNextValue();
        const float w  = wideSm.getNextValue() * Cfg::kChorusMaxWet;
        const float og = outSm.getNextValue();

        const float tilt  = (t - 0.5f) * 2.0f;                                   // -1..+1
        const float gLow  = juce::Decibels::decibelsToGain (-tilt * Cfg::kToneRangeDb * 0.5f);
        const float gHigh = juce::Decibels::decibelsToGain ( tilt * Cfg::kToneRangeDb * 0.5f);

        const float lfoA = std::sin (phaseA);
        const float lfoB = std::sin (phaseB);

        for (int ch = 0; ch < 2; ++ch)
        {
            float x = data[ch][i];

            // DC-блокер (после асимметричного клиппера)
            const float y = x - dcX[(size_t) ch] + dcR * dcY[(size_t) ch];
            dcX[(size_t) ch] = x;
            dcY[(size_t) ch] = y;
            x = y;

            // Tone: наклон спектра вокруг kTonePivotHz
            toneLow[(size_t) ch] += toneA1 * (x - toneLow[(size_t) ch]);
            const float low  = toneLow[(size_t) ch];
            const float high = x - low;
            x = low * gLow + high * gHigh;

            // Chorus: два голоса, левый и правый в противофазе
            chorusBuf[(size_t) ch][(size_t) chorusWrite] = x;
            const float sgn = (ch == 0) ? 1.0f : -1.0f;
            const float dA = (Cfg::kChorusBaseAms + sgn * Cfg::kChorusDepthMs * lfoA) * msToSamples;
            const float dB = (Cfg::kChorusBaseBms + sgn * Cfg::kChorusDepthMs * lfoB) * msToSamples;
            const float chorus = 0.5f * (readChorus (ch, dA) + readChorus (ch, dB));
            x = x * (1.0f - w) + chorus * w;

            data[ch][i] = x * og;
        }

        chorusWrite = (chorusWrite + 1) & chorusMask;
        phaseA += twoPi * Cfg::kChorusRateA / (float) sr;
        phaseB += twoPi * Cfg::kChorusRateB / (float) sr;
        if (phaseA > twoPi) phaseA -= twoPi;
        if (phaseB > twoPi) phaseB -= twoPi;
    }
}

juce::AudioProcessorEditor* BocchiProcessor::createEditor()
{
    return new BocchiEditor (*this);
}

void BocchiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BocchiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BocchiProcessor();
}
