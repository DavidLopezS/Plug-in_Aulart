/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Loudness_MeterAudioProcessorEditor::Loudness_MeterAudioProcessorEditor (Loudness_MeterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), gridRepresentation(p), mydBKnobs(juce::Colours::darkgrey), mySelectorManager(juce::Colours::darkgrey)
{
	addAndMakeVisible(&gridRepresentation);

	addAndMakeVisible(&mydBKnobs);
	for (int i = 0; i < numKnobs; ++i)
		knobAttachment(i);

	addAndMakeVisible(&mySelectorManager);
	for (int i = 0; i < numSelectors; ++i)
		selectorAttachment(i);

    setSize (900, 500);
}

Loudness_MeterAudioProcessorEditor::~Loudness_MeterAudioProcessorEditor()
{
	myKnobAttachments.clear(); 
	mySelectorAttachments.clear();
}

void Loudness_MeterAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkgrey);
}

void Loudness_MeterAudioProcessorEditor::resized()
{
	juce::Grid grid;

	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;

	grid.templateRows = { Track(Fr(3)), Track(Fr(1)), Track(Fr(2)) };
	grid.templateColumns = { Track(Fr(1)) };

	grid.items = { juce::GridItem(gridRepresentation), juce::GridItem(mySelectorManager), juce::GridItem(mydBKnobs) };

	grid.performLayout(getLocalBounds());
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
	juce::AudioBuffer<float> tempIncomingBuffer;
	while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
	{
		if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
		{
			auto size = tempIncomingBuffer.getNumSamples();

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
				monoBuffer.getReadPointer(0, size),
				monoBuffer.getNumSamples() - size);

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
				tempIncomingBuffer.getReadPointer(0, 0),
				size);

			leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, offsetRMS);//-48.0f
		}
	}

	const auto fftSizeRMS = leftChannelFFTDataGenerator.getFFTSize();
	const auto binWidthRMS = sampleRate / double(fftSizeRMS);

	while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
	{
		std::vector<float> fftDataRMS;
		if (leftChannelFFTDataGenerator.getFFTData(fftDataRMS))
		{
			pathProducer.generatePath(fftDataRMS, fftBounds, fftSizeRMS, binWidthRMS, offsetRMS);//-48.0f
		}
	}

	while (pathProducer.getNumPathsAvailable() > 0)
	{
		pathProducer.getPath(leftChannelFFTPath);
	}
}

void ImageProducer::process(double sampleRate)
{
	juce::AudioBuffer<float> tempIncomingBuffer;
	while(spectrChannelFifo->getNumCompleteBuffersAvailable() > 0)
	{
		if(spectrChannelFifo->getAudioBuffer(tempIncomingBuffer))
		{
			auto size = tempIncomingBuffer.getNumSamples();

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
				monoBuffer.getReadPointer(0, size),
				monoBuffer.getNumSamples() - size);

			juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
				tempIncomingBuffer.getReadPointer(0, 0),
				size);

			spectrChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, 0.0f);
		}
	}

	const auto fftSizeSpectr = spectrChannelFFTDataGenerator.getFFTSize();
	const auto binWidthSpectr = sampleRate / double(fftSizeSpectr);

	while(spectrChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
	{
		std::vector<float> fftDataSpectr;
		if(spectrChannelFFTDataGenerator.getFFTData(fftDataSpectr))
		{ 
			imageProducer.generateImage(fftDataSpectr, spectrChannelFFTImage, fftSizeSpectr, binWidthSpectr, 0.0f);
		}
	}

	//while(imageProducer.getNumImagesAvailable() > 0)
	//{
	//	imageProducer.getImage(spectrChannelFFTImage);
	//}
}

void Loudness_MeterAudioProcessorEditor::knobAttachment(int knobId)
{
	auto &myKnobs = *mydBKnobs.myKnobs[knobId];

	using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	myKnobAttachments.push_back(std::make_unique<Attachment>(audioProcessor.apvts, myKnobName[knobId], myKnobs));
}

void Loudness_MeterAudioProcessorEditor::selectorAttachment(int selectorId)
{
	auto &mySelectors = *mySelectorManager.myComboBoxes[selectorId];

	using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
	mySelectorAttachments.push_back(std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, mySelectorNames[selectorId], mySelectors));
}