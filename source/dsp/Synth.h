#pragma once

// The 8 voices and who plays which note.
// One voice: OSC 1 + OSC 2 + SUB + BRUIT -> ladder filter -> amp, each with its ADSR.
// POLY spreads the notes over the 8 voices; MONO and LEGATO play one voice,
// last note wins, and going back to a note still held glides to it.

#include "Blocks.h"

#include <array>

namespace socle
{
    constexpr int numVoices = 8;

    enum class Mode { poly, mono, legato };
    enum class LfoDestination { hauteur, filtre, largeur };

    // the front panel's values, read once per block
    struct SynthParams
    {
        Oscillator::Shape shape1 = Oscillator::Shape::saw, shape2 = Oscillator::Shape::square;
        float accord2 = 0.0f, desaccord2 = 7.0f; // semitones, cents
        float niveau1 = 0.8f, niveau2 = 0.6f, niveauSub = 0.7f, niveauBruit = 0.0f;
        float coupure = 1200.0f, resonance = 0.3f, drive = 0.4f, envFiltre = 0.5f, velocite = 0.4f;
        float fA = 0.002f, fD = 0.4f, fS = 0.2f, fR = 0.3f;
        float aA = 0.002f, aD = 0.6f, aS = 0.8f, aR = 0.2f;
        Mode mode = Mode::poly;
        float glide = 0.0f; // seconds
        float lfoVitesse = 2.0f, lfoQuantite = 0.0f;
        LfoDestination lfoDestination = LfoDestination::filtre;
        float derive = 0.2f;
    };

    // what changes every sample and is shared by all voices
    struct Modulation
    {
        float cutoffOctaves = 0.0f; // COUPURE + LFO + mod wheel, in octaves above 20 Hz
        float pitchOffset = 0.0f;   // pitch bend + LFO, in semitones
        float pulseWidth = 0.5f;
    };

    struct Voice
    {
        void prepare (double rate)
        {
            sampleRate = rate;
            // DÉRIVE: the slow wander settles over ~1.5 s and picks a new direction every 1 to 3 s
            wanderCoef = 1.0f - decayCoef (1.5, rate);
            filter.prepare (rate);
            ampEnv.kill();
            filterEnv.kill();
            note = -1;
        }

        // fromPitch < 0: no glide, the note starts on its own pitch
        void start (int midiNote, float vel, uint32_t order, bool retrigger, float fromPitch)
        {
            const auto wasSilent = ampEnv.isIdle();
            note = midiNote;
            velocity = vel;
            age = order;
            targetPitch = (float) midiNote;
            pitch = fromPitch < 0.0f ? targetPitch : fromPitch;
            // DÉRIVE: each note is a little out of tune and a little brighter or darker
            noteDetune = drift.next();
            noteDetune2 = drift.next();
            noteCutoff = drift.next();
            if (wasSilent)
            {
                // a fresh note starts from the same phase every time: the attack punches the same way;
                // with DÉRIVE the phases scatter, like free-running analog oscillators
                const auto scatter = (double) phaseScatter;
                osc1.reset (scatter * (0.5 + 0.5 * drift.next()));
                osc2.reset (scatter * (0.5 + 0.5 * drift.next()));
                sub.reset();
                filter.reset();
            }
            if (retrigger || wasSilent)
            {
                ampEnv.gateOn();
                filterEnv.gateOn();
            }
        }

        // legato: change the pitch (with glide) without restarting the envelopes
        void slideTo (int midiNote) { note = midiNote; targetPitch = (float) midiNote; }

        void stop()
        {
            ampEnv.gateOff();
            filterEnv.gateOff();
        }

        bool isActive() const { return ! ampEnv.isIdle(); }

        // once per block: envelope times, interval of OSC 2, glide speed
        void update (const SynthParams& p)
        {
            ampEnv.setTimes (p.aA, p.aD, p.aS, p.aR, sampleRate);
            filterEnv.setTimes (p.fA, p.fD, p.fS, p.fR, sampleRate);
            osc2Ratio = std::pow (2.0, (p.accord2 + p.desaccord2 / 100.0) / 12.0);
            phaseScatter = p.derive;
            // GLIDE = time to get (99 %) to the new note
            glideCoef = p.glide < 0.001f ? 1.0f : 1.0f - std::exp (-4.6f / (p.glide * (float) sampleRate));
        }

