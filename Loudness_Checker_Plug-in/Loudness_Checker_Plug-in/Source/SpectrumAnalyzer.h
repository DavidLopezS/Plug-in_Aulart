/*
  ==============================================================================

    SpectrumAnalyzer.h
    Created: 21 Apr 2021 12:34:25pm
    Author:  david

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SpectrumAnalyzer  : public juce::Component, private juce::Timer
{
public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer() override;


	//void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;
    void paint (juce::Graphics&) override;
    void resized() override;
	void timerCallback() override;
	void pushNextSampleIntoFifo(float sample) noexcept;
	void drawNextFrameOfSpectrum();
	void drawFrame(juce::Graphics&);

	enum
	{
		fftOrder = 11,
		fftSize= 1 << fftOrder,
		scopeSize = 512
	};

private:
	juce::dsp::FFT forwardFFT;
	juce::dsp::WindowingFunction<float> window;

	float fifo[fftSize];
	float fftData[2 * fftSize];
	int fifoIndex = 0;
	bool nextFFTBlockReady = false;
	float scopeData[scopeSize];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzer)
};
