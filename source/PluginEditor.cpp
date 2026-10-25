#include "PluginEditor.h"
#include "Params.h"

using namespace ui;

namespace
{
    // layout of the mockup (px on the 1100 x 600 panel)
    constexpr int topBar = 64, leftColumn = 210, rowSplit = 332;
    constexpr int row1 = topBar, row2 = rowSplit;

    // every knob and fader: parameter, label, centre, size (faders: width 28, height 104)
    struct Place
    {
        const char* id;
        const char* label;
        int cx, cy, size;
        bool fader = false;
    };

    constexpr int faderWidth = 28, faderHeight = 104, faderTop = row2 + 62;
    constexpr int faderX (int group, int index) { return 232 + group * 158 + index * 34 + faderWidth / 2; }

    const Place places[] = {
        // left: output
        { "master", "MASTER", 105, 320, 76 },
        // 01 OSCILLATEURS
        { "osc2_accord", "ACCORD 2", 482, row1 + 100, 44 },
        { "osc2_desaccord", "DÉSACCORD", 584, row1 + 100, 44 },
        { "niv_osc1", "OSC 1", 278, row1 + 190, 44 },
        { "niv_osc2", "OSC 2", 380, row1 + 190, 44 },
        { "niv_sub", "SUB", 482, row1 + 190, 44 },
        { "niv_bruit", "BRUIT", 584, row1 + 190, 44 },
        // 02 FILTRE
        { "coupure", "COUPURE", 697, row1 + 125, 84 },
        { "resonance", "RÉSONANCE", 797, row1 + 85, 40 },
        { "drive", "DRIVE", 857, row1 + 85, 40 },
        { "env_filtre", "ENV", 797, row1 + 165, 40 },
        { "velocite", "VÉLOCITÉ", 857, row1 + 165, 40 },
        // 03 JEU
        { "glide", "GLIDE", 1005, row1 + 190, 44 },
        // 04 ENVELOPPES
        { "envf_a", "A", faderX (0, 0), 0, 0, true }, { "envf_d", "D", faderX (0, 1), 0, 0, true },
        { "envf_s", "S", faderX (0, 2), 0, 0, true }, { "envf_r", "R", faderX (0, 3), 0, 0, true },
        { "enva_a", "A", faderX (1, 0), 0, 0, true }, { "enva_d", "D", faderX (1, 1), 0, 0, true },
        { "enva_s", "S", faderX (1, 2), 0, 0, true }, { "enva_r", "R", faderX (1, 3), 0, 0, true },
        // 05 LFO
        { "lfo_vitesse", "VITESSE", 601, row2 + 95, 40 },
        { "lfo_quantite", "QUANTITÉ", 679, row2 + 95, 40 },
        // 06 EFFETS
        { "saturation", "SATURATION", 811, row2 + 78, 40 },
        { "chorus", "CHORUS", 909, row2 + 78, 40 },
        { "delay", "DELAY", 811, row2 + 142, 40 },
        { "retour", "RETOUR", 909, row2 + 142, 40 },
        // MACHINE
        { "derive", "DÉRIVE", 1040, row2 + 98, 40 },
        { "poussiere", "POUSSIÈRE", 1040, row2 + 173, 40 },
    };

    // Ableton names middle C (60) C3, so 36 is C1
    juce::String noteName (int note) { return juce::MidiMessage::getMidiNoteName (note, true, true, 3); }

    juce::RangedAudioParameter& parameter (SocleProcessor& p, const char* id) { return *p.parameters.getParameter (id); }

    void centredText (juce::Graphics& g, const juce::String& text, float centreX, int y, int height, juce::Font f, juce::Colour c)
    {
        g.setFont (f);
        g.setColour (c);
        g.drawText (text, juce::Rectangle<float> (centreX - 60.0f, (float) y, 120.0f, (float) height), juce::Justification::centred);
    }

    void label (juce::Graphics& g, const juce::String& text, float centreX, int y)
    {
        centredText (g, text, centreX, y, 12, font (Face::mono, 10.0f, 0.08f), colours::poussiere);
    }

    // "01  OSCILLATEURS"
    void sectionTitle (juce::Graphics& g, const char* number, const char* title, int x, int y)
    {
        g.setColour (colours::gris);
        g.setFont (font (Face::mono, 10.0f));
        g.drawText (number, x + 22, y + 22, 20, 12, juce::Justification::centredLeft);
        g.setColour (colours::craie);
        g.setFont (font (Face::display, 24.0f, 0.03f));
        g.drawText (fr (title), x + 44, y + 12, 220, 30, juce::Justification::centredLeft);
    }
}

SocleEditor::Control::Control (SocleProcessor& p, const char* id, const char* text, bool fader)
    : slider (fader ? std::unique_ptr<juce::Slider> (std::make_unique<Fader> (parameter (p, id).getDefaultValue()))
                    : std::unique_ptr<juce::Slider> (std::make_unique<Knob> (parameter (p, id).getDefaultValue()))),
      attachment (p.parameters, id, *slider),
      label (fr (text))
{
    // the double-click default is in the slider's own range (the parameter's unit)
    auto& param = parameter (p, id);
    slider->setDoubleClickReturnValue (true, param.convertFrom0to1 (param.getDefaultValue()));
    slider->setTitle (param.getName (64));
}

