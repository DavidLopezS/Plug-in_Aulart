/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             SpectrumAnalyserTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Displays an FFT spectrum analyser.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_dsp, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             Component
 mainClass:        AnalyserComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class AnalyserComponent   : public juce::AudioAppComponent,
                            private juce::Timer
{
public:
    AnalyserComponent()
		: forwardFFT (fftOrder),
		window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        setOpaque (true);
        setAudioChannels (2, 0);  // we want a couple of input channels but no outputs
        startTimerHz (30);
        setSize (700, 500);
    }

    ~AnalyserComponent() override
    {
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int, double) override {}
    void releaseResources() override          {}

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToBeFilled) override 
	{
		if(bufferToBeFilled.buffer->getNumChannels() > 0)
		{
			auto* channelData = bufferToBeFilled.buffer->getReadPointer(0, bufferToBeFilled.startSample);

			for (auto i = 0; i < bufferToBeFilled.numSamples; ++i)
				pushNextSampleIntoFifo(channelData[i]);
		}
	}

    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);

        g.setOpacity (1.0f);
        g.setColour (juce::Colours::white);
        drawFrame (g);
    }

    void timerCallback() override 
	{
		if (nextFFTBlockReady)
		{
			drawNextFrameOfSpectrum();
			nextFFTBlockReady = false;
			repaint();
		}
	}

    void pushNextSampleIntoFifo (float sample) noexcept
    {
        // if the fifo contains enough data, set a flag to say
        // that the next frame should now be rendered..
		if(fifoIndex == fftSize)
		{
			if(!nextFFTBlockReady)
			{
				juce::zeromem(fftData, sizeof(fftData));
				memcpy(fftData, fifo, sizeof(fifo));
				nextFFTBlockReady = true;
			}
			fifoIndex = 0;
		}
		fifo[fifoIndex++] = sample;
    }

    void drawNextFrameOfSpectrum() 
	{
		// First, apply the windowing function to the incoming data by calling the multiplyWithWindowingTable() 
		//function on the window object and passing the data as an argument.
		window.multiplyWithWindowingTable(fftData, fftSize);

		//Then, render the FFT data using the performFrequencyOnlyForwardTransform() function on the FFT object with the fftData array as an argument.
		forwardFFT.performFrequencyOnlyForwardTransform(fftData);

		auto mindB = -100.0f;
		auto maxdB = 0.0f;

		//Now in the for loop for every point in the scope width, calculate the level proportionally to the desired minimum and maximum decibels. To do this, we first need to skew the x-axis to use a logarithmic scale to better represent our frequencies. 
		//We can then feed this scaling factor to retrieve the correct array index and use the amplitude value to map it to a range between 0.0 .. 1.0.
		for(int i = 0; i < scopeSize; ++i)
		{
			auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * 0.2f);
			auto fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionX * (float)fftSize * 0.5f));
			auto level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(fftData[fftDataIndex]) - juce::Decibels::gainToDecibels((float)fftSize)), 
																																			 mindB, maxdB, 0.0f, 1.0f);
			//Finally set the appropriate point with the correct amplitude to prepare the drawing process.
			scopeData[i] = level;
		}
	}

	//As a final step, the paint() callback will call our helper function drawFrame() whenever a repaint() request has been initiated and the frame can be drawn as follows
    void drawFrame (juce::Graphics& g)     
	{
		for(int i = 1; i < scopeSize; ++i)
		{
			auto width = getLocalBounds().getWidth();
			auto height = getLocalBounds().getHeight();

			g.drawLine({ (float)juce::jmap(i - 1, 0, scopeSize - 1, 0, width),
							   juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
						(float)juce::jmap(i, 0, scopeSize - 1, 0, width),
							   juce::jmap(scopeData[i], 0.0f, 1.0f, (float)height, 0.0f) });
		}
	}

	enum
	{
		fftOrder = 11,//The order designates the size of the FFT window, and the number of point corresponds to 2 to the power of the order, in this case 2^11 = 2024
		fftSize = 1 << fftOrder,//To calculate the corresponding FFT size, we use the left bit shift operator which produces 2048 as binary number 100000000000.
		scopeSize = 512//We also set the number of points in the visual representation of the spectrum as a scope size of 512.
	};

private:
	juce::dsp::FFT forwardFFT;//Declare a dsp::FFT object to perform the forward FFT on.
	juce::dsp::WindowingFunction<float> window;//Also declare a dsp::WindowingFunction object to apply the windowing function on the signal.
	 
	float fifo[fftSize];//The fifo float array of size 2048 will contain our incoming audio data in samples.
	float fftData[2 * fftSize];//The fftData float array of size 4096 will contain the results of our FFT calculations
	int fifoIndex = 0;//This temporary index keeps count of the amount of samples in the fifo.
	bool nextFFTBlockReady = false;//This temporary boolean tells us whether the next FFT block is ready to be rendered.
	float scopeData[scopeSize];//The scopeData float array of size 512 will contain the points to display on the screen.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyserComponent)
};
