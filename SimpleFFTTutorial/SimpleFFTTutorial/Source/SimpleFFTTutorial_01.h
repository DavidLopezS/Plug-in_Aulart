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

 name:             SimpleFFTTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Displays an FFT spectrogram.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_dsp, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             Component
 mainClass:        SpectrogramComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
class SpectrogramComponent   : public juce::AudioAppComponent,
                               private juce::Timer
{
public:
    SpectrogramComponent()
        : forwardFFT(fftOrder),//Constructor of the FFT with its order
		spectrogramImage (juce::Image::RGB, 512, 512, true)
    {
        setOpaque (true);
        setAudioChannels (2, 0);  // we want a couple of input channels but no outputs
        startTimerHz (60);
        setSize (700, 500);
    }

    ~SpectrogramComponent() override
    {
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int, double) override {}
    void releaseResources() override          {}
	
	//In this function we simply push all the samples contained in our current audio buffer block to the fifo to be processed at a later time
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override 
	{
		if(bufferToFill.buffer->getNumChannels() > 0)
		{
			auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

			for (auto i = 0; i < bufferToFill.numSamples; ++i)
				pushNextSampleIntoFifo(channelData[i]);
		}
	}

    //==============================================================================
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black);

        g.setOpacity (1.0f);
        g.drawImage (spectrogramImage, getLocalBounds().toFloat());
    }

    void timerCallback() override 
	{
		if(nextFFTBlockReady)
		{
			drawNextLineOfSpectrogram();
			nextFFTBlockReady = false;
			repaint();
		}
	}

    void pushNextSampleIntoFifo (float sample) noexcept
    {
        // if the fifo contains enough data, set a flag to say
        // that the next line should now be rendered..
		//We also set a flag to say that the next line should now be rendered and always reset the index to 0 to start filling the fifo again.
		if(fifoIndex == fftSize)
		{
			if(!nextFFTBlockReady)
			{
				std::fill(fftData.begin(), fftData.end(), 0.0f);
				std::copy(fifo.begin(), fifo.end(), fftData.begin());
				nextFFTBlockReady = true;
			}

			fifoIndex = 0;
		}
		fifo[(size_t)fifoIndex++] = sample;	
    }

    void drawNextLineOfSpectrogram() 
	{
		auto rightHandEdge = spectrogramImage.getWidth() - 1;
		auto imageHeight = spectrogramImage.getHeight();

		// First, shuffle the image leftwards by 1 pixel using the moveImageSection() function on the Image object. 
		// Specify the image section as the whole width minus one pixel and the whole height.
		spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

		// Then, render the FFT data using the performFrequencyOnlyForwardTransform() function on the FFT object with the fftData array as an argument.
		forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

		// Find the range of values produced, so we can scale our rendering to
		// show up the detail clearly.
		// We can do so using the FloatVectorOperations::findMinAndMax() function.
		auto maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), fftSize / 2);

		// Now in the for loop for every pixel in the spectrogram height, calculate the level proportionally to the sample set.
		// To do this, we first need to skew the y-axis to use a logarithmic scale to better represent our frequencies.
		// We can then feed this scaling factor to retrieve the correct array index and use the amplitude value to map it to a range between 0.0 .. 1.0.
		for(auto y = 1; y < imageHeight; ++y)
		{
			auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)imageHeight) * 0.2f);
			auto fftDataIndex = (size_t)juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
			auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);

			// Finally set the appropriate pixel with the correct colour to display the FFT data.
			spectrogramImage.setPixelAt(rightHandEdge, y, juce::Colour::fromHSV(level, 1.0f, level, 1.0f));
		}
	}

	static constexpr auto fftOrder = 10; //The order designates the size of the FFT window, and the number of point corresponds to 2 to the power of the order, in this case 2^10 = 1024
	static constexpr auto fftSize = 1 << fftOrder; //To calculate the corresponding FFT size, we use the left bit shift operator which produces 1024 as binary number 10000000000.

private:
	juce::dsp::FFT forwardFFT;//FFT variable in which we'll perform the forward FFT on
    juce::Image spectrogramImage;

	std::array<float, fftSize> fifo;//First in, First out structure array that will store the data in samples, the size will be 2 ^10 = 1024
	std::array<float, fftSize * 2> fftData;//The data array will contain the results of the FFT calculations by a bigger size 2048
	int fifoIndex = 0;//Temporary index that will keep count of the amount of samples in the fifo
	bool nextFFTBlockReady = false;// Temporary boolean that tells us if the next FFT block is ready to be rendered

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrogramComponent)
};
