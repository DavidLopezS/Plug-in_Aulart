/*
  ==============================================================================

    SpectrumAnalyzerComponent.h
    Created: 22 Apr 2021 1:10:11pm
    Author:  david

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "..\Data\SpectrumAnalyzerData.h"

//==============================================================================
/*
*/
class SpectrumAnalyzerComponent  : public juce::Component, private juce::Timer
{
public:
    SpectrumAnalyzerComponent();
    ~SpectrumAnalyzerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
	void timerCallback() override;
	void drawNextFrameOfSpectrum();
	void drawFrame(juce::Graphics&);

private:

	juce::dsp::FFT forwardFFT;
	juce::dsp::WindowingFunction<float> window;

	static SpectrumAnalyzerData dataAnalyzer;

	float scopeData[dataAnalyzer.scopeSize];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzerComponent)
};
