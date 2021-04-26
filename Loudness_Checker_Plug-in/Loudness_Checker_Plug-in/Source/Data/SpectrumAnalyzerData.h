/*
  ==============================================================================

    SpectrumAnalyzerData.h
    Created: 22 Apr 2021 1:10:24pm
    Author:  David López Saludes

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SpectrumAnalyzerData 
{
public:

	void processAudioBlock(const juce::AudioBuffer<float>&);
	void pushNextSampleIntoFifo(float) noexcept;

	enum
	{
		fftOrder = 11,
		fftSize = 1 << fftOrder,
		scopeSize = 2048 //512
	};

	float fftData[2 * fftSize];
	bool nextFFTBlockReady = false;

private:
	float fifo[fftSize];
	int fifoIndex = 0;
};