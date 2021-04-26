/*
  ==============================================================================

    SpectrumAnalyzerData.cpp
    Created: 22 Apr 2021 1:10:24pm
    Author:  David López Saludes

  ==============================================================================
*/

#include "SpectrumAnalyzerData.h"

void SpectrumAnalyzerData::processAudioBlock(const juce::AudioBuffer<float>& buffer)
{
	if(buffer.getNumChannels() > 0)
	{
	
		const float* channelData = buffer.getReadPointer(0);

		for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
			pushNextSampleIntoFifo(channelData[sample]);

	}
}

void SpectrumAnalyzerData::pushNextSampleIntoFifo(float sample) noexcept
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