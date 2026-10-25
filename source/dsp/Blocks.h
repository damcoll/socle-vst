#pragma once

// Small DSP building blocks. Header-only, no allocation, safe to call from
// the audio thread.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace socle
{
    constexpr double pi = 3.141592653589793;

    // -60 dB reached after `seconds`
    inline float decayCoef (double seconds, double sampleRate)
    {
        return (float) std::exp (-6.907755278982137 / (seconds * sampleRate));
    }

    // maps 0..1 onto lo..hi on an exponential curve (frequencies, times)
    inline double expMap (float x, double lo, double hi)
    {
        return lo * std::pow (hi / lo, (double) x);
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

    struct Noise
    {
        explicit Noise (uint32_t seed = 0x9e3779b9u) : state (seed) {}

        float next()
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return (float) (int32_t) state * (1.0f / 2147483648.0f);
        }

        uint32_t state;
    };

    // Sawtooth or pulse without the aliasing of naive waveforms.
    // `width` is the pulse width of the square (0.5 = square).
    struct Oscillator
    {
        enum class Shape { saw, square };

        void setFreq (double freq, double sampleRate) { inc = std::min (freq / sampleRate, 0.45); }
        void reset (double startPhase = 0.0) { phase = startPhase; }

        float next (Shape shape, float width = 0.5f)
        {
            float v;
            if (shape == Shape::saw)
            {
                v = (float) (2.0 * phase - 1.0) - polyBlep (phase, inc);
            }
            else
            {
                const auto w = (double) std::clamp (width, 0.05f, 0.95f);
                v = phase < w ? 1.0f : -1.0f;
                v += polyBlep (phase, inc);
                v -= polyBlep (std::fmod (phase + 1.0 - w, 1.0), inc);
                v += (float) (1.0 - 2.0 * w); // keep the average at 0 whatever the width
            }
            phase += inc;
            if (phase >= 1.0)
                phase -= 1.0;
            return v;
        }

        double phase = 0.0, inc = 0.0;
    };

    // ADSR with analog-like exponential segments. Retriggering starts from the
    // current level, so a stolen or legato note never clicks.
    struct Adsr
    {
        void setTimes (double a, double d, float s, double r, double sampleRate)
        {
            // the attack aims above 1 so it reaches the top in `a` seconds with a convex curve
            attackCoef = (float) std::exp (-std::log ((attackTarget) / (attackTarget - 1.0)) / (a * sampleRate));
            decayCoefficient = decayCoef (d, sampleRate);
            releaseCoef = decayCoef (r, sampleRate);
            sustain = s;
        }

        void gateOn() { stage = Stage::attack; }
        void gateOff() { if (stage != Stage::idle) stage = Stage::release; }
        void kill() { stage = Stage::idle; value = 0.0f; }
        bool isIdle() const { return stage == Stage::idle; }
        bool isReleasing() const { return stage == Stage::release; }

        float next()
        {
            switch (stage)
            {
                case Stage::attack:
                    value = attackTarget + (value - attackTarget) * attackCoef;
                    if (value >= 1.0f)
                    {
                        value = 1.0f;
                        stage = Stage::decay;
                    }
                    break;
                case Stage::decay:
                    value = sustain + (value - sustain) * decayCoefficient;
                    break;
                case Stage::release:
                    value *= releaseCoef;
                    if (value < 1.0e-4f)
                        kill();
                    break;
                case Stage::idle:
                    break;
            }
            return value;
        }

        enum class Stage { idle, attack, decay, release };
        static constexpr float attackTarget = 1.3f;
        Stage stage = Stage::idle;
        float value = 0.0f, sustain = 1.0f;
        float attackCoef = 0.0f, decayCoefficient = 0.0f, releaseCoef = 0.0f;
    };

    // State-variable filter (TPT): low, band and high outputs at once
    struct Svf
    {
        void set (double freq, double q, double sampleRate)
        {
            const auto g = std::tan (pi * freq / sampleRate);
            k = 1.0 / q;
            a1 = 1.0 / (1.0 + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        void process (float input)
        {
            const auto v3 = input - ic2;
            const auto v1 = a1 * ic1 + a2 * v3;
            const auto v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0 * v1 - ic1;
            ic2 = 2.0 * v2 - ic2;
            low = (float) v2;
            band = (float) v1;
            high = (float) (input - k * v1 - v2);
        }

        void reset() { ic1 = ic2 = 0.0; low = band = high = 0.0f; }

        double k = 1.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
        double ic1 = 0.0, ic2 = 0.0;
        float low = 0.0f, band = 0.0f, high = 0.0f;
    };

    // removes the DC offset that saturation leaves (first-order high-pass ~10 Hz)
    struct DcBlocker
    {
        void prepare (double sampleRate) { r = (float) (1.0 - 2.0 * pi * 10.0 / sampleRate); x1 = y1 = 0.0f; }

        float process (float x)
        {
            const auto y = x - x1 + r * y1;
            x1 = x;
            y1 = y;
            return y;
        }

        float r = 0.999f, x1 = 0.0f, y1 = 0.0f;
    };

    // 4-pole (24 dB) transistor-ladder low-pass, zero-delay feedback.
    // The input and the resonance loop go through a tanh: pushed hard it
    // saturates instead of clipping, and self-oscillation stays bounded.
    // Runs at twice the sample rate to keep the resonance clean up high.
    struct Ladder
    {
        void prepare (double rate) { sampleRate = rate * 2.0; reset(); }
        void reset() { s[0] = s[1] = s[2] = s[3] = 0.0f; previous = 0.0f; }

        // cutoff in Hz, resonance 0..1, drive 0..1
        void set (double cutoff, float resonance, float drive)
        {
            const auto fc = std::clamp (cutoff, 20.0, sampleRate * 0.42);
            const auto g = std::tan (pi * fc / sampleRate);
            G = (float) (g / (1.0 + g));
            k = 4.1f * resonance;
            inputGain = 1.0f + 7.0f * drive * drive;
            // a Moog thins out when the resonance rises: give part of the bass back
            makeUp = (1.0f + 0.45f * k) / std::sqrt (inputGain);
        }

        float process (float in)
        {
            // 2x oversampling: the midpoint first, then the sample; average the two outputs
            const auto a = tick (0.5f * (previous + in));
            const auto b = tick (in);
            previous = in;
            return 0.5f * (a + b) * makeUp;
        }

    private:
        float tick (float in)
        {
            // what the 4 stages would output from their states alone (no input)
            const auto oneMinusG = 1.0f - G;
            const auto G2 = G * G, G3 = G2 * G, G4 = G3 * G;
            const auto S = G3 * s[0] * oneMinusG + G2 * s[1] * oneMinusG + G * s[2] * oneMinusG + s[3] * oneMinusG;

            // solve the feedback loop linearly, then saturate what enters the ladder
            const auto u = in * inputGain;
            const auto y4 = (G4 * u + S) / (1.0f + k * G4);
            auto x = std::tanh (u - k * y4);

            for (auto& state : s)
            {
                const auto v = (x - state) * G;
                const auto y = v + state;
                state = y + v;
                x = y;
            }
            return x;
        }

        double sampleRate = 88200.0;
        float s[4] {};
        float G = 0.0f, k = 0.0f, inputGain = 1.0f, makeUp = 1.0f, previous = 0.0f;
    };
}
