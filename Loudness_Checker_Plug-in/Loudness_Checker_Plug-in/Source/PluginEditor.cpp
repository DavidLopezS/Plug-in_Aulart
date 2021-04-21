/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Loudness_Checker_PluginAudioProcessorEditor::Loudness_Checker_PluginAudioProcessorEditor (Loudness_Checker_PluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

Loudness_Checker_PluginAudioProcessorEditor::~Loudness_Checker_PluginAudioProcessorEditor()
{
}

//==============================================================================
void Loudness_Checker_PluginAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);
}

void Loudness_Checker_PluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
