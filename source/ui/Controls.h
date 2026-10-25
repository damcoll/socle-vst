#pragma once

#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace ui
{
    // Graphite disc with a dust ring and a chalk index line.
    // Vertical drag, mouse wheel, double-click returns to the default value.
    class Knob final : public juce::Slider
    {
    public:
        explicit Knob (float defaultValue);
        void paint (juce::Graphics&) override;
    };

    // Envelope fader: a dark slot and a chalk cap. Double-click returns to the default value.
    class Fader final : public juce::Slider
    {
    public:
        explicit Fader (float defaultValue);
        void paint (juce::Graphics&) override;
    };

    // A row or column of keys for a choice parameter; the chosen one is lit in amber
    class Segmented final : public juce::Component
    {
    public:
        Segmented (juce::RangedAudioParameter& parameter, const juce::StringArray& labels, bool vertical);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;

    private:
        int keyAt (juce::Point<int>) const;
        juce::Rectangle<float> keyBounds (int index) const;

        juce::StringArray labels;
        bool vertical;
        int selected = 0, hovered = -1;
        juce::ParameterAttachment attachment;
    };

    // Small amber screen showing a choice; click for the list, mouse wheel to step through
    class Selector final : public juce::Component
    {
    public:
        Selector (juce::RangedAudioParameter& parameter, const juce::StringArray& labels);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    private:
        juce::StringArray labels;
        int selected = 0;
        juce::ParameterAttachment attachment;
    };

    // Momentary key with a mono label (preset arrows, SAUVER)
    class Key final : public juce::Button
    {
    public:
        explicit Key (const juce::String& label) : juce::Button (label) {}
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    };

    // The preset screen in the top bar: number, name in amber, a star once edited. Click for the list.
    class PresetScreen final : public juce::Component
    {
    public:
        PresetScreen() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }

        void show (const juce::String& number, const juce::String& name, bool modified);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;

        std::function<void()> onClick;

    private:
        juce::String number, name;
        bool modified = false;
    };

    // Amber screen with a VU needle and the last note played
    class VuMeter final : public juce::Component
    {
    public:
        // peak since last call (linear) and the text in the bottom right corner
        void update (float peak, const juce::String& label);
        void paint (juce::Graphics&) override;

    private:
        float needle = 0.0f; // 0..1 across the scale
        juce::String name;
    };
}