SocleEditor::Panel::Panel (SocleProcessor& p)
    : processor (p),
      forme1 (parameter (p, "osc1_forme"), socle::params::formes, true),
      forme2 (parameter (p, "osc2_forme"), socle::params::formes, true),
      mode (parameter (p, "mode"), socle::params::modes, true),
      destination (parameter (p, "lfo_dest"), { "HAUT.", "FILTRE", "LARG." }, false),
      delayTemps (parameter (p, "delay_temps"), socle::params::divisions)
{
    for (const auto& place : places)
        add (place.id, place.label, place.fader);

    for (juce::Component* c : { (juce::Component*) &forme1, (juce::Component*) &forme2, (juce::Component*) &mode,
                                (juce::Component*) &destination, (juce::Component*) &delayTemps, (juce::Component*) &vu,
                                (juce::Component*) &previous, (juce::Component*) &next, (juce::Component*) &save })
        addAndMakeVisible (c);

    // presets come in the next step
    previous.setTitle (fr ("Preset précédent"));
    next.setTitle ("Preset suivant");
}

SocleEditor::Control& SocleEditor::Panel::add (const char* id, const char* text, bool fader)
{
    auto& control = *controls.emplace_back (std::make_unique<Control> (processor, id, text, fader));
    addAndMakeVisible (*control.slider);
    return control;
}

void SocleEditor::Panel::resized()
{
    for (size_t i = 0; i < std::size (places); ++i)
    {
        const auto& place = places[i];
        auto& slider = *controls[i]->slider;
        if (place.fader)
            slider.setBounds (place.cx - faderWidth / 2, faderTop, faderWidth, faderHeight);
        else
            slider.setBounds (place.cx - place.size / 2, place.cy - place.size / 2, place.size, place.size);
    }

    forme1.setBounds (278 - 36, row1 + 78, 72, 44);
    forme2.setBounds (380 - 36, row1 + 78, 72, 44);
    mode.setBounds (1005 - 48, row1 + 62, 96, 68);
    destination.setBounds (564, row2 + 160, 152, 20);
    delayTemps.setBounds (812, row2 + 192, 120, 24);

    vu.setBounds (15, 84, 180, 124);
    previous.setBounds (612, 16, 32, 32);
    next.setBounds (880, 16, 32, 32);
    save.setBounds (920, 16, 80, 32);
}

