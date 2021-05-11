/*
  ==============================================================================

    SpectrogramRepresentation.cpp
    Created: 26 Apr 2021 9:19:36am
    Author:  David López Saludes

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpectrogramRepresentation.h"

//==============================================================================
SpectrogramRepresentation::SpectrogramRepresentation() : forwardFFT(fftOrder), spectrogramImage(juce::Image::RGB, 512, 512, true), 
														 lvlKnobSpectr(0.1), skewedPropSpectr(0.1f), lvlOffsetSpectr(2.0f)
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
	g.drawText("Spectrogram", getLocalBounds(), juce::Justification::centredTop);
	g.drawImage(spectrogramImage, getLocalBounds().toFloat());	

	g.clipRegionIntersects(getLocalBounds());
}

void SpectrogramRepresentation::resized()
{

}

void SpectrogramRepresentation::mouseDown(const juce::MouseEvent& e)
{
	clicked = true;
	repaint();
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
		maxLevel.setEnd(lvlKnobSpectr);//0.00001

	for(int i = 1; i < imageHeight; ++i)
	{
		const float skewedProportionY = 1.0f - std::exp(std::log(i / (float)imageHeight) * skewedPropSpectr);//0.2f
		const int fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
		const float level = juce::jmap(fftData[fftDataIndex], 0.0f, maxLevel.getEnd(), 0.0f, lvlOffsetSpectr);//Original targetRangeMax = 1.0f, needs to be tweaked/tested

		spectrogramImage.setPixelAt(rightHandEdge, i, juce::Colour::fromHSL(level, 1.0f, level, 1.0f));//Colour::fromHSV
	}

}
