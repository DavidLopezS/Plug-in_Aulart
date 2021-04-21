/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrumAnalyzer.h"

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

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Loudness_Checker_PluginAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Loudness_Checker_PluginAudioProcessorEditor)
};
