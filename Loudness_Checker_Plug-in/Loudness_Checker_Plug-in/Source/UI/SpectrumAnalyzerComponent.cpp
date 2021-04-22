/*
  ==============================================================================

    SpectrumAnalyzerComponent.cpp
    Created: 22 Apr 2021 1:10:11pm
    Author:  david

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpectrumAnalyzerComponent.h"
//#include "..\Data\SpectrumAnalyzerData.h"

//==============================================================================
SpectrumAnalyzerComponent::SpectrumAnalyzerComponent() : forwardFFT(dataAnalyzer.fftOrder), window(dataAnalyzer.fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.

}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
}

void SpectrumAnalyzerComponent::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);

	g.setOpacity(1.0f);
	g.setColour(juce::Colours::white);
	drawFrame(g);
}

void SpectrumAnalyzerComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

}

void SpectrumAnalyzerComponent::timerCallback()
{
	if (dataAnalyzer.nextFFTBlockReady)
	{
		drawNextFrameOfSpectrum();
		dataAnalyzer.nextFFTBlockReady = false;
		repaint();
	}
}

void SpectrumAnalyzerComponent::drawNextFrameOfSpectrum()
{

	window.multiplyWithWindowingTable(dataAnalyzer.fftData, dataAnalyzer.fftSize);

	forwardFFT.performFrequencyOnlyForwardTransform(dataAnalyzer.fftData);

	auto mindB = -100.0f;
	auto maxdB = 0.0f;

	for (int i = 0; i < dataAnalyzer.scopeSize; ++i)
	{
		auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)dataAnalyzer.scopeSize) * 0.2f);
		auto fftDataIndex = juce::jlimit(0, dataAnalyzer.fftSize / 2, (int)(skewedProportionX * (float)dataAnalyzer.fftSize * 0.5f));
		auto level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(dataAnalyzer.fftData[fftDataIndex]) - juce::Decibels::gainToDecibels((float)dataAnalyzer.fftSize)),
			mindB, maxdB, 0.0f, 1.0f);

		scopeData[i] = level;
	}

}

void SpectrumAnalyzerComponent::drawFrame(juce::Graphics& g)
{
	for (int i = 1; i < dataAnalyzer.scopeSize; ++i)
	{
		auto width = getLocalBounds().getWidth();
		auto height = getLocalBounds().getHeight();

		g.drawLine({ (float)juce::jmap(i - 1, 0, dataAnalyzer.scopeSize - 1, 0, width),
						   juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
					(float)juce::jmap(i, 0, dataAnalyzer.scopeSize - 1, 0, width),
						   juce::jmap(scopeData[i], 0.0f, 1.0f, (float)height, 0.0f) });
	}
}
