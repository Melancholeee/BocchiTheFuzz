#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Палитра взята из твоих картинок
namespace Pal
{
    inline const juce::Colour pink      { 0xfff2649a };   // волосы / лого
    inline const juce::Colour pinkLight { 0xfff98fb4 };
    inline const juce::Colour cream     { 0xfff6dcd8 };   // кроссовки
    inline const juce::Colour wine      { 0xff7a1530 };   // корпус ручек
    inline const juce::Colour ink       { 0xff2e0714 };   // контур
    inline const juce::Colour orangeDark{ 0xffb87518 };   // фон потемнее
}

// Слайдер, который сообщает о наведении (чтобы показать значение вместо подписи)
class GrungeSlider : public juce::Slider
{
public:
    std::function<void()> onHover;

    void mouseEnter (const juce::MouseEvent& e) override { Slider::mouseEnter (e); if (onHover) onHover(); }
    void mouseExit  (const juce::MouseEvent& e) override { Slider::mouseExit  (e); if (onHover) onHover(); }
};

class GrungeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    float scale = 1.0f;   // масштаб окна относительно 640x480

    // Неровный "нарисованный от руки" круг
    static juce::Path makeBlob (juce::Point<float> c, float R, float amt, int seed)
    {
        juce::Random rng (seed);
        const float p1 = rng.nextFloat() * 6.28f, p2 = rng.nextFloat() * 6.28f, p3 = rng.nextFloat() * 6.28f;
        juce::Path p;
        const int n = 72;
        for (int k = 0; k < n; ++k)
        {
            const float th = juce::MathConstants<float>::twoPi * (float) k / (float) n;
            const float rr = R * (1.0f + amt * (0.5f * std::sin (3.0f * th + p1)
                                              + 0.3f * std::sin (5.0f * th + p2)
                                              + 0.2f * std::sin (9.0f * th + p3)));
            const juce::Point<float> pt (c.x + rr * std::sin (th), c.y - rr * std::cos (th));
            if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        return p;
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float a0, float a1, juce::Slider& s) override
    {
        const float sc       = scale;
        const float margin   = (float) s.getProperties().getWithDefault ("margin", 12.0f) * sc;
        const int   ticks    = (int)   s.getProperties().getWithDefault ("ticks", 0);
        const bool  reactive = (bool)  s.getProperties().getWithDefault ("reactive", false);
        const int   seed     = (int)   s.getProperties().getWithDefault ("seed", 1);

        const juce::Rectangle<float> bounds ((float) x, (float) y, (float) w, (float) h);
        const auto  c   = bounds.getCentre();
        const float R   = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - margin;
        const float ang = a0 + pos * (a1 - a0);
        const auto  strokeRound = [] (float width) { return juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded); };

        // ---------- дуга-индикатор (маркером) ----------
        const float rArc = R + 7.0f * sc;
        juce::Path track, fill;
        track.addCentredArc (c.x, c.y, rArc, rArc, 0.0f, a0, a1, true);
        fill .addCentredArc (c.x, c.y, rArc, rArc, 0.0f, a0, juce::jmax (ang, a0 + 0.001f), true);

        g.setColour (Pal::ink);        g.strokePath (track, strokeRound (8.0f * sc));
        g.setColour (Pal::orangeDark); g.strokePath (track, strokeRound (4.0f * sc));
        if (pos > 0.002f)
        {
            g.setColour (Pal::ink);  g.strokePath (fill, strokeRound (8.0f * sc));
            g.setColour (Pal::pink); g.strokePath (fill, strokeRound (4.0f * sc));
        }

        // ---------- деления 0..10 (для большой ручки) ----------
        for (int i = 0; i < ticks; ++i)
        {
            const float frac = (float) i / (float) juce::jmax (1, ticks - 1);
            const float a    = a0 + frac * (a1 - a0);
            const float rin  = R + 14.0f * sc, rout = R + 20.0f * sc;
            juce::Path t;
            t.startNewSubPath (c.x + rin  * std::sin (a), c.y - rin  * std::cos (a));
            t.lineTo          (c.x + rout * std::sin (a), c.y - rout * std::cos (a));
            const bool lit = pos >= frac - 0.001f;
            g.setColour (Pal::ink);                                  g.strokePath (t, strokeRound (5.0f * sc));
            g.setColour (lit ? Pal::pink : Pal::orangeDark);         g.strokePath (t, strokeRound (2.4f * sc));
        }

        // ---------- корпус ----------
        const float wobble = 0.012f + (reactive ? 0.03f * pos : 0.0f);
        const auto  blob   = makeBlob (c, R, wobble, seed);

        // жёсткая смещённая тень (как у лого)
        g.setColour (Pal::ink.withAlpha (0.85f));
        g.fillPath (blob, juce::AffineTransform::translation (3.0f * sc, 4.0f * sc));

        g.setGradientFill (juce::ColourGradient (Pal::wine.brighter (0.3f), c.x - R * 0.4f, c.y - R * 0.5f,
                                                 Pal::wine.darker (0.5f),   c.x + R * 0.5f, c.y + R * 0.6f, true));
        g.fillPath (blob);

        g.saveState();
        g.reduceClipRegion (blob);

        // растр (полутон), крупнее в тени
        const float step = 5.5f * sc;
        int row = 0;
        for (float yy = c.y - R; yy < c.y + R; yy += step, ++row)
            for (float xx = c.x - R + (row % 2 ? step * 0.5f : 0.0f); xx < c.x + R; xx += step)
            {
                const float dx = xx - c.x, dy = yy - c.y;
                if (dx * dx + dy * dy > R * R) continue;
                const float d  = (dx * 0.6f + dy * 0.8f) / R;                // -1 (свет) .. +1 (тень)
                const float rr = step * (0.12f + 0.22f * (d * 0.5f + 0.5f));
                g.setColour (Pal::ink.withAlpha (0.35f));
                g.fillEllipse (xx - rr, yy - rr, rr * 2.0f, rr * 2.0f);
            }

        // светлые полосы, как лампасы на костюме
        juce::Path st1, st2;
        st1.addCentredArc (c.x, c.y, R * 0.78f, R * 0.78f, 0.0f, -1.25f, -0.35f, true);
        st2.addCentredArc (c.x, c.y, R * 0.66f, R * 0.66f, 0.0f, -1.10f, -0.55f, true);
        g.setColour (Pal::cream.withAlpha (0.55f)); g.strokePath (st1, strokeRound (2.4f * sc));
        g.setColour (Pal::cream.withAlpha (0.35f)); g.strokePath (st2, strokeRound (1.8f * sc));

        // царапины (зерно фиксировано, не мерцают; с Drive их больше)
        juce::Random rng (seed * 7 + 1);
        const int scratches = 6 + (reactive ? (int) (16.0f * pos) : 0);
        for (int i = 0; i < scratches; ++i)
        {
            const float ra = rng.nextFloat() * juce::MathConstants<float>::twoPi;
            const float rd = std::sqrt (rng.nextFloat()) * R * 0.9f;
            const juce::Point<float> p (c.x + rd * std::cos (ra), c.y + rd * std::sin (ra));
            const float la  = 0.6f + rng.nextFloat() * 0.5f;
            const float len = (0.10f + rng.nextFloat() * 0.25f) * R;
            g.setColour (Pal::cream.withAlpha (0.18f + 0.12f * rng.nextFloat()));
            g.drawLine (p.x, p.y, p.x + len * std::cos (la), p.y + len * std::sin (la), 1.0f * sc);
        }
        g.restoreState();

        // внутреннее кольцо и контур
        g.setColour (Pal::cream.withAlpha (0.22f));
        g.strokePath (makeBlob (c, R * 0.88f, wobble * 0.8f, seed + 3), strokeRound (1.4f * sc));
        g.setColour (Pal::ink);
        g.strokePath (blob, strokeRound ((3.5f + (reactive ? 2.5f * pos : 0.0f)) * sc));

        // ---------- риска-указатель (штрих маркером) ----------
        const juce::Point<float> dir (std::sin (ang), -std::cos (ang));
        const juce::Point<float> perp (-dir.y, dir.x);
        const auto p1  = c + dir * (R * 0.22f);
        const auto p2  = c + dir * (R * 0.88f);
        const auto mid = (p1 + p2) * 0.5f + perp * (R * 0.05f);
        juce::Path ptr;
        ptr.startNewSubPath (p1);
        ptr.quadraticTo (mid, p2);
        g.setColour (Pal::ink);  g.strokePath (ptr, strokeRound (9.0f * sc));
        g.setColour (Pal::pink); g.strokePath (ptr, strokeRound (5.0f * sc));
    }
};