        float next (const SynthParams& p, const Modulation& mod)
        {
            pitch += (targetPitch - pitch) * glideCoef;

            // DÉRIVE at full: the voice wanders +/-15 cents, each note +/-7, OSC 2 +/-5 on its own
            if (--wanderCountdown <= 0)
            {
                wanderTarget = drift.next();
                wanderCountdown = (int) (sampleRate * (2.0 + drift.next()));
            }
            wander += (wanderTarget - wander) * wanderCoef;
            const auto cents = p.derive * (15.0f * wander + 7.0f * noteDetune);

            const auto f1 = noteToHz ((double) (pitch + mod.pitchOffset + cents / 100.0f));
            osc1.setFreq (f1, sampleRate);
            osc2.setFreq (f1 * osc2Ratio * (1.0 + p.derive * 0.0029 * noteDetune2), sampleRate);
            sub.setFreq (f1 * 0.5, sampleRate);

            const auto mix = osc1.next (p.shape1, mod.pulseWidth) * p.niveau1
                           + osc2.next (p.shape2, mod.pulseWidth) * p.niveau2
                           + sub.next (Oscillator::Shape::square) * p.niveauSub
                           + noise.next() * p.niveauBruit;

            // ENV opens up to 7 octaves; the cutoff follows the keyboard by half;
            // VÉLOCITÉ opens the filter on hard notes and softens the quiet ones
            const auto fEnv = filterEnv.next();
            const auto octaves = mod.cutoffOctaves
                               + 7.0f * p.envFiltre * fEnv
                               + 0.5f * (pitch - 48.0f) / 12.0f
                               + 3.0f * p.velocite * (velocity - 0.5f)
                               + 0.3f * p.derive * noteCutoff;
            filter.set (20.0 * std::exp2 ((double) octaves), p.resonance, p.drive);

            const auto level = 1.0f - p.velocite * (1.0f - velocity) * 0.7f;
            const auto out = filter.process (mix * 0.35f) * ampEnv.next() * level;
            if (ampEnv.isIdle())
                note = -1;
            return out;
        }

