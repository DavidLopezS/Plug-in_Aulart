/*
  ==============================================================================

    SpectrumAnalyzerData.h
    Created: 22 Apr 2021 1:10:24pm
    Author:  david

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SpectrumAnalyzerData /*: public juce::AudioProcessor*/
{
public:

	void pushNextSampleIntoFifo(float) noexcept;

	enum
	{
		fftOrder = 11,
		fftSize = 1 << fftOrder,
		scopeSize = 2048
	};

	float fftData[2 * fftSize];
	bool nextFFTBlockReady = false;

private:
	float fifo[fftSize];
	int fifoIndex = 0;
};