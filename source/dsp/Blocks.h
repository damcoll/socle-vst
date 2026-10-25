#pragma once

// Small DSP building blocks. Header-only, no allocation, safe to call from
// the audio thread.

#include <cmath>
#include <cstdint>

namespace socle
{
    // -60 dB reached after `seconds`
    inline float decayCoef (double seconds, double sampleRate)
    {
        return (float) std::exp (-6.907755278982137 / (seconds * sampleRate));
    }

    inline double noteToHz (double note) { return 440.0 * std::pow (2.0, (note - 69.0) / 12.0); }

    // band-limited step correction (polyBLEP) around a discontinuity at phase 0
    inline float polyBlep (double t, double inc)
    {
        if (t < inc)
        {
            t /= inc;
            return (float) (t + t - t * t - 1.0);
        }
        if (t > 1.0 - inc)
        {
            t = (t - 1.0) / inc;
            return (float) (t * t + t + t + 1.0);
        }
        return 0.0f;
    }

    // Sawtooth without the aliasing of a naive ramp
    struct Saw
    {
        void setFreq (double freq, double sampleRate) { inc = freq / sampleRate; }
        void reset (double startPhase = 0.0) { phase = startPhase; }

        float next()
        {
            const auto v = (float) (2.0 * phase - 1.0) - polyBlep (phase, inc);
            phase += inc;
            if (phase >= 1.0)
                phase -= 1.0;
            return v;
        }

        double phase = 0.0, inc = 0.0;
    };

    // Envelope that follows a gate: rises while held, falls once released
    struct GateEnvelope
    {
        void setTimes (double attackSeconds, double releaseSeconds, double sampleRate)
        {
            attack = decayCoef (attackSeconds, sampleRate);
            release = decayCoef (releaseSeconds, sampleRate);
        }

        void open (float level) { target = level; }
        void close() { target = 0.0f; }
        bool isSilent() const { return target == 0.0f && value < 1.0e-4f; }

        float next()
        {
            const auto coef = target > value ? attack : release;
            value = target + (value - target) * coef;
            return value;
        }

        float value = 0.0f, target = 0.0f, attack = 0.0f, release = 0.0f;
    };
}
