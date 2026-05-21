#include "PluginProcessor.h"
#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Parameter layout
// ─────────────────────────────────────────────────────────────────────────────

static const juce::StringArray kBandNames { "Low", "LowMid", "HighMid", "High" };

juce::AudioProcessorValueTreeState::ParameterLayout
SpectralSatAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int b = 0; b < 4; ++b)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            gainParamID (b),
            kBandNames[b] + " Gain",
            juce::NormalisableRange<float> (0.0f, 12.0f, 0.01f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")
        ));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            qParamID (b),
            kBandNames[b] + " Q",
            juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f, 0.4f), // skewed
            0.707f
        ));
    }

    layout.add (std::make_unique<juce::AudioParameterBool> (
        oversampleParamID(), "4x Oversample", false
    ));

    return layout;
}

// ─────────────────────────────────────────────────────────────────────────────

SpectralSatAudioProcessor::SpectralSatAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout())
{
}

void SpectralSatAudioProcessor::prepareToPlay (double sr, int blockSize)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = static_cast<juce::uint32> (blockSize);
    spec.numChannels      = static_cast<juce::uint32> (getTotalNumInputChannels());

    saturator.prepare (spec);
}

void SpectralSatAudioProcessor::releaseResources() {}

void SpectralSatAudioProcessor::syncParams()
{
    for (int b = 0; b < 4; ++b)
    {
        saturator.bandParams[b].gainDB = apvts.getRawParameterValue (gainParamID (b))->load();
        saturator.bandParams[b].q      = apvts.getRawParameterValue (qParamID    (b))->load();
    }
    saturator.oversample4x = apvts.getRawParameterValue (oversampleParamID())->load() > 0.5f;
    saturator.updateFilters();
}

void SpectralSatAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    syncParams();
    saturator.processBlock (buffer);
}

// ─────────────────────────────────────────────────────────────────────────────
// State
// ─────────────────────────────────────────────────────────────────────────────

void SpectralSatAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, dest);
}

void SpectralSatAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralSatAudioProcessor();
}

juce::AudioProcessorEditor* SpectralSatAudioProcessor::createEditor()
{
    return new SpectralSatAudioProcessorEditor (*this);
}
