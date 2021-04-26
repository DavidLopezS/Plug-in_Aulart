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

	SpectrumAnalyzerComponent mySpectrAnComp;
	SpectrogramRepresentation mySpectrRep;

private:

    Loudness_Checker_PluginAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Loudness_Checker_PluginAudioProcessorEditor)
};
