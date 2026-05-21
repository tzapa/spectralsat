#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

// ---------------------------------------------------------------------------
// One saturated band
//   - Linkwitz–Riley crossover filters split the signal into 4 bands
//   - Each band is saturated with a soft-knee tape/tube algo
//   - Only the *delta* (harmonics) is mixed back: dry + gain * delta
// ---------------------------------------------------------------------------

class BandSaturator
{
public:
    struct Params
    {
        float gainDB  = 0.0f;   // 0 .. 12 dB  (how much harmonic to add)
        float q       = 0.7f;   // 0.1 .. 4.0  (crossover Q / resonance)
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    // Processes one sample; returns the saturated band sample
    // Call after the crossover filters have been applied.
    float processSample (float x, float gainLinear) noexcept;

private:
    // Tape-style saturation: soft-knee atan shaper
    static float saturate (float x) noexcept
    {
        // atan saturator normalised to unity gain at low levels
        // f(x) = (2/pi) * atan(pi/2 * x)
        constexpr float piOver2 = 1.5707963267948966f;
        return (2.0f / juce::MathConstants<float>::pi) * std::atan(piOver2 * x);
    }
};

// ---------------------------------------------------------------------------

class MultibandSaturator
{
public:
    // Band indices
    enum Band { kLow = 0, kLowMid, kHighMid, kHigh, kNumBands };

    // Crossover frequencies (Hz) – defaults match spec
    static constexpr float kCross1 = 300.0f;
    static constexpr float kCross2 = 1000.0f;
    static constexpr float kCross3 = 7000.0f;

    // Per-band user params
    BandSaturator::Params bandParams[kNumBands];

    // Oversampling: 1x or 4x
    bool oversample4x = false;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void processBlock (juce::AudioBuffer<float>& buffer);

    // Call when any param changes (Q or crossover freqs)
    void updateFilters();

private:
    double sampleRate  = 44100.0;
    int    blockSize   = 512;
    int    numChannels = 2;

    // ── Oversampling ─────────────────────────────────────────────────────────
    // We use JUCE's built-in oversampler (2-stage, 4x)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    bool lastOversample = false;

    // ── Linkwitz–Riley filters (LR4 = 2x BQ cascaded) ────────────────────────
    // We need 3 crossover points × 2 channels × LP+HP × 2 cascaded BQ
    // Layout: [crossoverIdx][channelIdx][lpOrHp][cascade0or1]
    using BiquadFilter = juce::dsp::IIR::Filter<float>;
    using Coeffs       = juce::dsp::IIR::Coefficients<float>;

    // LR4 LP: two cascaded 2nd-order Butterworth LP
    // LR4 HP: two cascaded 2nd-order Butterworth HP
    struct LRCrossover
    {
        // per channel, 2 cascaded biquads each for LP and HP
        std::array<BiquadFilter, 2> lpA, lpB; // lpA[ch], lpB[ch]
        std::array<BiquadFilter, 2> hpA, hpB;

        float processLP (int ch, float x) noexcept
        {
            return lpB[ch].processSample (lpA[ch].processSample (x));
        }
        float processHP (int ch, float x) noexcept
        {
            return hpB[ch].processSample (hpA[ch].processSample (x));
        }
        void reset()
        {
            for (int c = 0; c < 2; ++c)
            {
                lpA[c].reset(); lpB[c].reset();
                hpA[c].reset(); hpB[c].reset();
            }
        }
    };

    std::array<LRCrossover, 3> crossovers; // cross1, cross2, cross3

    // ── Band saturators (one per band per channel) ────────────────────────────
    // [band][channel]
    std::array<std::array<BandSaturator, 2>, kNumBands> bandSat;

    void makeCoeffsForCrossover (LRCrossover& cr, float freq, float q, double sr);
    void processBlockInternal   (juce::dsp::AudioBlock<float>& block);
};
