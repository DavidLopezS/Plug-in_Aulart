/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Loudness_Checker_PluginAudioProcessorEditor::Loudness_Checker_PluginAudioProcessorEditor (Loudness_Checker_PluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
	setOpaque(true);

	addAndMakeVisible(&mySpectrAnComp);
	addAndMakeVisible(&mySpectrRep);

    setSize (700, 500);
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

	const auto paddingX = 5;
	const auto paddingY = 35;
	const auto paddingY2 = 235;
	const auto width = 300;
	const auto height = 200;
	
	mySpectrRep.setBounds(paddingX, paddingY2, getWidth(), height);
	mySpectrAnComp.setBounds(paddingX, paddingY, getWidth(), height);

}

