#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params.h"

SocleProcessor::SocleProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Sortie", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "SOCLE", socle::params::createLayout())
{
    masterParam = parameters.getRawParameterValue ("master");
    attackParam = parameters.getRawParameterValue ("enva_a");
    releaseParam = parameters.getRawParameterValue ("enva_r");
}

void SocleProcessor::prepareToPlay (double rate, int)
{
    sampleRate = rate;
    synth.prepare (rate);
    masterGain.reset (rate, 0.02);
    masterGain.setCurrentAndTargetValue (masterParam->load());
}

bool SocleProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SocleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    synth.setEnvelope (attackParam->load(), releaseParam->load(), sampleRate);
    masterGain.setTargetValue (masterParam->load());

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    int sample = 0;

    const auto renderUntil = [&] (int end)
    {
        for (; sample < end; ++sample)
        {
            // MASTER at its default (0.8) is unity gain; tanh tames the peaks of the sum
            const auto out = std::tanh (synth.next() * 0.3f * 1.25f * masterGain.getNextValue());
            left[sample] = out;
            right[sample] = out;
        }
    };

    // render sample-accurately between MIDI events
    for (const auto meta : midi)
    {
        renderUntil (meta.samplePosition);
        const auto msg = meta.getMessage();

        if (msg.isNoteOn())
        {
            lastNote.store (msg.getNoteNumber(), std::memory_order_relaxed);
            synth.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        }
        else if (msg.isNoteOff())
        {
            synth.noteOff (msg.getNoteNumber());
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            synth.allNotesOff();
        }
    }
    renderUntil (buffer.getNumSamples());
}

juce::AudioProcessorEditor* SocleProcessor::createEditor()
{
    return new SocleEditor (*this);
}

void SocleProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SocleProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SocleProcessor();
}
