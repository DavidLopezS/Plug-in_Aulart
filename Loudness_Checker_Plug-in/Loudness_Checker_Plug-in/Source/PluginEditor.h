/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI\SpectrumAnalyzerComponent.h"
#include "UI\SpectrogramRepresentation.h"


//==============================================================================
/**
*/
class Loudness_Checker_PluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Loudness_Checker_PluginAudioProcessorEditor (Loudness_Checker_PluginAudioProcessor&);
    ~Loudness_Checker_PluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
	void mouseDown(const juce::MouseEvent&) override;
	void printRMS(juce::Grid);
	void printSpectr(juce::Grid);

	SpectrumAnalyzerComponent mySpectrAnComp;
	SpectrogramRepresentation mySpectrRep;

private:
	
	bool isClicked = true;

	juce::Slider mindBSlider;
	juce::Slider maxdBSlider;

	using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	std::unique_ptr<Attachment> mindBSliderAttachment;
	std::unique_ptr<Attachment> maxdBSliderAttachment;

    Loudness_Checker_PluginAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Loudness_Checker_PluginAudioProcessorEditor)
};
