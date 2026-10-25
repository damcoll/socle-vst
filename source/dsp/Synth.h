#pragma once

// The 8 voices and who plays which note. Step 1: one sawtooth per voice
// through a gate envelope; oscillators, filter and ADSR come in step 2.

#include "Blocks.h"

#include <array>

namespace socle
{
    constexpr int numVoices = 8;

    struct Voice
    {
        void prepare (double rate) { sampleRate = rate; }

        void start (int midiNote, float velocity, uint32_t order)
        {
            note = midiNote;
            age = order;
            saw.setFreq (noteToHz (note), sampleRate);
            env.open (0.25f + 0.75f * velocity);
        }

        void stop() { env.close(); }
        bool isActive() const { return note >= 0; }
        bool isHeld() const { return note >= 0 && env.target > 0.0f; }

        float next()
        {
            const auto out = saw.next() * env.next();
            if (env.isSilent())
                note = -1;
            return out;
        }

        Saw saw;
        GateEnvelope env;
        double sampleRate = 44100.0;
        int note = -1;
        uint32_t age = 0;
    };

    struct Synth
    {
        void prepare (double rate)
        {
            for (auto& v : voices)
            {
                v.prepare (rate);
                v.env.value = v.env.target = 0.0f;
                v.note = -1;
            }
        }

        void setEnvelope (double attack, double release, double rate)
        {
            for (auto& v : voices)
                v.env.setTimes (attack, release, rate);
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
            float mix = 0.0f;
            for (auto& v : voices)
                if (v.isActive())
                    mix += v.next();
            return mix;
        }

        std::array<Voice, numVoices> voices;
        uint32_t counter = 0;
    };
}
