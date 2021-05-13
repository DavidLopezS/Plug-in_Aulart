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
SpectrumAnalyzerComponent::SpectrumAnalyzerComponent() : forwardFFT(fftOrder), window(fftSize, juce::dsp::WindowingFunction<float>::hann), 
							mindBValue(-100.0f), maxdBValue(0.0f), skewedYKnobRMS(0.3f), dataIndexKnob(0.5f), lvlOffsetRMS(1.0f)
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
	if(buffer.getNumChannels() > 0)
	{
		const float* channelData = buffer.getReadPointer(0);

		for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
			pushNextSampleIntoFifo(channelData[sample]);
	}
}

void SpectrumAnalyzerComponent::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);

	auto responseArea = getAnalysisArea();

	g.drawImage(background, getLocalBounds().toFloat());

	g.setOpacity(1.0f);
	g.setColour(juce::Colours::white);
	drawFrame(g);

	g.clipRegionIntersects(getLocalBounds());
}

void SpectrumAnalyzerComponent::resized()
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
	for(auto f : freq)
	{
		auto normX = juce::mapFromLog10(f, 20.0f, 20000.0f);
		xPos.add(left + width * normX);
	}

	g.setColour(juce::Colours::dimgrey);
	for(auto x : xPos)
	{
		g.drawVerticalLine(x, top, bottom);
	}

	juce::Array<float> gain
	{
		-24, -12, 0, 12, 24
	};
	
	for(auto gDb : gain)
	{
		auto y = juce::jmap(gDb, -24.0f, 24.0f, float(bottom), float(top));
		g.setColour(gDb == 0.0f ? juce::Colour(0u, 172u, 1u) : juce::Colours::darkgrey);
		g.drawHorizontalLine(y, left, right); 
	}

	//Values representation
	g.setColour(juce::Colours::lightgrey);
	const int fontHeight = 10;
	g.setFont(fontHeight);

	for(int i = 0; i < freq.size(); ++i)
	{
		auto f = freq[i];
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

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		juce::Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setCentre(x, 0);
		r.setY(1);

		g.drawFittedText(str, r, juce::Justification::centred, 1);
	}

	for(auto gDb : gain)
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

juce::Rectangle<int> SpectrumAnalyzerComponent::getRenderArea()
{
	auto area = getLocalBounds();

	area.removeFromTop(11);
	area.removeFromBottom(-16);
	area.removeFromRight(20);
	area.removeFromLeft(20);

	return area;
}

juce::Rectangle<int> SpectrumAnalyzerComponent::getAnalysisArea()
{
	auto area = getRenderArea();

	area.removeFromTop(4);
	area.removeFromBottom(4);

	return area;
}

void SpectrumAnalyzerComponent::mouseDown(const juce::MouseEvent& e)
{
	clicked = true;
	repaint();
}

void SpectrumAnalyzerComponent::timerCallback()
{
	if (nextFFTBlockReady)
	{
		drawNextFrameOfSpectrum();
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

void SpectrumAnalyzerComponent::drawNextFrameOfSpectrum()
{

	window.multiplyWithWindowingTable(fftData, fftSize);

	forwardFFT.performFrequencyOnlyForwardTransform(fftData);

	auto mindB = mindBValue;//-100.0f
	auto maxdB = maxdBValue;//0.0f

	for (int i = 0; i < scopeSize; ++i)
	{
		auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * skewedYKnobRMS);//0.2f
		auto fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionX * (float)fftSize * dataIndexKnob));//0.5f
		auto level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(fftData[fftDataIndex]) - juce::Decibels::gainToDecibels((float)fftSize)),
			mindB, maxdB, 0.0f, lvlOffsetRMS);//1.0f

		scopeData[i] = level;
	}

}

void SpectrumAnalyzerComponent::drawFrame(juce::Graphics& g)
{
	for (int i = 1; i < scopeSize; ++i)
	{
		auto width = getLocalBounds().getWidth();
		auto height = getLocalBounds().getHeight();

		g.drawLine({ (float)juce::jmap(i - 1, 0, scopeSize - 1, 0, width),
						   juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
					(float)juce::jmap(i, 0, scopeSize - 1, 0, width),
						   juce::jmap(scopeData[i], 0.0f, 1.0f, (float)height, 0.0f) });
	}
}
