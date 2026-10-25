#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params.h"

SocleProcessor::SocleProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Sortie", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "SOCLE", socle::params::createLayout())
{
    masterParam = parameters.getRawParameterValue ("master");

    // a new SOCLE starts on the first factory preset
    presets.load (0);
}

void SocleProcessor::prepareToPlay (double rate, int)
{
    synth.setParams (readParams());
    synth.prepare (rate);
    dcBlocker.prepare (rate);
    dust.prepare (rate);
    chorus.prepare (rate);
    delay.prepare (rate);
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
    p.derive = value ("derive");
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
    saturation.setAmount (value ("saturation"));
    dust.setAmount (value ("poussiere"));
    chorus.setAmount (value ("chorus"));

    // the delay follows Ableton's tempo (120 BPM when the host gives none)
    double bpm = 120.0;
    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
            bpm = position->getBpm().orFallback (120.0);
    const auto division = socle::delayDivisions[juce::jlimit (0, 5, juce::roundToInt (value ("delay_temps")))];
    delay.set (value ("delay"), value ("retour"), division * 60.0 / juce::jmax (20.0, bpm));

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    int sample = 0;

    const auto renderUntil = [&] (int end)
    {
        for (; sample < end; ++sample)
        {
            // mono until the chorus: SATURATION, POUSSIÈRE, then CHORUS and DELAY in stereo
            const auto mono = dust.process (saturation.process (dcBlocker.process (synth.next())));
            float l, r;
            chorus.process (mono, l, r);
            delay.process (l, r);

            // MASTER at its default (0.8) is unity gain; tanh tames the peaks
            const auto gain = 1.25f * masterGain.getNextValue();
            left[sample] = std::tanh (l * gain);
            right[sample] = std::tanh (r * gain);
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
            noteCounter.fetch_add (1, std::memory_order_relaxed);
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

    lfoValue.store (synth.lfoValue, std::memory_order_relaxed);
    const auto peak = buffer.getMagnitude (0, buffer.getNumSamples());
    if (peak > outputPeak.load (std::memory_order_relaxed))
        outputPeak.store (peak, std::memory_order_relaxed);
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
    {
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
        // the host may restore from its own thread: the preset bookkeeping lives on the message thread
        juce::MessageManager::callAsync ([this, alive = std::weak_ptr<bool> (aliveFlag)]
        {
            if (! alive.expired())
                presets.restoreFromState();
        });
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SocleProcessor();
}