        Oscillator osc1, osc2, sub;
        Noise noise, drift { 0x5eed1u };
        Ladder filter;
        Adsr ampEnv, filterEnv;
        double sampleRate = 44100.0, osc2Ratio = 1.0;
        int note = -1;
        float velocity = 1.0f, pitch = 0.0f, targetPitch = 0.0f, glideCoef = 1.0f;
        float wander = 0.0f, wanderTarget = 0.0f, wanderCoef = 0.0f, phaseScatter = 0.0f;
        float noteDetune = 0.0f, noteDetune2 = 0.0f, noteCutoff = 0.0f;
        int wanderCountdown = 0;
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
                v.drift.state = seed ^ 0xa5a5a5a5u;
                v.update (params);
            }
            heldCount = 0;
            smoothedCutoff = cutoffOctaves (params.coupure);
            // ~5 ms to follow a turned knob, the pitch bend or the mod wheel without zipper noise
            smoothing = 1.0f - std::exp (-1.0f / (0.005f * (float) rate));
        }

        void setParams (const SynthParams& p)
        {
            // switching between POLY and MONO / LEGATO: release everything, nothing stays stuck
            if (p.mode != params.mode)
                allNotesOff();
            params = p;
            for (auto& v : voices)
                v.update (params);
        }

        void noteOn (int note, float velocity)
        {
            const auto glideFrom = params.glide > 0.0f && lastPitch >= 0.0f ? lastPitch : -1.0f;

            if (params.mode == Mode::poly)
            {
                auto& v = pickVoice (note);
                v.start (note, velocity, ++counter, true, glideFrom);
            }
            else
            {
                const auto legato = heldCount > 0;
                hold (note);
                auto& v = voices[0];
                if (params.mode == Mode::mono)
                    v.start (note, velocity, ++counter, true, v.isActive() ? v.pitch : glideFrom);
                else if (legato) // LEGATO: a note joined to the previous one slides without a new attack
                    v.slideTo (note);
                else
                    v.start (note, velocity, ++counter, true, -1.0f);
            }
            lastPitch = (float) note;
        }

        void noteOff (int note)
        {
            if (params.mode == Mode::poly)
            {
                for (auto& v : voices)
                    if (v.note == note)
                        v.stop();
                return;
            }

            const auto wasPlaying = heldCount > 0 && held[(size_t) heldCount - 1] == note;
            release (note);
            if (! wasPlaying)
                return;

            if (heldCount > 0)
            {
                // back to the note still held under the finger
                const auto previous = held[(size_t) heldCount - 1];
                voices[0].slideTo (previous);
                lastPitch = (float) previous;
            }
            else
            {
                voices[0].stop();
            }
        }

        void allNotesOff()
        {
            heldCount = 0;
            for (auto& v : voices)
                v.stop();
        }

        void setPitchBend (float semitones) { bendTarget = semitones; }
        void setModWheel (float amount) { wheelTarget = amount; }

        float next()
        {
            smoothedCutoff += (cutoffOctaves (params.coupure) - smoothedCutoff) * smoothing;
            bend += (bendTarget - bend) * smoothing;
            wheel += (wheelTarget - wheel) * smoothing;

            // LFO: one triangle shared by all voices, -1..1
            lfoPhase += params.lfoVitesse / sampleRate;
            if (lfoPhase >= 1.0)
                lfoPhase -= 1.0;
            lfoValue = (float) (4.0 * std::abs (lfoPhase - 0.5) - 1.0);
            const auto depth = params.lfoQuantite * lfoValue;

            // the mod wheel opens the filter by up to 4 octaves
            Modulation mod;
            mod.cutoffOctaves = smoothedCutoff + 4.0f * wheel;
            mod.pitchOffset = bend;
            switch (params.lfoDestination)
            {
                // squared: small amounts stay a vibrato, the full turn swings an octave
                case LfoDestination::hauteur: mod.pitchOffset += 12.0f * params.lfoQuantite * depth; break;
                case LfoDestination::filtre: mod.cutoffOctaves += 3.0f * depth; break;
                case LfoDestination::largeur: mod.pulseWidth += 0.45f * depth; break;
            }

            float mix = 0.0f;
            for (auto& v : voices)
                if (v.isActive())
                    mix += v.next (params, mod);
            return mix;
        }

        static float cutoffOctaves (float hz) { return std::log2 (hz / 20.0f); }

        std::array<Voice, numVoices> voices;
        SynthParams params;
        float lfoValue = 0.0f; // for the LFO light on the front panel

    private:
        // POLY: the same note again retriggers its voice; else a free voice; else the oldest is stolen
        Voice& pickVoice (int note)
        {
            for (auto& v : voices)
                if (v.note == note)
                    return v;

            for (auto& v : voices)
                if (! v.isActive())
                    return v;

            auto* oldest = &voices[0];
            for (auto& v : voices)
                if (v.age < oldest->age)
                    oldest = &v;
            return *oldest;
        }

        // the keys held in MONO / LEGATO, most recent last
        void hold (int note)
        {
            release (note);
            if (heldCount == (int) held.size())
                release (held[0]);
            held[(size_t) heldCount++] = note;
        }

        void release (int note)
        {
            int kept = 0;
            for (int i = 0; i < heldCount; ++i)
                if (held[(size_t) i] != note)
                    held[(size_t) kept++] = held[(size_t) i];
            heldCount = kept;
        }

        std::array<int, 16> held {};
        int heldCount = 0;
        double sampleRate = 44100.0, lfoPhase = 0.0;
        float smoothedCutoff = 0.0f, smoothing = 0.01f;
        float bend = 0.0f, bendTarget = 0.0f, wheel = 0.0f, wheelTarget = 0.0f;
        float lastPitch = -1.0f;
        uint32_t counter = 0;
    };
}
