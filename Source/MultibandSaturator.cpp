#include "MultibandSaturator.h"

// ─────────────────────────────────────────────────────────────────────────────
// BandSaturator
// ─────────────────────────────────────────────────────────────────────────────

void BandSaturator::prepare (const juce::dsp::ProcessSpec&) {}
void BandSaturator::reset() {}

float BandSaturator::processSample (float x, float gainLinear) noexcept
{
    if (gainLinear <= 0.001f) return x;

    // Drive the saturator harder as gain increases (up to ~6x)
    float drive = 1.0f + gainLinear * 5.0f;
    float wet   = saturate (x * drive) / drive;   // normalised output
    float delta = wet - x;                         // only the added harmonics
    return x + gainLinear * delta;
}

// ─────────────────────────────────────────────────────────────────────────────
// MultibandSaturator
// ─────────────────────────────────────────────────────────────────────────────

void MultibandSaturator::makeCoeffsForCrossover (LRCrossover& cr, float freq, float q, double sr)
{
    // Butterworth 2nd-order LP / HP coefficients
    // LR4 = two cascaded identical 2nd-order BW stages
    auto lpCoeffs = Coeffs::makeLowPass  (sr, static_cast<double>(freq), static_cast<double>(q));
    auto hpCoeffs = Coeffs::makeHighPass (sr, static_cast<double>(freq), static_cast<double>(q));

    for (int c = 0; c < 2; ++c)
    {
        *cr.lpA[c].coefficients = *lpCoeffs;
        *cr.lpB[c].coefficients = *lpCoeffs;
        *cr.hpA[c].coefficients = *hpCoeffs;
        *cr.hpB[c].coefficients = *hpCoeffs;
    }
}

void MultibandSaturator::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate  = spec.sampleRate;
    blockSize   = static_cast<int> (spec.maximumBlockSize);
    numChannels = static_cast<int> (spec.numChannels);

    // Prepare oversampler (4x, 2-stage)
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        spec.numChannels,
        2,   // 2^2 = 4x
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        false
    );
    oversampler->initProcessing (static_cast<size_t> (blockSize));
    lastOversample = oversample4x;

    // Prepare filters
    juce::dsp::ProcessSpec filterSpec { sampleRate, static_cast<juce::uint32>(blockSize), 2 };
    for (int c = 0; c < 2; ++c)
    {
        for (auto& cr : crossovers)
        {
            cr.lpA[c].prepare (filterSpec);
            cr.lpB[c].prepare (filterSpec);
            cr.hpA[c].prepare (filterSpec);
            cr.hpB[c].prepare (filterSpec);
        }
    }

    // Prepare band saturators
    for (auto& bandArr : bandSat)
        for (auto& sat : bandArr)
            sat.prepare (spec);

    updateFilters();
    reset();
}

void MultibandSaturator::reset()
{
    for (auto& cr : crossovers) cr.reset();
    if (oversampler) oversampler->reset();
}

void MultibandSaturator::updateFilters()
{
    float q0 = bandParams[kLow].q;
    float q1 = bandParams[kLowMid].q;
    float q2 = bandParams[kHighMid].q;

    makeCoeffsForCrossover (crossovers[0], kCross1, q0, sampleRate);
    makeCoeffsForCrossover (crossovers[1], kCross2, q1, sampleRate);
    makeCoeffsForCrossover (crossovers[2], kCross3, q2, sampleRate);
}

void MultibandSaturator::processBlock (juce::AudioBuffer<float>& buffer)
{
    // Rebuild oversampler if mode changed
    if (oversample4x != lastOversample)
    {
        lastOversample = oversample4x;
        oversampler->reset();
    }

    juce::dsp::AudioBlock<float> block (buffer);

    if (oversample4x)
    {
        auto osBlock = oversampler->processSamplesUp (block);
        processBlockInternal (osBlock);
        oversampler->processSamplesDown (block);
    }
    else
    {
        processBlockInternal (block);
    }
}

void MultibandSaturator::processBlockInternal (juce::dsp::AudioBlock<float>& block)
{
    // Pre-compute linear gains for each band
    float gains[kNumBands];
    for (int b = 0; b < kNumBands; ++b)
        gains[b] = juce::Decibels::decibelsToGain (bandParams[b].gainDB) - 1.0f;
    // gains[b] now represents 0..~3.98 (0 dB = 0, 12 dB ≈ 2.98)
    // We clamp to [0, 1] for the blend amount
    for (int b = 0; b < kNumBands; ++b)
        gains[b] = juce::jlimit (0.0f, 1.0f, gains[b] / 3.0f);

    const int numSamples  = static_cast<int> (block.getNumSamples());
    const int numCh       = static_cast<int> (block.getNumChannels());

    for (int ch = 0; ch < numCh && ch < 2; ++ch)
    {
        float* data = block.getChannelPointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float x = data[i];

            // ── Split into 4 bands via 3 LR crossovers ──────────────────────
            // Crossover 1 at 300 Hz → below = low, above continues
            float lo1 = crossovers[0].processLP (ch, x);
            float hi1 = crossovers[0].processHP (ch, x);

            // Crossover 2 at 1 kHz → split hi1
            float lo2 = crossovers[1].processLP (ch, hi1);
            float hi2 = crossovers[1].processHP (ch, hi1);

            // Crossover 3 at 7 kHz → split hi2
            float lo3 = crossovers[2].processLP (ch, hi2);
            float hi3 = crossovers[2].processHP (ch, hi2);

            // Band signals
            float bands[kNumBands] = { lo1, lo2, lo3, hi3 };

            // ── Saturate each band and mix back ─────────────────────────────
            float out = 0.0f;
            for (int b = 0; b < kNumBands; ++b)
                out += bandSat[b][ch].processSample (bands[b], gains[b]);

            data[i] = out;
        }
    }
}
