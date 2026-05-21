#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
// One band strip: vertical slider (gain) + rotary knob (Q) + label
// ─────────────────────────────────────────────────────────────────────────────

class BandStrip : public juce::Component
{
public:
    BandStrip (juce::AudioProcessorValueTreeState& apvts, int bandIndex,
               const juce::String& label, juce::Colour colour);

    void resized() override;
    void paint   (juce::Graphics& g) override;

private:
    juce::String  bandLabel;
    juce::Colour  bandColour;

    juce::Slider  gainSlider { juce::Slider::LinearVertical,   juce::Slider::TextBoxBelow };
    juce::Slider  qKnob      { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Label   nameLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> qAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandStrip)
};

// ─────────────────────────────────────────────────────────────────────────────

class SpectralSatAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpectralSatAudioProcessorEditor (SpectralSatAudioProcessor&);
    ~SpectralSatAudioProcessorEditor() override = default;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    SpectralSatAudioProcessor& processor;

    // 4 band strips
    BandStrip stripLow     { processor.apvts, 0, "LOW",      juce::Colour (0xFF4DB6AC) };
    BandStrip stripLowMid  { processor.apvts, 1, "LOW MID",  juce::Colour (0xFF81C784) };
    BandStrip stripHighMid { processor.apvts, 2, "HIGH MID", juce::Colour (0xFFFFB74D) };
    BandStrip stripHigh    { processor.apvts, 3, "HIGH",     juce::Colour (0xFFE57373) };

    juce::ToggleButton osButton { "4x" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> osAttach;

    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralSatAudioProcessorEditor)
};
