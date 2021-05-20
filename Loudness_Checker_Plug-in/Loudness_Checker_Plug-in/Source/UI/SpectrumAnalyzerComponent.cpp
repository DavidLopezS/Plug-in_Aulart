/*
  ==============================================================================

    SpectrumAnalyzerComponent.cpp
    Created: 22 Apr 2021 1:10:11pm
    Author:  David López Saludes

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpectrumAnalyzerComponent.h"

//==============================================================================
SpectrumAnalyzerComponent::SpectrumAnalyzerComponent() : leftPathProducer(leftChannelFifo), rightPathProducer(rightChannelFifo),
														 forwardFFT(fftOrder), spectrogramImage(juce::Image::RGB, 512, 512, true)
{
	setOpaque(true);
	startTimerHz(30);//30
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
	stopTimer();
}

void SpectrumAnalyzerComponent::processAudioBlock(const juce::AudioBuffer<float>& buffer)
{
	if (buffer.getNumChannels() > 0)
	{
		const float* channelData = buffer.getReadPointer(0);
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
			pushNextSampleIntoFifo(channelData[sample]);
	}
}

void SpectrumAnalyzerComponent::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);

	auto responseAreaRMS = getAnalysisAreaRMS();
	auto renderAreaRMS = getRenderAreaRMS();

	auto responseAreaSpectr = getAnalysisAreaRMS();
	auto renderAreaSpectr = getRenderAreaRMS();

	g.setOpacity(1.0f);

	if (isRMS)
	{
		g.drawImage(backgroundRMS, getLocalBounds().toFloat());

		auto leftChannelFFTPath = leftPathProducer.getPath();
		auto rightChannelFFTPath = rightPathProducer.getPath();

		leftChannelFFTPath.applyTransform(juce::AffineTransform().translation(responseAreaRMS.getX(), -10.0f));
		rightChannelFFTPath.applyTransform(juce::AffineTransform().translation(responseAreaRMS.getX(), -10.0f));

		g.setColour(juce::Colours::white);
		g.strokePath(leftChannelFFTPath, juce::PathStrokeType(1.0f));

		g.setColour(juce::Colours::skyblue);
		g.strokePath(rightChannelFFTPath, juce::PathStrokeType(1.0f));

		g.setColour(juce::Colours::orange);
		g.drawRoundedRectangle(responseAreaRMS.toFloat(), 4.0f, 1.0f);

		g.clipRegionIntersects(getLocalBounds());
	}
	else
	{
		g.drawImage(backgroundSpectr, getLocalBounds().toFloat());
		g.drawImage(spectrogramImage, responseAreaSpectr.toFloat());
	}
}



void SpectrumAnalyzerComponent::resized()
{
	backgroundRMS = juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true);
	backgroundSpectr = juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true);


	juce::Graphics gRMS(backgroundRMS);
	juce::Graphics gSpectr(backgroundSpectr);

	auto renderAreaRMS = getAnalysisAreaRMS();
	auto leftRMS = renderAreaRMS.getX();
	auto rightRMS = renderAreaRMS.getRight();
	auto topRMS = renderAreaRMS.getY();
	auto bottomRMS = renderAreaRMS.getHeight();
	auto widthRMS = renderAreaRMS.getWidth();

	//RMS grid
	juce::Array<float> freqRMS
	{
		20, 50, 100,
		200, 500, 1000,
		2000, 5000, 10000, 20000
	};

	//lines representation
	juce::Array<float> xPos;
	for(auto f : freqRMS)
	{
		auto normX = juce::mapFromLog10(f, 20.0f, 20000.0f);
		xPos.add(leftRMS + widthRMS * normX);
	}

	gRMS.setColour(juce::Colours::dimgrey);
	for(auto x : xPos)
	{
		gRMS.drawVerticalLine(x, topRMS, bottomRMS);
	}

	juce::Array<float> gain
	{
		-24, -12, 0, 12, 24
	};
	
	for(auto gDb : gain)
	{
		auto y = juce::jmap(gDb, -24.0f, 24.0f, float(bottomRMS), float(topRMS));
		gRMS.setColour(gDb == 0.0f ? juce::Colour(0u, 172u, 1u) : juce::Colours::darkgrey);
		gRMS.drawHorizontalLine(y, leftRMS, rightRMS); 
	}

	//Values representation
	gRMS.setColour(juce::Colours::lightgrey);
	const int fontHeightRMS = 10;
	gRMS.setFont(fontHeightRMS);

	for(int i = 0; i < freqRMS.size(); ++i)
	{
		auto f = freqRMS[i];
		auto x = xPos[i];

		bool addK = false;
		juce::String str;
		if(f > 999.0f)
		{
			addK = true;
			f /= 1000.0f;
		}

		str << f;
		if (addK)
			str << "k";
		str << "Hz";

		auto textWidth = gRMS.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r;
		r.setSize(textWidth, fontHeightRMS);
		r.setCentre(x, 0);
		r.setY(1);

		gRMS.drawFittedText(str, r, juce::Justification::centred, 1);
	}

	for(auto gDb : gain)
	{
		auto y = juce::jmap(gDb, -24.0f, 24.0f, float(bottomRMS), float(topRMS));

		juce::String str;
		if (gDb > 0)
			str << "+";
		str << gDb;

		auto textWidth = gRMS.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r;
		r.setSize(textWidth, fontHeightRMS);
		r.setX(getWidth() - textWidth);
		r.setCentre(r.getCentreX(), y);

		gRMS.setColour(gDb == 0.0f ? juce::Colour(0u, 172u, 1u) : juce::Colours::lightgrey);

		gRMS.drawFittedText(str, r, juce::Justification::centred, 1);

		str.clear();
		str << (gDb - 24.0f);

		r.setX(1);
		textWidth = gRMS.getCurrentFont().getStringWidth(str);
		r.setSize(textWidth, fontHeightRMS);

		gRMS.drawFittedText(str, r, juce::Justification::centred, 1);
	}
	
	auto renderAreaSpectr = getAnalysisAreaSpectr();
	auto leftSpectr = renderAreaSpectr.getX();
	auto rightSpectr = renderAreaSpectr.getRight();
	auto topSpectr = renderAreaSpectr.getY();
	auto bottomSpectr = renderAreaSpectr.getHeight();
	auto widthSpectr = renderAreaSpectr.getWidth();

	//Spectr Grid
	juce::Array<float> freqSpectr
	{
		20, 5000, 10000, 15000, 20000
	};

	//Values representation
	gSpectr.setColour(juce::Colours::lightgrey);
	const int fontHeight = 10;
	gSpectr.setFont(fontHeight);


	for (auto f : freqSpectr)
	{
		auto y = juce::jmap(f, 20.0f, 20000.0f, float(bottomSpectr), float(topSpectr));

		bool addK = false;
		juce::String str;
		if (f > 999.0f)
		{
			addK = true;
			f /= 1000.0f;
		}

		str << f;
		if (addK)
			str << "k";
		str << "Hz";

		auto textWidth = gSpectr.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setX(getWidth() - textWidth);
		r.setCentre(r.getCentreX(), y);

		gSpectr.setColour(f == 0.0f ? juce::Colour(0u, 172u, 1u) : juce::Colours::lightgrey);

		gSpectr.drawFittedText(str, r, juce::Justification::centred, 1);

	}
}

void SpectrumAnalyzerComponent::selGrid(const int choice)
{
	switch(choice)
	{
	case 0:
		isRMS = true;
		break;
	case 1:
		isRMS = false;
		break;
	default:
		jassertfalse;
		break;
	}
}

juce::Rectangle<int> SpectrumAnalyzerComponent::getRenderAreaRMS()
{
	auto area = getLocalBounds();
	
	
	area.removeFromTop(15);
	area.removeFromBottom(0);
	area.removeFromRight(20);
	area.removeFromLeft(20);

	return area;
}

juce::Rectangle<int> SpectrumAnalyzerComponent::getRenderAreaSpectr()
{
	auto area = getLocalBounds();

	area.removeFromTop(0);
	area.removeFromBottom(-3);
	area.removeFromRight(31);
	area.removeFromLeft(0);

	return area;
}

juce::Rectangle<int> SpectrumAnalyzerComponent::getAnalysisAreaRMS()
{
	auto area = getRenderAreaRMS();
	
	area.removeFromTop(0);
	area.removeFromBottom(2);

	return area;
}

juce::Rectangle<int> SpectrumAnalyzerComponent::getAnalysisAreaSpectr()
{
	auto area = getRenderAreaSpectr();

	area.removeFromTop(4);
	area.removeFromBottom(4);//4

	return area;
}

void SpectrumAnalyzerComponent::mouseDown(const juce::MouseEvent& e)
{
	clicked = true;
	repaint();
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

			leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
		}
	}

	const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
	const auto binWidth = sampleRate / double(fftSize);

	while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
	{
		std::vector<float> fftData;
		if (leftChannelFFTDataGenerator.getFFTData(fftData))
		{
			pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
		}
	}

	while (pathProducer.getNumPathsAvailable() > 0)
	{
		pathProducer.getPath(leftChannelFFTPath);
	}
}

void SpectrumAnalyzerComponent::timerCallback()
{
		auto fftBounds = getAnalysisAreaRMS().toFloat();

		leftPathProducer.process(fftBounds, sampleRate);
		rightPathProducer.process(fftBounds, sampleRate);

		repaint();

		if (nextFFTBlockReady)
		{
			drawNextLineOfSpectrogram();
			nextFFTBlockReady = false;
			repaint();
		}
}

void SpectrumAnalyzerComponent::pushNextSampleIntoFifo(float sample) noexcept
{
	if (fifoIndex == fftSize)
	{
		if (!nextFFTBlockReady)
		{
			juce::zeromem(fftData, sizeof(fftData));
			memcpy(fftData, fifo, sizeof(fifo));
			nextFFTBlockReady = true;
		}
		fifoIndex = 0;
	}
	fifo[fifoIndex++] = sample;
}

void SpectrumAnalyzerComponent::drawNextLineOfSpectrogram()
{
	const int rightHandEdge = spectrogramImage.getWidth() - 1;
	const int imageHeight = spectrogramImage.getHeight();

	spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	juce::Range<float> maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);
	if (maxLevel.getEnd() == 0.0f)
		maxLevel.setEnd(0.00001f);//4.1f

	for (int i = 1; i < imageHeight; ++i)
	{
		const float skewedProportionY = 1.0f - std::exp(std::log(i / (float)imageHeight) * 0.2f);//0.4f
		const int fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
		const float level = juce::jmap(fftData[fftDataIndex], 0.0f, maxLevel.getEnd(), 0.0f, 1.0f);//Original targetRangeMax = 3.9f, needs to be tweaked/tested

		spectrogramImage.setPixelAt(rightHandEdge, i, juce::Colour::fromHSL(level, 1.0f, level, 1.0f));//Colour::fromHSV
	}

}

