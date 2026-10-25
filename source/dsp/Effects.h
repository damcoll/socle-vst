#pragma once

// The EFFETS section, in this order: SATURATION -> CHORUS -> DELAY.
// The synth is mono until the chorus; the chorus and the delay spread it in
// stereo but leave the low end in the middle, where a bass belongs.

#include "Blocks.h"

#include <vector>

namespace socle
{
    // Tube-like saturation: asymmetric, so it adds even harmonics (warmth)
    // before odd ones (grit). 0 = untouched.
    struct Saturation
    {
        // 0..1, once per block
        void setAmount (float amount)
        {
            wet = std::min (1.0f, amount * 4.0f);
            gain = 1.0f + 11.0f * amount * amount;
            bias = 0.25f * amount;
            biasOffset = std::tanh (bias);
            // a mid-level signal (0.5) comes out at the same level: it gets denser, not louder
            makeUp = 0.5f / (std::tanh (0.5f * gain + bias) - biasOffset);
        }

        float process (float x)
        {
            if (wet <= 0.0f)
                return x;
            const auto shaped = (std::tanh (x * gain + bias) - biasOffset) * makeUp;
            return x + (shaped - x) * wet;
        }

        float wet = 0.0f, gain = 1.0f, bias = 0.0f, biasOffset = 0.0f, makeUp = 1.0f;
    };

    // Fractional delay line with linear interpolation
    struct DelayLine
    {
        void prepare (int maxSamples)
        {
            buffer.assign ((size_t) maxSamples + 2, 0.0f);
            write = 0;
        }

        void clear() { std::fill (buffer.begin(), buffer.end(), 0.0f); }

        void push (float x)
        {
            buffer[(size_t) write] = x;
            if (++write == (int) buffer.size())
                write = 0;
        }

        // `delay` samples behind the last push (>= 1)
        float read (float delay) const
        {
            const auto size = (int) buffer.size();
            auto pos = (float) write - 1.0f - delay;
            while (pos < 0.0f)
                pos += (float) size;
            const auto i = (int) pos;
            const auto frac = pos - (float) i;
            const auto a = buffer[(size_t) i];
            const auto b = buffer[(size_t) ((i + 1) % size)];
            return a + (b - a) * frac;
        }

        std::vector<float> buffer;
        int write = 0;
    };

    // Juno-style chorus: one delay line swept by a slow triangle, added to the
    // left and subtracted from the right (wide, and mono-safe: the sum is the
    // dry sound). Only what is above ~150 Hz is chorused, the bass stays centred.
    struct Chorus
    {
        void prepare (double rate)
        {
            sampleRate = rate;
            line.prepare ((int) (0.03 * rate));
            lowCut.set (150.0, 0.707, rate);
            lowCut.reset();
        }

        // 0..1, once per block: how much chorus, a little deeper as it goes
        void setAmount (float amount)
        {
            mix = amount;
            depth = (float) ((0.0015 + 0.0025 * amount) * sampleRate);
        }

        void process (float in, float& left, float& right)
        {
            line.push (in);
            phase += 0.5 / sampleRate; // 0.5 Hz, like the Juno's chorus I
            if (phase >= 1.0)
                phase -= 1.0;
            if (mix <= 0.0f)
            {
                left = right = in;
                return;
            }

            const auto tri = (float) (4.0 * std::abs (phase - 0.5) - 1.0);
            lowCut.process (line.read ((float) (0.007 * sampleRate) + depth * tri));
            const auto wet = lowCut.high * 0.8f * mix;
            left = in + wet;
            right = in - wet;
        }

        DelayLine line;
        Svf lowCut;
        double sampleRate = 48000.0, phase = 0.0;
        float mix = 0.0f, depth = 0.0f;
    };

    // Tempo-synced ping-pong delay: the repeats bounce left / right and get
    // darker and thinner each time, like a tape echo. DELAY = how loud the
    // echoes are, RETOUR = how many.
    struct Delay
    {
        static constexpr double maxSeconds = 2.5;

        void prepare (double rate)
        {
            sampleRate = rate;
            left.prepare ((int) (maxSeconds * rate));
            right.prepare ((int) (maxSeconds * rate));
            tone.set (3000.0, 0.6, rate);
            thin.set (120.0, 0.6, rate);
            tone.reset();
            thin.reset();
            smoothedTime = -1.0f;
            // the delay time follows a tempo change in ~50 ms instead of jumping (no click)
            timeSmoothing = 1.0f - std::exp (-1.0f / (0.05f * (float) rate));
        }

        // once per block
        void set (float mixAmount, float retour, double seconds)
        {
            mix = mixAmount;
            feedback = 0.9f * retour;
            targetTime = (float) (std::clamp (seconds, 0.01, maxSeconds - 0.01) * sampleRate);
            if (smoothedTime < 0.0f)
                smoothedTime = targetTime;
        }

        void process (float& l, float& r)
        {
            smoothedTime += (targetTime - smoothedTime) * timeSmoothing;
            const auto echoL = left.read (smoothedTime);
            const auto echoR = right.read (smoothedTime);

            // what goes back in: darker (3 kHz), thinner (120 Hz) and softly saturated
            tone.process (echoR);
            thin.process (tone.low);
            const auto back = std::tanh (thin.high * feedback);

            // ping-pong: the input enters on the left, the left echo feeds the right
            left.push ((l + r) * 0.5f + back);
            right.push (echoL);

            l += echoL * mix;
            r += echoR * mix;
        }

        void clear()
        {
            left.clear();
            right.clear();
        }

        DelayLine left, right;
        Svf tone, thin;
        double sampleRate = 48000.0;
        float mix = 0.0f, feedback = 0.0f, targetTime = 0.0f, smoothedTime = -1.0f, timeSmoothing = 0.001f;
    };

    // delay divisions, in quarter notes
    inline constexpr double delayDivisions[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
}
