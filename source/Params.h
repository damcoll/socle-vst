#pragma once

// Every SOCLE parameter, as on the front panel. The IDs are stable: Ableton
// automation and saved sets depend on them, never rename one.

#include <juce_audio_processors/juce_audio_processors.h>

namespace socle::params
{
    inline juce::String fr (const char* utf8) { return juce::String::fromUTF8 (utf8); }

    inline const juce::StringArray formes { "SCIE", fr ("CARRÉ") };
    inline const juce::StringArray modes { "POLY", "MONO", "LEGATO" };
    inline const juce::StringArray destinations { "HAUTEUR", "FILTRE", "LARGEUR" };

    // 0..1 knob with its centre at `centre` (times, frequencies)
    inline juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> range (lo, hi);
        range.setSkewForCentre (centre);
        return range;
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using Float = juce::AudioParameterFloat;
        using Choice = juce::AudioParameterChoice;
        using Attributes = juce::AudioParameterFloatAttributes;
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        const juce::NormalisableRange<float> unit (0.0f, 1.0f);

        const auto add = [&] (const char* id, const char* name, juce::NormalisableRange<float> range, float def, const char* label = "")
        {
            layout.add (std::make_unique<Float> (juce::ParameterID { id, 1 }, fr (name), range, def, Attributes().withLabel (label)));
        };

        // 01 OSCILLATEURS
        layout.add (std::make_unique<Choice> (juce::ParameterID { "osc1_forme", 1 }, "FORME 1", formes, 0));
        layout.add (std::make_unique<Choice> (juce::ParameterID { "osc2_forme", 1 }, "FORME 2", formes, 1));
        add ("osc2_accord", "ACCORD 2", { -24.0f, 24.0f, 1.0f }, 0.0f, "dt");
        add ("osc2_desaccord", "DÉSACCORD", { -50.0f, 50.0f }, 7.0f, "ct");
        add ("niv_osc1", "OSC 1", unit, 0.8f);
        add ("niv_osc2", "OSC 2", unit, 0.6f);
        add ("niv_sub", "SUB", unit, 0.7f);
        add ("niv_bruit", "BRUIT", unit, 0.0f);

        // 02 FILTRE
        add ("coupure", "COUPURE", skewed (30.0f, 18000.0f, 800.0f), 1200.0f, "Hz");
        add ("resonance", "RÉSONANCE", unit, 0.3f);
        add ("drive", "DRIVE", unit, 0.4f);
        add ("env_filtre", "ENV", unit, 0.5f);
        add ("velocite", "VÉLOCITÉ", unit, 0.4f);

        // 03 JEU
        layout.add (std::make_unique<Choice> (juce::ParameterID { "mode", 1 }, "MODE", modes, 0));
        add ("glide", "GLIDE", skewed (0.0f, 2.0f, 0.2f), 0.0f, "s");

        // 04 ENVELOPPES (seconds, sustain 0..1)
        add ("envf_a", "FILTRE A", skewed (0.001f, 5.0f, 0.2f), 0.002f, "s");
        add ("envf_d", "FILTRE D", skewed (0.005f, 10.0f, 0.5f), 0.4f, "s");
        add ("envf_s", "FILTRE S", unit, 0.2f);
        add ("envf_r", "FILTRE R", skewed (0.005f, 10.0f, 0.5f), 0.3f, "s");
        add ("enva_a", "AMPLI A", skewed (0.001f, 5.0f, 0.2f), 0.002f, "s");
        add ("enva_d", "AMPLI D", skewed (0.005f, 10.0f, 0.5f), 0.6f, "s");
        add ("enva_s", "AMPLI S", unit, 0.8f);
        add ("enva_r", "AMPLI R", skewed (0.005f, 10.0f, 0.5f), 0.2f, "s");

        // 05 LFO
        add ("lfo_vitesse", "VITESSE", skewed (0.05f, 20.0f, 2.0f), 2.0f, "Hz");
        add ("lfo_quantite", "QUANTITÉ", unit, 0.0f);
        layout.add (std::make_unique<Choice> (juce::ParameterID { "lfo_dest", 1 }, "DESTINATION", destinations, 1));

        // 06 EFFETS
        add ("saturation", "SATURATION", unit, 0.3f);
        add ("chorus", "CHORUS", unit, 0.0f);
        add ("delay", "DELAY", unit, 0.0f);
        add ("retour", "RETOUR", unit, 0.35f);

        // MACHINE + sortie
        add ("derive", "DÉRIVE", unit, 0.2f);
        add ("poussiere", "POUSSIÈRE", unit, 0.1f);
        add ("master", "MASTER", unit, 0.8f);
        return layout;
    }
}
