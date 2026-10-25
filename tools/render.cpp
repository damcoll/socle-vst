// Plays bass lines through the whole plugin with a few settings, one .wav
// each, to tune the sound without a DAW. Prints the peak level of each file.
//
//   socle_render <output folder>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace
{
    constexpr double renderRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr double bpm = 120.0;

    struct Note
    {
        int step, length, note, velocity;
    };

    // a 2-bar bass line in sixteenths, around E1 (note 28)
    const std::vector<Note> ligne {
        { 0, 2, 28, 120 }, { 3, 1, 28, 70 }, { 4, 2, 40, 100 }, { 6, 1, 28, 60 }, { 8, 3, 31, 115 },
        { 11, 1, 33, 75 }, { 12, 2, 35, 105 }, { 14, 2, 28, 80 },
        { 16, 2, 28, 120 }, { 19, 1, 28, 70 }, { 20, 2, 40, 100 }, { 22, 1, 38, 70 }, { 24, 4, 26, 127 },
        { 28, 1, 26, 70 }, { 29, 1, 38, 90 }, { 30, 2, 26, 100 },
    };

    // chords for the polyphony: E minor, C, D, held a bar each
    const std::vector<Note> accords {
        { 0, 15, 40, 100 }, { 0, 15, 43, 90 }, { 0, 15, 47, 90 }, { 0, 15, 28, 110 },
        { 16, 15, 36, 100 }, { 16, 15, 40, 90 }, { 16, 15, 43, 90 }, { 16, 15, 24, 110 },
        { 32, 15, 38, 100 }, { 32, 15, 42, 90 }, { 32, 15, 45, 90 }, { 32, 15, 26, 110 },
    };

    // overlapping notes: each one starts before the previous is released (MONO / LEGATO, glide)
    const std::vector<Note> liee {
        { 0, 5, 28, 110 }, { 4, 5, 40, 90 }, { 8, 5, 31, 100 }, { 12, 5, 35, 90 },
        { 16, 9, 28, 120 }, { 24, 5, 38, 100 }, { 28, 4, 26, 110 },
    };

    const std::vector<Note> tenue { { 0, 63, 33, 110 } };

    juce::RangedAudioParameter* find (juce::AudioProcessor& p, const juce::String& id)
    {
        for (auto* param : p.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param); ranged != nullptr && ranged->getParameterID() == id)
                return ranged;
        jassertfalse;
        return nullptr;
    }

    // value in the parameter's own unit (Hz, seconds, choice index...)
    void set (juce::AudioProcessor& p, const juce::String& id, float value)
    {
        if (auto* param = find (p, id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    }

    struct Setting
    {
        const char* id;
        float value;
    };

    struct Render
    {
        const char* file;
        const std::vector<Note>* notes;
        int repeats;
        std::vector<Setting> settings;
        std::function<void (juce::AudioProcessor&, double progress)> automation = {};
    };
}

int main (int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI juce;
    const auto outDir = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "renders");
    outDir.createDirectory();

    const std::vector<Render> renders {
        { "01_defaut", &ligne, 2, {} },
        { "02_acide", &ligne, 2, { { "osc2_forme", 0 }, { "niv_osc2", 0 }, { "niv_sub", 0.3f }, { "coupure", 220 }, { "resonance", 0.8f },
                                   { "env_filtre", 0.65f }, { "envf_d", 0.18f }, { "envf_s", 0 }, { "drive", 0.6f }, { "velocite", 0.8f } } },
        { "03_sub_rond", &ligne, 2, { { "osc1_forme", 1 }, { "niv_osc2", 0 }, { "niv_sub", 1 }, { "coupure", 180 }, { "resonance", 0.1f },
                                      { "env_filtre", 0.3f }, { "envf_d", 0.3f }, { "drive", 0.2f } } },
        { "04_drive_sale", &ligne, 2, { { "osc2_forme", 0 }, { "osc2_desaccord", 18 }, { "coupure", 500 }, { "resonance", 0.5f }, { "drive", 1 },
                                        { "env_filtre", 0.5f }, { "niv_bruit", 0.15f } } },
        { "05_balayage", &tenue, 1, { { "osc2_forme", 0 }, { "resonance", 0.9f }, { "env_filtre", 0 }, { "enva_s", 1 }, { "drive", 0.3f } },
          [] (juce::AudioProcessor& p, double progress) { set (p, "coupure", (float) (30.0 * std::pow (600.0, 1.0 - std::abs (2.0 * progress - 1.0)))); } },
        { "07_mono_glide", &liee, 2, { { "mode", 1 }, { "glide", 0.08f }, { "osc2_forme", 0 }, { "coupure", 400 }, { "resonance", 0.5f },
                                       { "env_filtre", 0.5f }, { "envf_d", 0.25f } } },
        { "08_legato", &liee, 2, { { "mode", 2 }, { "glide", 0.12f }, { "osc2_forme", 0 }, { "coupure", 400 }, { "resonance", 0.5f },
                                   { "env_filtre", 0.5f }, { "envf_d", 0.25f } } },
        { "09_lfo_filtre", &tenue, 1, { { "osc2_forme", 0 }, { "coupure", 500 }, { "resonance", 0.6f }, { "env_filtre", 0 }, { "enva_s", 1 },
                                        { "lfo_dest", 1 }, { "lfo_vitesse", 3 }, { "lfo_quantite", 0.6f } } },
        { "10_lfo_largeur", &tenue, 1, { { "osc1_forme", 1 }, { "niv_sub", 0.4f }, { "coupure", 2500 }, { "env_filtre", 0 }, { "enva_s", 1 },
                                         { "lfo_dest", 2 }, { "lfo_vitesse", 0.7f }, { "lfo_quantite", 0.9f } } },
        { "11_effets", &ligne, 2, { { "osc2_forme", 0 }, { "coupure", 350 }, { "resonance", 0.6f }, { "env_filtre", 0.55f }, { "envf_d", 0.2f },
                                    { "saturation", 0.7f }, { "chorus", 0.6f }, { "delay", 0.35f }, { "retour", 0.5f } } },
        { "12_propre", &ligne, 2, { { "saturation", 0 }, { "derive", 0 }, { "poussiere", 0 } } },
        { "13_machine_usee", &ligne, 2, { { "derive", 0.75f }, { "poussiere", 0.6f }, { "saturation", 0.5f } } },
        { "14_accords_chorus", &accords, 1, { { "osc2_forme", 0 }, { "coupure", 900 }, { "env_filtre", 0.3f }, { "enva_a", 0.02f }, { "enva_r", 0.6f },
                                              { "chorus", 0.8f }, { "delay", 0.25f }, { "retour", 0.4f }, { "delay_temps", 3 } } },
        { "06_accords", &accords, 1, { { "osc2_forme", 0 }, { "coupure", 900 }, { "env_filtre", 0.3f }, { "enva_a", 0.02f }, { "enva_r", 0.6f } } },
    };

    const auto samplesPerStep = (int) (renderRate * 60.0 / bpm / 4.0);

    for (const auto& render : renders)
    {
        std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
        plugin->setPlayConfigDetails (0, 2, renderRate, blockSize);
        for (const auto& [id, value] : render.settings)
            set (*plugin, id, value);
        plugin->prepareToPlay (renderRate, blockSize);

        int patternSteps = 0;
        for (const auto& n : *render.notes)
            patternSteps = std::max (patternSteps, ((n.step + n.length + 15) / 16) * 16);
        const auto patternSamples = patternSteps * samplesPerStep;
        const auto playedSamples = patternSamples * render.repeats;
        const auto totalSamples = playedSamples + (int) renderRate;

        juce::AudioBuffer<float> out (2, totalSamples);
        juce::AudioBuffer<float> block (2, blockSize);

        for (int start = 0; start < totalSamples; start += blockSize)
        {
            const auto n = std::min (blockSize, totalSamples - start);
            if (render.automation)
                render.automation (*plugin, std::min (1.0, (double) start / playedSamples));

            juce::MidiBuffer midi;
            for (int s = start; s < start + n && s < playedSamples; ++s)
            {
                const auto inPattern = s % patternSamples;
                if (inPattern % samplesPerStep != 0)
                    continue;
                const auto step = inPattern / samplesPerStep;
                // note-offs first, so a note repeated on the same step is retriggered
                for (const auto& note : *render.notes)
                    if (note.step + note.length == step)
                        midi.addEvent (juce::MidiMessage::noteOff (1, note.note), s - start);
                for (const auto& note : *render.notes)
                    if (note.step == step)
                        midi.addEvent (juce::MidiMessage::noteOn (1, note.note, (juce::uint8) note.velocity), s - start);
            }
            if (start + n >= playedSamples && start < playedSamples)
                for (const auto& note : *render.notes)
                    midi.addEvent (juce::MidiMessage::noteOff (1, note.note), std::max (0, playedSamples - 1 - start));

            block.setSize (2, n, false, false, true);
            plugin->processBlock (block, midi);
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, start, block, ch, 0, n);
        }

        const auto file = outDir.getChildFile (juce::String (render.file) + ".wav");
        file.deleteFile();
        std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
        juce::WavAudioFormat wav;
        auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}.withSampleRate (renderRate).withNumChannels (2).withBitsPerSample (24));
        if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (out, 0, totalSamples))
            return 1;
        writer.reset();

        const auto peak = out.getMagnitude (0, 0, totalSamples);
        const auto rms = out.getRMSLevel (0, 0, playedSamples);
        std::printf ("%-14s peak %6.1f dBFS   rms %6.1f dBFS\n", render.file,
                     juce::Decibels::gainToDecibels (peak), juce::Decibels::gainToDecibels (rms));
    }
    return 0;
}
