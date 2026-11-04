#include "PluginEditor.h"
#include <BinaryData.h>

// ===== Раскладка окна 640 x 480 (логические пиксели) =====
namespace Layout
{
    constexpr float W = 640.0f, H = 480.0f;

    const juce::Rectangle<float> logoBox   { 20.0f,  12.0f, 250.0f, 148.0f };
    const juce::Rectangle<float> girlBox   { 280.0f,  0.0f, 360.0f, 480.0f };   // ширина персонажа = ширина бокса (больше число = крупнее)

    // центр (x, y), диаметр корпуса, запас вокруг
    constexpr float driveX = 100.0f, driveY = 240.0f, driveD = 120.0f, driveM = 26.0f;
    constexpr float toneX  = 224.0f, toneY  = 268.0f, toneD  =  64.0f, toneM  = 12.0f;
    constexpr float wideX  = 100.0f, wideY  = 394.0f, wideD  =  84.0f, wideM  = 12.0f;
    constexpr float outX   = 224.0f, outY   = 394.0f, outD   =  84.0f, outM   = 12.0f;

    // Смена персонажа по Drive: начинает меняться после fadeStart, полностью "закручена" на fadeEnd
    constexpr float fadeStart = 0.35f, fadeEnd = 0.85f;
}

BocchiEditor::BocchiEditor (BocchiProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    bg   = juce::ImageCache::getFromMemory (BinaryData::background_jpg, BinaryData::background_jpgSize);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png,       BinaryData::logo_pngSize);
    calm = juce::ImageCache::getFromMemory (BinaryData::calm_girl_png,  BinaryData::calm_girl_pngSize);
    fuzz = juce::ImageCache::getFromMemory (BinaryData::fuzz_girl_png,  BinaryData::fuzz_girl_pngSize);

    knobs = {
        { &drive, { Layout::driveX, Layout::driveY }, Layout::driveD, Layout::driveM, "Drive" },
        { &tone,  { Layout::toneX,  Layout::toneY  }, Layout::toneD,  Layout::toneM,  "Tone"  },
        { &wide,  { Layout::wideX,  Layout::wideY  }, Layout::wideD,  Layout::wideM,  "Wide"  },
        { &out,   { Layout::outX,   Layout::outY   }, Layout::outD,   Layout::outM,   "Out"   },
    };

    setupKnob (knobs[0], 11, true,  11, Cfg::kDefDrive);
    setupKnob (knobs[1],  0, false, 23, Cfg::kDefTone);
    setupKnob (knobs[2],  0, false, 37, Cfg::kDefWide);
    setupKnob (knobs[3],  0, false, 41, Cfg::kDefOutDb);

    aDrive = std::make_unique<Attach> (proc.apvts, Cfg::kIdDrive, drive);
    aTone  = std::make_unique<Attach> (proc.apvts, Cfg::kIdTone,  tone);
    aWide  = std::make_unique<Attach> (proc.apvts, Cfg::kIdWide,  wide);
    aOut   = std::make_unique<Attach> (proc.apvts, Cfg::kIdOut,   out);

    // двойной клик = сброс (после attach, чтобы точно сработало)
    drive.setDoubleClickReturnValue (true, Cfg::kDefDrive);
    tone .setDoubleClickReturnValue (true, Cfg::kDefTone);
    wide .setDoubleClickReturnValue (true, Cfg::kDefWide);
    out  .setDoubleClickReturnValue (true, Cfg::kDefOutDb);

    setResizable (true, true);
    setResizeLimits (480, 360, 1280, 960);
    getConstrainer()->setFixedAspectRatio ((double) Layout::W / (double) Layout::H);
    setSize ((int) Layout::W, (int) Layout::H);
}

BocchiEditor::~BocchiEditor()
{
    for (auto& k : knobs)
        k.slider->setLookAndFeel (nullptr);
}

void BocchiEditor::setupKnob (Knob& k, int ticks, bool reactive, int seed, double)
{
    auto& s = *k.slider;
    addAndMakeVisible (s);
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setMouseDragSensitivity (220);
    s.setLookAndFeel (&lnf);

    s.getProperties().set ("margin",   k.margin);
    s.getProperties().set ("ticks",    ticks);
    s.getProperties().set ("reactive", reactive);
    s.getProperties().set ("seed",     seed);

    s.onValueChange = [this] { repaint(); };
    s.onHover       = [this] { repaint(); };
}

void BocchiEditor::resized()
{
    scale = (float) getWidth() / Layout::W;
    lnf.scale = scale;

    for (auto& k : knobs)
    {
        const float half = (k.body * 0.5f + k.margin) * scale;
        k.slider->setBounds (juce::Rectangle<float> (0, 0, half * 2.0f, half * 2.0f)
                                 .withCentre (k.centre * scale)
                                 .toNearestInt());
    }
}

void BocchiEditor::drawOutlinedText (juce::Graphics& g, const juce::String& text,
                                     juce::Rectangle<float> r, juce::Colour fill, float outline)
{
    g.setColour (Pal::ink);
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
            if (dx != 0 || dy != 0)
                g.drawText (text, r.translated ((float) dx * outline, (float) dy * outline),
                            juce::Justification::centred, false);
    g.setColour (fill);
    g.drawText (text, r, juce::Justification::centred, false);
}

void BocchiEditor::paint (juce::Graphics& g)
{
    const float sc = scale;
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);

    // фон-текстура на всё окно (с обрезкой, пропорции сохраняются)
    g.setOpacity (1.0f);
    g.drawImage (bg, getLocalBounds().toFloat(),
                 juce::RectanglePlacement (juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination));

    // логотип
    g.drawImage (logo, Layout::logoBox * sc, juce::RectanglePlacement::centred);

    // персонаж: плавный кроссфейд Calm -> Fuzz по Drive
    const float d = (float) drive.getValue();
    float f = juce::jlimit (0.0f, 1.0f, (d - Layout::fadeStart) / (Layout::fadeEnd - Layout::fadeStart));
    f = f * f * (3.0f - 2.0f * f);                                       // smoothstep
    const float calmA = juce::jlimit (0.0f, 1.0f, 2.0f * (1.0f - f));    // на краях без "призраков"
    const float fuzzA = juce::jlimit (0.0f, 1.0f, 2.0f * f);

    const auto girlRect = Layout::girlBox * sc;
    if (calmA > 0.0f) { g.setOpacity (calmA); g.drawImage (calm, girlRect, juce::RectanglePlacement::centred); }
    if (fuzzA > 0.0f) { g.setOpacity (fuzzA); g.drawImage (fuzz, girlRect, juce::RectanglePlacement::centred); }
    g.setOpacity (1.0f);

    // подписи под ручками; при наведении/вращении показывают значение
    g.setFont (juce::Font (juce::FontOptions (17.0f * sc, juce::Font::bold)));
    for (auto& k : knobs)
    {
        const bool active = k.slider->isMouseOverOrDragging();
        const auto text   = active ? k.slider->getTextFromValue (k.slider->getValue()) : k.name;
        const juce::Rectangle<float> r (0, 0, 120.0f * sc, 20.0f * sc);
        const auto pos = juce::Point<float> (k.centre.x, k.centre.y + k.body * 0.5f + 16.0f) * sc;
        drawOutlinedText (g, text, r.withCentre (pos), active ? Pal::pink : Pal::cream, 1.2f * sc);
    }
}
