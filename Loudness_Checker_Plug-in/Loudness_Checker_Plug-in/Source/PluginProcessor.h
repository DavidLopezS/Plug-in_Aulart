/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SpectrumAnalyzer.h"

//==============================================================================
/**
*/
class Loudness_Checker_PluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    Loudness_Checker_PluginAudioProcessor();
    ~Loudness_Checker_PluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	void pushNextSampleIntoFifo(float sample) noexcept;

	enum
	{
		fftOrder = 11,
		fftSize = 1 << fftOrder,
		scopeSize = 512
	};

	float fifo[fftSize];
	float fftData[2 * fftSize];
	int fifoIndex = 0;

	float scopeData[scopeSize];

	bool nextFFTBlockReady = false;

private:
    
	//SpectrumAnalyzer spectrumAnalysis;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Loudness_Checker_PluginAudioProcessor)
};
