#pragma once

// The MONOLITHE family look, shared by SOCLE: palette and typefaces.

#include <juce_gui_basics/juce_gui_basics.h>

namespace ui
{
    namespace colours
    {
        const juce::Colour noirAbri  { 0xff0e0e0e }; // front panel
        const juce::Colour ecran     { 0xff1a1916 }; // VU screen background
        const juce::Colour graphite  { 0xff2a2927 }; // knobs, dividers
        const juce::Colour joint     { 0xff3a3936 }; // main dividers
        const juce::Colour beton     { 0xff5a5853 }; // dark pads, screen bezel
        const juce::Colour gris      { 0xff9c9a95 }; // discreet text
        const juce::Colour poussiere { 0xffc9c7c1 }; // knob rings, labels, light pads
        const juce::Colour craie     { 0xffe9e7e2 }; // silkscreen
        const juce::Colour ambre     { 0xffffb000 }; // the only signal: what is alive
    }

    enum class Face { stencil, display, mono, monoBold };

    // the embedded typefaces (OFL), shared by every editor instance
    juce::Font font (Face face, float height, float tracking = 0.0f);

    // keeps the typefaces loaded while an editor is open (they are freed with the last one)
    class FontsLease
    {
    public:
        FontsLease();
        ~FontsLease();

    private:
        struct Holder;
        std::unique_ptr<Holder> holder;
    };

    // UTF-8 literal -> juce::String
    inline juce::String fr (const char* utf8) { return juce::String::fromUTF8 (utf8); }
}
