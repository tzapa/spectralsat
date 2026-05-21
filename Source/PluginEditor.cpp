#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Colour palette
// ─────────────────────────────────────────────────────────────────────────────
namespace Pal
{
    const juce::Colour bg         { 0xFF1A1A1A };
    const juce::Colour panel      { 0xFF242424 };
    const juce::Colour divider    { 0xFF333333 };
    const juce::Colour textMain   { 0xFFE0E0E0 };
    const juce::Colour textDim    { 0xFF888888 };
}

// ─────────────────────────────────────────────────────────────────────────────
// Custom LookAndFeel for sliders and knobs
// ─────────────────────────────────────────────────────────────────────────────
class SpectralLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpectralLookAndFeel()
    {
        setColour (juce::Slider::trackColourId,          juce::Colours::transparentBlack);
        setColour (juce::Slider::backgroundColourId,     Pal::panel);
        setColour (juce::Slider::textBoxTextColourId,    Pal::textDim);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId,            Pal::textMain);
    }

    // Vertical slider
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float, float,
                           const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        auto accent = slider.findColour (juce::Slider::thumbColourId);

        // Track
        auto trackW = 4.0f;
        auto trackX = x + width * 0.5f - trackW * 0.5f;
        g.setColour (Pal::divider);
        g.fillRoundedRectangle (trackX, (float)y, trackW, (float)height, 2.0f);

        // Filled portion (from bottom up)
        auto fillH = (float)y + (float)height - sliderPos;
        g.setColour (accent.withAlpha (0.7f));
        g.fillRoundedRectangle (trackX, sliderPos, trackW, fillH, 2.0f);

        // Thumb
        auto thumbR = 8.0f;
        g.setColour (accent);
        g.fillEllipse (x + width * 0.5f - thumbR, sliderPos - thumbR, thumbR * 2, thumbR * 2);
        g.setColour (Pal::textMain);
        g.drawEllipse (x + width * 0.5f - thumbR, sliderPos - thumbR, thumbR * 2, thumbR * 2, 1.0f);
    }

    // Rotary knob
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto accent = slider.findColour (juce::Slider::thumbColourId);
        auto centre = juce::Point<float> (x + width * 0.5f, y + height * 0.5f);
        auto radius = juce::jmin (width, height) * 0.4f;

        // Background circle
        g.setColour (Pal::divider);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2, radius * 2);

        // Arc
        juce::Path arc;
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                           rotaryStartAngle, angle, true);
        g.setColour (accent);
        g.strokePath (arc, juce::PathStrokeType (3.0f));

        // Pointer line
        auto lineR = radius * 0.6f;
        g.setColour (Pal::textMain);
        g.drawLine (juce::Line<float> (centre,
                    centre.translated (std::sin (angle) * lineR, -std::cos (angle) * lineR)), 2.0f);
    }
};

static SpectralLookAndFeel gLnf;

// ─────────────────────────────────────────────────────────────────────────────
// BandStrip
// ─────────────────────────────────────────────────────────────────────────────

BandStrip::BandStrip (juce::AudioProcessorValueTreeState& apvts, int bandIndex,
                      const juce::String& label, juce::Colour colour)
    : bandLabel (label), bandColour (colour)
{
    setLookAndFeel (&gLnf);

    // Gain slider
    gainSlider.setRange (0.0, 12.0, 0.01);
    gainSlider.setTextValueSuffix (" dB");
    gainSlider.setColour (juce::Slider::thumbColourId, colour);
    addAndMakeVisible (gainSlider);

    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, SpectralSatAudioProcessor::gainParamID (bandIndex), gainSlider);

    // Q knob
    qKnob.setRange (0.1, 4.0, 0.01);
    qKnob.setSkewFactor (0.4);
    qKnob.setColour (juce::Slider::thumbColourId, colour);
    addAndMakeVisible (qKnob);

    qAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, SpectralSatAudioProcessor::qParamID (bandIndex), qKnob);

    // Label
    nameLabel.setText (label, juce::dontSendNotification);
    nameLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    nameLabel.setColour (juce::Label::textColourId, colour);
    nameLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (nameLabel);
}

void BandStrip::paint (juce::Graphics& g)
{
    g.setColour (Pal::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 6.0f);

    // Top accent bar
    g.setColour (bandColour.withAlpha (0.8f));
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    g.fillRoundedRectangle (b.removeFromTop (3.0f), 2.0f);
}

void BandStrip::resized()
{
    auto area = getLocalBounds().reduced (4);

    nameLabel .setBounds (area.removeFromTop    (22));
    area.removeFromTop (4);

    auto knobArea = area.removeFromBottom (80);
    qKnob     .setBounds (knobArea.reduced (8, 0));

    gainSlider.setBounds (area.reduced (0, 4));
}

// ─────────────────────────────────────────────────────────────────────────────
// Editor
// ─────────────────────────────────────────────────────────────────────────────

SpectralSatAudioProcessorEditor::SpectralSatAudioProcessorEditor (SpectralSatAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&gLnf);
    setSize (520, 380);

    addAndMakeVisible (stripLow);
    addAndMakeVisible (stripLowMid);
    addAndMakeVisible (stripHighMid);
    addAndMakeVisible (stripHigh);

    // Oversample toggle
    osButton.setColour (juce::ToggleButton::textColourId, Pal::textMain);
    osButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xFF80CBC4));
    addAndMakeVisible (osButton);
    osAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.apvts, SpectralSatAudioProcessor::oversampleParamID(), osButton);

    // Title
    titleLabel.setText ("SPECTRAL SAT", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, Pal::textMain);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);
}

void SpectralSatAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Pal::bg);

    // Subtle crossover freq labels at bottom
    g.setColour (Pal::textDim);
    g.setFont (10.0f);
    auto w = getWidth();
    // Divider lines between strips
    int stripW = (w - 20) / 4;
    for (int i = 1; i < 4; ++i)
    {
        float lx = 10 + stripW * i;
        g.setColour (Pal::divider);
        g.drawLine (lx, 40, lx, getHeight() - 10, 1.0f);
    }

    // Crossover labels
    juce::String labels[] { "300 Hz", "1 kHz", "7 kHz" };
    for (int i = 0; i < 3; ++i)
    {
        float lx = 10 + stripW * (i + 1);
        g.setColour (Pal::textDim);
        g.drawText (labels[i], (int)(lx - 25), getHeight() - 16, 50, 12,
                    juce::Justification::centred, false);
    }
}

void SpectralSatAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    // Header row
    auto header = area.removeFromTop (32);
    titleLabel.setBounds (header.removeFromLeft (180));
    header.removeFromLeft (20);
    osButton.setBounds (header.removeFromRight (100));

    area.removeFromTop (6);

    // 4 strips side by side
    int stripW = area.getWidth() / 4;
    stripLow    .setBounds (area.removeFromLeft (stripW).reduced (2, 0));
    stripLowMid .setBounds (area.removeFromLeft (stripW).reduced (2, 0));
    stripHighMid.setBounds (area.removeFromLeft (stripW).reduced (2, 0));
    stripHigh   .setBounds (area.reduced (2, 0));
}
