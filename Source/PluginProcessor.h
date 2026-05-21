#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "MultibandSaturator.h"

class SpectralSatAudioProcessor : public juce::AudioProcessor
{
public:
    SpectralSatAudioProcessor();
    ~SpectralSatAudioProcessor() override = default;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& data) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ── APVTS ─────────────────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Param IDs
    static juce::String gainParamID (int band) { return "gain" + juce::String (band); }
    static juce::String qParamID    (int band) { return "q"    + juce::String (band); }
    static const char*  oversampleParamID()     { return "oversample"; }

private:
    MultibandSaturator saturator;
    void syncParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralSatAudioProcessor)
};
