#pragma once

#include "PluginProcessor.h"
#include "ui/Controls.h"

// The SOCLE front panel, drawn at 1100 x 600 and scaled to the window size.
class SocleEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit SocleEditor (SocleProcessor&);
    ~SocleEditor() override = default;

    void resized() override;

    static constexpr int panelWidth = 1100, panelHeight = 600;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    // a knob or a fader, its parameter and its label under it
    struct Control
    {
        Control (SocleProcessor&, const char* id, const char* label, bool fader);

        std::unique_ptr<juce::Slider> slider;
        SliderAttachment attachment;
        juce::String label;
    };

    // everything lives in this fixed-size panel, scaled as one block
    struct Panel final : public juce::Component
    {
        explicit Panel (SocleProcessor&);
        void paint (juce::Graphics&) override;
        void resized() override;

        Control& add (const char* id, const char* label, bool fader = false);

        // presets: the arrows step through the list, the screen opens it, SAUVER asks for a name
        void stepPreset (int delta);
        void showPresetMenu();
        void askPresetName();
        void refreshPresetScreen();

        SocleProcessor& processor;
        std::vector<std::unique_ptr<Control>> controls;
        ui::Segmented forme1, forme2, mode, destination;
        ui::Selector delayTemps;
        ui::Key previous { juce::String::fromUTF8 ("◀") }, next { juce::String::fromUTF8 ("▶") }, save { "SAUVER" };
        ui::PresetScreen presetScreen;
        ui::VuMeter vu;

        float ledLevel = 0.0f, lfoLevel = 0.0f;
    };

    void timerCallback() override;

    SocleProcessor& processor;
    ui::FontsLease fonts;
    Panel panel;
    int lastNoteCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SocleEditor)
};
