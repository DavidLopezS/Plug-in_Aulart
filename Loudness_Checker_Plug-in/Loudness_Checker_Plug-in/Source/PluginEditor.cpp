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
    : AudioProcessorEditor (&p), audioProcessor (p), forwardFFT(audioProcessor.fftOrder), window(audioProcessor.fftSize, juce::dsp::WindowingFunction<float>::hann)
{
	setOpaque(true);
	//setAudioChannels(2, 0);  // we want a couple of input channels but no outputs
	startTimerHz(30);
    setSize (700, 500);

	/*addAndMakeVisible(spectrumAnalisisRep);*/
}

Loudness_Checker_PluginAudioProcessorEditor::~Loudness_Checker_PluginAudioProcessorEditor()
{
}

//==============================================================================
void Loudness_Checker_PluginAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);

	g.setOpacity(1.0f);
	g.setColour(juce::Colours::white);
	drawFrame(g);

	/*spectrumAnalisisRep.drawFrame(g);
	spectrumAnalisisRep.timerCallback();*/
}

void Loudness_Checker_PluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
	const auto paddingX = 5;
	const auto paddingY = 35;
	const auto paddingY2 = 235;
	const auto width = 300;
	const auto height = 200;

}

void Loudness_Checker_PluginAudioProcessorEditor::timerCallback()
{
	if (audioProcessor.nextFFTBlockReady)
	{
		drawNextFrameOfSpectrum();
		audioProcessor.nextFFTBlockReady = false;
		repaint();
	}
}

void Loudness_Checker_PluginAudioProcessorEditor::drawNextFrameOfSpectrum()
{

	window.multiplyWithWindowingTable(audioProcessor.fftData, audioProcessor.fftSize);

	forwardFFT.performFrequencyOnlyForwardTransform(audioProcessor.fftData);

	auto mindB = -100.0f;
	auto maxdB = 0.0f;

	for (int i = 0; i < audioProcessor.scopeSize; ++i)
	{
		auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)audioProcessor.scopeSize) * 0.2f);
		auto fftDataIndex = juce::jlimit(0, audioProcessor.fftSize / 2, (int)(skewedProportionX * (float)audioProcessor.fftSize * 0.5f));
		auto level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(audioProcessor.fftData[fftDataIndex]) - juce::Decibels::gainToDecibels((float)audioProcessor.fftSize)),
			mindB, maxdB, 0.0f, 1.0f);

		audioProcessor.scopeData[i] = level;
	}

}

void Loudness_Checker_PluginAudioProcessorEditor::drawFrame(juce::Graphics& g)
{
	for (int i = 1; i < audioProcessor.scopeSize; ++i)
	{
		auto width = getLocalBounds().getWidth();
		auto height = getLocalBounds().getHeight();

		g.drawLine({ (float)juce::jmap(i - 1, 0, audioProcessor.scopeSize - 1, 0, width),
						   juce::jmap(audioProcessor.scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
					(float)juce::jmap(i, 0, audioProcessor.scopeSize - 1, 0, width),
						   juce::jmap(audioProcessor.scopeData[i], 0.0f, 1.0f, (float)height, 0.0f) });
	}
}
