#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Presets.h"
#include "dsp/Effects.h"
#include "dsp/Machine.h"
#include "dsp/Synth.h"

#include <atomic>

class SocleProcessor final : public juce::AudioProcessor
{
public:
    SocleProcessor();
    ~SocleProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return socle::Delay::maxSeconds; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;
    PresetManager presets { parameters };

    // read by the editor (message thread): MIDI LED, last note, LFO light
    std::atomic<int> noteCounter { 0 };
    std::atomic<int> lastNote { -1 };
    std::atomic<float> lfoValue { 0.0f };

    // loudest sample since the editor last read it (read with exchange (0)): VU needle
    std::atomic<float> outputPeak { 0.0f };

private:
    socle::Synth synth;
    socle::DcBlocker dcBlocker;
    socle::Saturation saturation;
    socle::Dust dust;
    socle::Chorus chorus;
    socle::Delay delay;
    juce::SmoothedValue<float> masterGain;

    // the panel's current values, gathered for the synth once per block
    socle::SynthParams readParams() const;
    float value (const char* id) const { return parameters.getRawParameterValue (id)->load(); }

    std::atomic<float>* masterParam = nullptr;

    // lets callbacks posted to the message thread know the processor still exists
    std::shared_ptr<bool> aliveFlag = std::make_shared<bool> (true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SocleProcessor)
};
