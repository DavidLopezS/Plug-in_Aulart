/*
  ==============================================================================

    SpectrogramRepresentation.h
    Created: 26 Apr 2021 9:19:36am
    Author:  David López Saludes

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
	void resized() override;
	void mouseDown(const juce::MouseEvent&) override;
	void timerCallback() override;
	void pushNextSampleIntoFifo(float) noexcept;
	void drawNextLineOfSpectrogram();

	enum 
	{
		fftOrder = 11,//10
		fftSize = 1 << fftOrder //adds the ordet 10 into the binary number, so 10 extra zeros (00010000000000 = 1024)
	};

	float lvlKnobSpectr;
	float skewedPropSpectr;
	float lvlOffsetSpectr;

private:

	juce::Image background;
	juce::Rectangle<int> getRenderArea();
	juce::Rectangle<int> getAnalysisArea();

	juce::dsp::FFT forwardFFT;
	juce::Image spectrogramImage;

	float fifo[fftSize];
	int fifoIndex = 0;
	float fftData[2 * fftSize];
	bool nextFFTBlockReady = false;

	bool clicked = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramRepresentation)
};
