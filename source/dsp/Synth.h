#pragma once

// The 8 voices and who plays which note.
// One voice: OSC 1 + OSC 2 + SUB + BRUIT -> ladder filter -> amp, each with its ADSR.

#include "Blocks.h"

#include <array>

namespace socle
{
    constexpr int numVoices = 8;

    // the front panel's values, read once per block
    struct SynthParams
    {
        Oscillator::Shape shape1 = Oscillator::Shape::saw, shape2 = Oscillator::Shape::square;
        float accord2 = 0.0f, desaccord2 = 7.0f; // semitones, cents
        float niveau1 = 0.8f, niveau2 = 0.6f, niveauSub = 0.7f, niveauBruit = 0.0f;
        float coupure = 1200.0f, resonance = 0.3f, drive = 0.4f, envFiltre = 0.5f, velocite = 0.4f;
        float fA = 0.002f, fD = 0.4f, fS = 0.2f, fR = 0.3f;
        float aA = 0.002f, aD = 0.6f, aS = 0.8f, aR = 0.2f;
    };

    struct Voice
    {
        void prepare (double rate)
        {
            sampleRate = rate;
            filter.prepare (rate);
            ampEnv.kill();
            filterEnv.kill();
            note = -1;
        }

        void start (int midiNote, float vel, uint32_t order)
        {
            const auto wasSilent = ampEnv.isIdle();
            note = midiNote;
            velocity = vel;
            age = order;
            if (wasSilent)
            {
                // a fresh note starts from the same phase every time: the attack punches the same way
                osc1.reset();
                osc2.reset();
                sub.reset();
                filter.reset();
            }
            ampEnv.gateOn();
            filterEnv.gateOn();
        }

        void stop()
        {
            ampEnv.gateOff();
            filterEnv.gateOff();
        }

        bool isActive() const { return ! ampEnv.isIdle(); }
        bool isHeld() const { return isActive() && ! ampEnv.isReleasing(); }

        // once per block: pitches and envelope times
        void update (const SynthParams& p)
        {
            ampEnv.setTimes (p.aA, p.aD, p.aS, p.aR, sampleRate);
            filterEnv.setTimes (p.fA, p.fD, p.fS, p.fR, sampleRate);
            const auto f1 = noteToHz (note);
            osc1.setFreq (f1, sampleRate);
            osc2.setFreq (f1 * std::pow (2.0, (p.accord2 + p.desaccord2 / 100.0) / 12.0), sampleRate);
            sub.setFreq (f1 * 0.5, sampleRate);
        }

        // cutoffOctaves: COUPURE as octaves above 20 Hz, smoothed by the synth
        float next (const SynthParams& p, float cutoffOctaves)
        {
            auto mix = osc1.next (p.shape1) * p.niveau1
                     + osc2.next (p.shape2) * p.niveau2
                     + sub.next (Oscillator::Shape::square) * p.niveauSub
                     + noise.next() * p.niveauBruit;

            // ENV opens up to 7 octaves; the cutoff follows the keyboard by half;
            // VÉLOCITÉ opens the filter on hard notes and softens the quiet ones
            const auto fEnv = filterEnv.next();
            const auto octaves = cutoffOctaves
                               + 7.0f * p.envFiltre * fEnv
                               + 0.5f * (float) (note - 48) / 12.0f
                               + 3.0f * p.velocite * (velocity - 0.5f);
            filter.set (20.0 * std::exp2 ((double) octaves), p.resonance, p.drive);

            const auto level = 1.0f - p.velocite * (1.0f - velocity) * 0.7f;
            const auto out = filter.process (mix * 0.35f) * ampEnv.next() * level;
            if (ampEnv.isIdle())
                note = -1;
            return out;
        }

        Oscillator osc1, osc2, sub;
        Noise noise;
        Ladder filter;
        Adsr ampEnv, filterEnv;
        double sampleRate = 44100.0;
        int note = -1;
        float velocity = 1.0f;
        uint32_t age = 0;
    };

    struct Synth
    {
        void prepare (double rate)
        {
            sampleRate = rate;
            uint32_t seed = 0x1234567u;
            for (auto& v : voices)
            {
                v.prepare (rate);
                v.noise.state = (seed += 0x9e3779b9u);
            }
            smoothedCutoff = cutoffOctaves (params.coupure);
            // ~5 ms to follow a turned COUPURE knob without zipper noise
            smoothing = 1.0f - std::exp (-1.0f / (0.005f * (float) rate));
        }

        void setParams (const SynthParams& p)
        {
            params = p;
            for (auto& v : voices)
                if (v.isActive())
                    v.update (params);
        }

        void noteOn (int note, float velocity)
        {
            // the same note again: retrigger its voice; else a free voice; else steal the oldest
            Voice* chosen = nullptr;
            for (auto& v : voices)
                if (v.note == note)
                    chosen = &v;

            if (chosen == nullptr)
                for (auto& v : voices)
                    if (! v.isActive())
                    {
                        chosen = &v;
                        break;
                    }

            if (chosen == nullptr)
            {
                chosen = &voices[0];
                for (auto& v : voices)
                    if (v.age < chosen->age)
                        chosen = &v;
            }

            chosen->start (note, velocity, ++counter);
            chosen->update (params);
        }

        void noteOff (int note)
        {
            for (auto& v : voices)
                if (v.note == note)
                    v.stop();
        }

        void allNotesOff()
        {
            for (auto& v : voices)
                v.stop();
        }

        float next()
        {
            smoothedCutoff += (cutoffOctaves (params.coupure) - smoothedCutoff) * smoothing;
            float mix = 0.0f;
            for (auto& v : voices)
                if (v.isActive())
                    mix += v.next (params, smoothedCutoff);
            return mix;
        }

        static float cutoffOctaves (float hz) { return std::log2 (hz / 20.0f); }

        std::array<Voice, numVoices> voices;
        SynthParams params;
        double sampleRate = 44100.0;
        float smoothedCutoff = 0.0f, smoothing = 0.01f;
        uint32_t counter = 0;
    };
}
