/*
  ==============================================================================

    SpectrogramRepresentation.cpp
    Created: 26 Apr 2021 9:19:36am
    Author:  David López Saludes

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SpectrogramRepresentation.h"

//==============================================================================
SpectrogramRepresentation::SpectrogramRepresentation() : forwardFFT(fftOrder), spectrogramImage(juce::Image::RGB, 512, 512, true), 
														 lvlKnobSpectr(4.1f), skewedPropSpectr(0.4f), lvlOffsetSpectr(3.9f)
{
	setOpaque(true);
	startTimerHz(60);
}

SpectrogramRepresentation::~SpectrogramRepresentation()
{
	stopTimer();
}

void SpectrogramRepresentation::processAudioBlock(const juce::AudioBuffer<float>& buffer)
{
	if(buffer.getNumChannels() > 0)
	{
		const float* channelData = buffer.getReadPointer(0);
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
			pushNextSampleIntoFifo(channelData[sample]);
	}
}

void SpectrogramRepresentation::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::grey);

	g.drawImage(background, getLocalBounds().toFloat());

	g.setOpacity(1.0f);
	g.drawImage(spectrogramImage, getAnalysisArea().toFloat());	

	g.clipRegionIntersects(getLocalBounds());
}

void SpectrogramRepresentation::resized()
{
	background = juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true);

	juce::Graphics g(background);

	juce::Array<float> freq
	{
		20, 50, 100,
		200, 500, 1000,
		2000, 5000, 10000, 20000
	};

	auto renderArea = getAnalysisArea();
	auto left = renderArea.getX();
	auto right = renderArea.getRight();
	auto top = renderArea.getY();
	auto bottom = renderArea.getHeight();
	auto width = renderArea.getWidth();

	//lines representation
	juce::Array<float> xPos;
	for (auto f : freq)
	{
		auto normX = juce::mapFromLog10(f, 20.0f, 20000.0f);
		xPos.add(left + width * normX);
	}

	g.setColour(juce::Colours::dimgrey);
	for (auto x : xPos)
	{
		g.drawVerticalLine(x, top, bottom);
	}

	juce::Array<float> gain
	{
		-24, -12, 0, 12, 24
	};

	for (auto gDb : gain)
	{
		auto y = juce::jmap(gDb, -24.0f, 24.0f, float(bottom), float(top));
		g.setColour(gDb == 0.0f ? juce::Colour(0u, 172u, 1u) : juce::Colours::darkgrey);
		g.drawHorizontalLine(y, left, right);
	}

	//Values representation
	g.setColour(juce::Colours::lightgrey);
	const int fontHeight = 10;
	g.setFont(fontHeight);

	for (int i = 0; i < freq.size(); ++i)
	{
		auto f = freq[i];
		auto x = xPos[i];

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

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setCentre(x, 0);
		r.setY(1);

		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}

	for (auto gDb : gain)
	{
		auto y = juce::jmap(gDb, -24.0f, 24.0f, float(bottom), float(top));

		juce::String str;
		if (gDb > 0)
			str << "+";
		str << gDb;

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setX(getWidth() - textWidth);
		r.setCentre(r.getCentreX(), y);

		g.setColour(gDb == 0.0f ? juce::Colour(0u, 172u, 1u) : juce::Colours::lightgrey);

		g.drawFittedText(str, r, juce::Justification::centred, 1);

		str.clear();
		str << (gDb - 24.0f);

		r.setX(1);
		textWidth = g.getCurrentFont().getStringWidth(str);
		r.setSize(textWidth, fontHeight);

		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}

}

juce::Rectangle<int> SpectrogramRepresentation::getRenderArea()
{
	auto area = getLocalBounds();

	area.removeFromTop(11);
	area.removeFromBottom(0);
	area.removeFromRight(20);
	area.removeFromLeft(20);

	return area;
}

juce::Rectangle<int> SpectrogramRepresentation::getAnalysisArea()
{
	auto area = getRenderArea();

	area.removeFromTop(4);
	area.removeFromBottom(4);//4

	return area;
}

void SpectrogramRepresentation::mouseDown(const juce::MouseEvent& e)
{
	clicked = true;
	repaint();
}

void SpectrogramRepresentation::timerCallback()
{
	if(nextFFTBlockReady)
	{
		drawNextLineOfSpectrogram();
		nextFFTBlockReady = false;
		repaint();
	}
}

void SpectrogramRepresentation::pushNextSampleIntoFifo(float sample) noexcept
{
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

void SpectrogramRepresentation::drawNextLineOfSpectrogram()
{
	const int rightHandEdge = spectrogramImage.getWidth() - 1;
	const int imageHeight = spectrogramImage.getHeight();

	spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	juce::Range<float> maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);
	if (maxLevel.getEnd() == 0.0f)
		maxLevel.setEnd(lvlKnobSpectr);//0.00001

	for(int i = 1; i < imageHeight; ++i)
	{
		const float skewedProportionY = 1.0f - std::exp(std::log(i / (float)imageHeight) * skewedPropSpectr);//0.2f
		const int fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
		const float level = juce::jmap(fftData[fftDataIndex], 0.0f, maxLevel.getEnd(), 0.0f, lvlOffsetSpectr);//Original targetRangeMax = 1.0f, needs to be tweaked/tested

		spectrogramImage.setPixelAt(rightHandEdge, i, juce::Colour::fromHSL(level, 1.0f, level, 1.0f));//Colour::fromHSV
	}

}
