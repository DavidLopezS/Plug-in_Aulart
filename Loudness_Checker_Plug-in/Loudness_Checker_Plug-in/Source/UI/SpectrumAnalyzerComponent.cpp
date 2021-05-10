/*
  ==============================================================================

    SpectrumAnalyzerComponent.cpp
    Created: 22 Apr 2021 1:10:11pm
    Author:  David López Saludes

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpectrumAnalyzerComponent.h"

//==============================================================================
SpectrumAnalyzerComponent::SpectrumAnalyzerComponent() : forwardFFT(fftOrder), window(fftSize, juce::dsp::WindowingFunction<float>::hann), 
							mindBValue(-100.0f), maxdBValue(0.0f), skewedYKnobRMS(0.2f), lvlOffsetRMS(1.0f)
{
	setOpaque(true);
	startTimerHz(30);
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
	stopTimer();
}

void SpectrumAnalyzerComponent::processAudioBlock(const juce::AudioBuffer<float>& buffer)
{
	if(buffer.getNumChannels() > 0)
	{
		const float* channelData = buffer.getReadPointer(0);

		for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
			pushNextSampleIntoFifo(channelData[sample]);
	}
}

void SpectrumAnalyzerComponent::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);

	g.setOpacity(1.0f);
	g.setColour(juce::Colours::white);
	drawFrame(g);

	g.drawText("RMS", getLocalBounds(), juce::Justification::centredTop);
	g.clipRegionIntersects(getLocalBounds());
}

void SpectrumAnalyzerComponent::resized()
{

}

void SpectrumAnalyzerComponent::mouseDown(const juce::MouseEvent& e)
{
	clicked = true;
	repaint();
}

void SpectrumAnalyzerComponent::timerCallback()
{
	if (nextFFTBlockReady)
	{
		drawNextFrameOfSpectrum();
		nextFFTBlockReady = false;
		repaint();
	}
}

void SpectrumAnalyzerComponent::pushNextSampleIntoFifo(float sample) noexcept
{
	if (fifoIndex == fftSize)
	{
		if (!nextFFTBlockReady)
		{
			juce::zeromem(fftData, sizeof(fftData));
			memcpy(fftData, fifo, sizeof(fifo));
			nextFFTBlockReady = true;
		}
		fifoIndex = 0;
	}
	fifo[fifoIndex++] = sample;
}

void SpectrumAnalyzerComponent::drawNextFrameOfSpectrum()
{

	window.multiplyWithWindowingTable(fftData, fftSize);

	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	auto mindB = mindBValue;//-100.0f
	auto maxdB = maxdBValue;//0.0f

	for (int i = 0; i < scopeSize; ++i)
	{
		auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * skewedYKnobRMS);//0.2f
		auto fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionX * (float)fftSize * 0.5f));
		auto level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(fftData[fftDataIndex]) - juce::Decibels::gainToDecibels((float)fftSize)),
			mindB, maxdB, 0.0f, lvlOffsetRMS);//1.0f

		scopeData[i] = level;
	}

}

void SpectrumAnalyzerComponent::drawFrame(juce::Graphics& g)
{
	for (int i = 1; i < scopeSize; ++i)
	{
		auto width = getLocalBounds().getWidth();
		auto height = getLocalBounds().getHeight();

		g.drawLine({ (float)juce::jmap(i - 1, 0, scopeSize - 1, 0, width),
						   juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
					(float)juce::jmap(i, 0, scopeSize - 1, 0, width),
						   juce::jmap(scopeData[i], 0.0f, 1.0f, (float)height, 0.0f) });
	}
}