void SocleEditor::Panel::paint (juce::Graphics& g)
{
    g.fillAll (colours::noirAbri);

    // dividers: the main ones, then between the sections of each row
    g.setColour (colours::joint);
    g.fillRect (0, topBar - 1, panelWidth, 1);
    g.fillRect (leftColumn - 1, topBar, 1, panelHeight - topBar);
    g.fillRect (leftColumn, rowSplit - 1, panelWidth - leftColumn, 1);
    g.setColour (colours::graphite);
    for (const auto x : { 620, 910 })
        g.fillRect (x, row1 + 16, 1, rowSplit - row1 - 32);
    for (const auto x : { 540, 740, 980 })
        g.fillRect (x, row2 + 16, 1, panelHeight - row2 - 32);

    // top bar: stencil logo, subtitle, preset screen, MIDI LED
    const auto logoFont = font (Face::stencil, 40.0f);
    const auto logoWidth = (int) std::ceil (juce::GlyphArrangement::getStringWidth (logoFont, "SOCLE"));
    g.setColour (colours::craie);
    g.setFont (logoFont);
    g.drawText ("SOCLE", 24, 10, logoWidth + 4, 46, juce::Justification::centredLeft);
    g.setColour (colours::gris);
    g.setFont (font (Face::mono, 11.0f, 0.16f));
    g.drawText (fr ("BASSE · 8 VOIX · SOUSTRACTIF"), 24 + logoWidth + 18, 25, 320, 16, juce::Justification::centredLeft);

    const juce::Rectangle<float> screen (652.0f, 16.0f, 220.0f, 32.0f);
    g.setColour (colours::ecran);
    g.fillRect (screen);
    g.setColour (colours::beton);
    g.drawRect (screen, 2.0f);
    g.setColour (colours::gris);
    g.setFont (font (Face::mono, 11.0f));
    g.drawText ("--", screen.withTrimmedLeft (12.0f), juce::Justification::centredLeft);
    g.setColour (colours::ambre);
    g.setFont (font (Face::monoBold, 12.0f, 0.12f));
    g.drawText (presetName, screen.withTrimmedLeft (40.0f), juce::Justification::centredLeft);

    g.setColour (colours::graphite.interpolatedWith (colours::ambre, ledLevel));
    g.fillEllipse (1030.0f, 27.0f, 10.0f, 10.0f);
    g.setColour (colours::craie);
    g.setFont (font (Face::mono, 11.0f, 0.16f));
    g.drawText ("MIDI", 1046, 24, 50, 16, juce::Justification::centredLeft);

    // left: output
    centredText (g, "SORTIE", 105.0f, 216, 12, font (Face::mono, 10.0f, 0.1f), colours::gris);
    centredText (g, fr ("v0.1 · CODE OUVERT"), 105.0f, 572, 12, font (Face::mono, 10.0f, 0.05f), colours::gris);

    // section titles and the labels of the selectors
    sectionTitle (g, "01", "OSCILLATEURS", 210, row1);
    label (g, "FORME 1", 278.0f, row1 + 130);
    label (g, "FORME 2", 380.0f, row1 + 130);

    sectionTitle (g, "02", "FILTRE", 620, row1);
    g.setColour (colours::gris);
    g.setFont (font (Face::mono, 10.0f));
    g.drawText ("24 dB", 735, row1 + 22, 50, 12, juce::Justification::centredLeft);

    sectionTitle (g, "03", "JEU", 910, row1);
    label (g, "MODE", 1005.0f, row1 + 138);

    sectionTitle (g, "04", "ENVELOPPES", 210, row2);
    centredText (g, "FILTRE", (float) (faderX (0, 0) + faderX (0, 3)) * 0.5f, faderTop + faderHeight + 26, 12, font (Face::mono, 10.0f, 0.1f), colours::gris);
    centredText (g, "AMPLI", (float) (faderX (1, 0) + faderX (1, 3)) * 0.5f, faderTop + faderHeight + 26, 12, font (Face::mono, 10.0f, 0.1f), colours::gris);

    sectionTitle (g, "05", "LFO", 540, row2);
    g.setColour (colours::graphite.interpolatedWith (colours::ambre, lfoLevel));
    g.fillEllipse (628.0f, (float) row2 + 24.0f, 9.0f, 9.0f);
    label (g, "DESTINATION", 640.0f, row2 + 186);

    sectionTitle (g, "06", "EFFETS", 740, row2);
    g.setColour (colours::poussiere);
    g.setFont (font (Face::mono, 10.0f, 0.08f));
    g.drawText ("TEMPS", 762, row2 + 198, 46, 12, juce::Justification::centredLeft);

    centredText (g, "MACHINE", 1040.0f, row2 + 12, 30, font (Face::display, 24.0f, 0.03f), colours::craie);

    // labels under the knobs and faders
    for (size_t i = 0; i < std::size (places); ++i)
    {
        const auto& place = places[i];
        if (place.fader)
            centredText (g, controls[i]->label, (float) place.cx, faderTop + faderHeight + 6, 12, font (Face::mono, 10.0f), colours::poussiere);
        else
            label (g, controls[i]->label, (float) place.cx, place.cy + place.size / 2 + 6);
    }
}

SocleEditor::SocleEditor (SocleProcessor& p)
    : AudioProcessorEditor (&p), processor (p), panel (p)
{
    addAndMakeVisible (panel);

    // resizable from 75 % to 200 %, keeping the proportions; the size is saved with the set
    setResizable (true, true);
    setResizeLimits (panelWidth * 3 / 4, panelHeight * 3 / 4, panelWidth * 2, panelHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) panelWidth / panelHeight);
    const auto savedWidth = (int) p.parameters.state.getProperty ("uiWidth", panelWidth);
    setSize (savedWidth, savedWidth * panelHeight / panelWidth);

    startTimerHz (30);
}

void SocleEditor::resized()
{
    const auto scale = (float) getWidth() / (float) panelWidth;
    panel.setBounds (0, 0, panelWidth, panelHeight);
    panel.setTransform (juce::AffineTransform::scale (scale));
    processor.parameters.state.setProperty ("uiWidth", getWidth(), nullptr);
}

void SocleEditor::timerCallback()
{
    // MIDI LED: amber flash on every note, fading out
    const auto count = processor.noteCounter.load (std::memory_order_relaxed);
    const auto newNote = count != lastNoteCount;
    lastNoteCount = count;
    const auto led = newNote ? 1.0f : panel.ledLevel * 0.8f;
    if (std::abs (led - panel.ledLevel) > 0.01f || newNote)
    {
        panel.ledLevel = led;
        panel.repaint (1024, 20, 24, 24);
    }

    // LFO light: follows the triangle while the LFO does something
    const auto quantite = processor.parameters.getRawParameterValue ("lfo_quantite")->load();
    const auto lfo = quantite > 0.0f ? 0.5f + 0.5f * processor.lfoValue.load (std::memory_order_relaxed) : 0.0f;
    if (std::abs (lfo - panel.lfoLevel) > 0.02f)
    {
        panel.lfoLevel = lfo;
        panel.repaint (624, row2 + 20, 18, 18);
    }

    const auto note = processor.lastNote.load (std::memory_order_relaxed);
    panel.vu.update (processor.outputPeak.exchange (0.0f, std::memory_order_relaxed), note >= 0 ? noteName (note) : juce::String());
}
