#pragma once

// Factory presets (built into the plugin) and the user's own, saved as
// readable .socle files in Documents/SOCLE/Presets. A preset stores every
// parameter in its own unit (Hz, seconds...); a missing one gets its default,
// so presets keep loading when new parameters appear.
// Everything here runs on the message thread.

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager
{
public:
    struct Preset
    {
        juce::String name;
        bool factory = true;
        juce::String xml;  // factory presets
        juce::File file;   // user presets
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState&);

    // factory presets first, then the user's, by name
    const std::vector<Preset>& all() const { return presets; }
    int numFactory() const { return factoryCount; }
    void rescanUserFolder();

    void load (int index);
    bool save (const juce::String& name);

    // the preset loaded last (-1: none) and whether a control moved since
    int current() const { return currentIndex; }
    juce::String currentName() const;
    bool isModified() const;

    // after the host restored a saved set: find the preset by the name stored with it
    void restoreFromState();

    static juce::File userFolder();
    static juce::String fileExtension() { return ".socle"; }

private:
    std::vector<float> valuesFrom (const juce::XmlElement&) const;
    void apply (const juce::XmlElement&);
    void takeSnapshot();

    juce::AudioProcessorValueTreeState& state;
    std::vector<Preset> presets;
    int factoryCount = 0;
    int currentIndex = -1;
    std::vector<float> snapshot; // normalised values right after loading
};
