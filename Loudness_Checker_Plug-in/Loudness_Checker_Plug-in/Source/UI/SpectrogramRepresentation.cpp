/*
  ==============================================================================

    SpectrogramRepresentation.cpp
    Created: 26 Apr 2021 9:19:36am
    Author:  David L�pez Saludes

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpectrogramRepresentation.h"

//==============================================================================
SpectrogramRepresentation::SpectrogramRepresentation() : forwardFFT(fftOrder), spectrogramImage(juce::Image::RGB, 400, 300, true)
{
	setOpaque(true);
	startTimerHz(60);
}

SpectrogramRepresentation::~SpectrogramRepresentation()
{
	stopTimer();
}

void SpectrogramRepresentation::processAudioBlock(const juce::AudioBuffer<float>& buffer)
{
	if(buffer.getNumChannels() > 0)
	{
		const float* channelData = buffer.getReadPointer(0);
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
			pushNextSampleIntoFifo(channelData[sample]);
	}
}

void SpectrogramRepresentation::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::grey);

	g.setOpacity(1.0f);
	g.drawImage(spectrogramImage, getLocalBounds().toFloat());	
}

void SpectrogramRepresentation::timerCallback()
{
	if(nextFFTBlockReady)
	{
		drawNextLineOfSpectrogram();
		nextFFTBlockReady = false;
		repaint();
	}
}

void SpectrogramRepresentation::pushNextSampleIntoFifo(float sample) noexcept
{
	if(fifoIndex == fftSize)
	{
		if(!nextFFTBlockReady)
		{
			juce::zeromem(fftData, sizeof(fftData));
			memcpy(fftData, fifo, sizeof(fifo));
			nextFFTBlockReady = true;
		}

		fifoIndex = 0;

	}

	fifo[fifoIndex++] = sample;

}

void SpectrogramRepresentation::drawNextLineOfSpectrogram()
{
	const int rightHandEdge = spectrogramImage.getWidth() - 1;
	const int imageHeight = spectrogramImage.getHeight();

	spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	juce::Range<float> maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);
	if (maxLevel.getEnd() == 0.0f)
		maxLevel.setEnd(0.00001);

	for(int i = 1; i < imageHeight; ++i)
	{
		const float skewedProportionY = 1.0f - std::exp(std::log(i / (float)imageHeight) * 0.2f);
		const int fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
		const float level = juce::jmap(fftData[fftDataIndex], 0.0f, maxLevel.getEnd(), 0.0f, 1.0f);

		spectrogramImage.setPixelAt(rightHandEdge, i, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
	}

}