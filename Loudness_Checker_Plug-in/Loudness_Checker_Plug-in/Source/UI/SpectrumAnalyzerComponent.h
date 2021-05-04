/*
  ==============================================================================

    SpectrumAnalyzerComponent.h
    Created: 22 Apr 2021 1:10:11pm
    Author:  David López Saludes

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SpectrumAnalyzerComponent  : public juce::Component, private juce::Timer
{
public:
    SpectrumAnalyzerComponent();
    ~SpectrumAnalyzerComponent() override;

	void processAudioBlock(const juce::AudioBuffer<float>&);
    void paint (juce::Graphics&) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent&) override;
	void pushNextSampleIntoFifo(float) noexcept;
	void timerCallback() override;
	void drawNextFrameOfSpectrum(/*const float*/);
	void drawFrame(juce::Graphics&);

	enum
	{
		fftOrder = 11,
		fftSize = 1 << fftOrder,
		scopeSize = 512 //2048
	};

	float mindBValue;
	float maxdBValue;

	

private:

	juce::dsp::FFT forwardFFT;
	juce::dsp::WindowingFunction<float> window;

	float fifo[fftSize];
	int fifoIndex = 0;
	float fftData[2 * fftSize];
	bool nextFFTBlockReady = false;


	float scopeData[scopeSize];

	bool clicked = false;

	using SliderAttatchment = juce::AudioProcessorValueTreeState::SliderAttachment;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzerComponent)
};
