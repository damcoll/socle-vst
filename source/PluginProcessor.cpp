#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params.h"

SocleProcessor::SocleProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Sortie", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "SOCLE", socle::params::createLayout())
{
    masterParam = parameters.getRawParameterValue ("master");
}

void SocleProcessor::prepareToPlay (double rate, int)
{
    synth.setParams (readParams());
    synth.prepare (rate);
    dcBlocker.prepare (rate);
    masterGain.reset (rate, 0.02);
    masterGain.setCurrentAndTargetValue (masterParam->load());
}

socle::SynthParams SocleProcessor::readParams() const
{
    using Shape = socle::Oscillator::Shape;
    socle::SynthParams p;
    p.shape1 = value ("osc1_forme") < 0.5f ? Shape::saw : Shape::square;
    p.shape2 = value ("osc2_forme") < 0.5f ? Shape::saw : Shape::square;
    p.accord2 = value ("osc2_accord");
    p.desaccord2 = value ("osc2_desaccord");
    p.niveau1 = value ("niv_osc1");
    p.niveau2 = value ("niv_osc2");
    p.niveauSub = value ("niv_sub");
    p.niveauBruit = value ("niv_bruit");
    p.coupure = value ("coupure");
    p.resonance = value ("resonance");
    p.drive = value ("drive");
    p.envFiltre = value ("env_filtre");
    p.velocite = value ("velocite");
    p.fA = value ("envf_a");
    p.fD = value ("envf_d");
    p.fS = value ("envf_s");
    p.fR = value ("envf_r");
    p.aA = value ("enva_a");
    p.aD = value ("enva_d");
    p.aS = value ("enva_s");
    p.aR = value ("enva_r");
    p.mode = (socle::Mode) juce::roundToInt (value ("mode"));
    p.glide = value ("glide");
    p.lfoVitesse = value ("lfo_vitesse");
    p.lfoQuantite = value ("lfo_quantite");
    p.lfoDestination = (socle::LfoDestination) juce::roundToInt (value ("lfo_dest"));
    return p;
}

bool SocleProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SocleProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    synth.setParams (readParams());
    masterGain.setTargetValue (masterParam->load());

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    int sample = 0;

    const auto renderUntil = [&] (int end)
    {
        for (; sample < end; ++sample)
        {
            // MASTER at its default (0.8) is unity gain; tanh tames the peaks of the sum
            const auto out = std::tanh (dcBlocker.process (synth.next()) * 1.25f * masterGain.getNextValue());
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
        else if (msg.isPitchWheel())
        {
            // +/- 2 semitones, the usual range of a keyboard's bend wheel
            synth.setPitchBend (2.0f * (float) (msg.getPitchWheelValue() - 8192) / 8192.0f);
        }
        else if (msg.isControllerOfType (1))
        {
            synth.setModWheel ((float) msg.getControllerValue() / 127.0f);
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
