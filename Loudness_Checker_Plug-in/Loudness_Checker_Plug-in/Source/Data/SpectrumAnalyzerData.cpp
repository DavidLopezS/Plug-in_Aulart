/*
  ==============================================================================

    SpectrumAnalyzerData.cpp
    Created: 22 Apr 2021 1:10:24pm
    Author:  david

  ==============================================================================
*/

#include "SpectrumAnalyzerData.h"

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