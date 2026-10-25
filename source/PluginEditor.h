#pragma once

#include "PluginProcessor.h"

// Temporary window until the real front panel (step 5): every parameter as a
// generic slider, plus the last MIDI note received.
class SocleEditor final : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit SocleEditor (SocleProcessor&);
    ~SocleEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    SocleProcessor& processor;
    juce::GenericAudioProcessorEditor generic;
    int shownNote = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SocleEditor)
};
