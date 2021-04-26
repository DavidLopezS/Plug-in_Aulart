/*
  ==============================================================================

    SpectrogramRepresentation.h
    Created: 26 Apr 2021 9:19:36am
    Author:  david

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SpectrogramRepresentation  : public juce::Component, private juce::Timer
{
public:
    SpectrogramRepresentation();
    ~SpectrogramRepresentation() override;

	void processAudioBlock(const juce::AudioBuffer<float>&);
    void paint (juce::Graphics&) override;
	void timerCallback() override;
	void pushNextSampleIntoFifo(float) noexcept;
	void drawNextLineOfSpectrogram();

	enum 
	{
		fftOrder = 10,
		fftSize = 1 << fftOrder //adds the ordet 10 into the binary number, so 10 extra zeros (00010000000000 = 1024)
	};

private:

	juce::dsp::FFT forwardFFT;
	juce::Image spectrogramImage;

	float fifo[fftSize];
	float fftData[2 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramRepresentation)
};
