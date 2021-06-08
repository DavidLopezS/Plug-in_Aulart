/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder
{
	order2048 = 11,
	order4096 = 12,
	order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
	/**
	 produces the FFT data from an audio buffer.
	 */
	void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
	{
		const auto fftSize = getFFTSize();

		fftData.assign(fftData.size(), 0);
		auto* readIndex = audioData.getReadPointer(0);
		std::copy(readIndex, readIndex + fftSize, fftData.begin());

		// first apply a windowing function to our data
		window->multiplyWithWindowingTable(fftData.data(), fftSize);       

		// then render our FFT data..
		forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());  

		int numBins = (int)fftSize / 2;

		//normalize the fft values.
		for (int i = 0; i < numBins; ++i)
		{
			auto v = fftData[i];
			//            fftData[i] /= (float) numBins;
			if (!std::isinf(v) && !std::isnan(v))
			{
				v /= float(numBins);
			}
			else
			{
				v = 0.f;
			}
			fftData[i] = v;
		}

		//convert them to decibels
		for (int i = 0; i < numBins; ++i)
		{
			fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
		}

		fftDataFifo.push(fftData);
	}

	void changeOrder(FFTOrder newOrder)
	{
		//when you change order, recreate the window, forwardFFT, fifo, fftData
		//also reset the fifoIndex
		//things that need recreating should be created on the heap via std::make_unique<>

		order = newOrder;
		auto fftSize = getFFTSize();

		forwardFFT = std::make_unique<juce::dsp::FFT>(order);
		window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

		fftData.clear();
		fftData.resize(fftSize * 2, 0);

		fftDataFifo.prepare(fftData.size());
	}
	//==============================================================================
	int getFFTSize() const { return 1 << order; }
	int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
	//==============================================================================
	bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
	FFTOrder order;
	BlockType fftData;
	std::unique_ptr<juce::dsp::FFT> forwardFFT;
	std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

	Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
	/*
	 converts 'renderData[]' into a juce::Path
	 */
	void generatePath(const std::vector<float>& renderData,
		juce::Rectangle<float> fftBounds, int fftSize, float binWidth, float negativeInfinity)
	{
		auto top = fftBounds.getY();
		auto bottom = fftBounds.getHeight();
		auto width = fftBounds.getWidth();

		int numBins = (int)fftSize / 2;

		PathType p;
		p.preallocateSpace(3 * (int)fftBounds.getWidth());

		auto map = [bottom, top, negativeInfinity](float v)
		{
			return juce::jmap(v,
				negativeInfinity, 0.f,
				float(bottom + 10), top);
		};

		auto y = map(renderData[0]);

		if (std::isnan(y) || std::isinf(y))
			y = bottom;

		p.startNewSubPath(0, y);

		const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

		for (int binNum = 1; binNum < numBins; binNum += pathResolution)
		{
			y = map(renderData[binNum]);

			if (!std::isnan(y) && !std::isinf(y))
			{
				auto binFreq = binNum * binWidth;
				auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
				int binX = std::floor(normalizedBinX * width);
				p.lineTo(binX, y);
			}
		}

		pathFifo.push(p);
	}

	int getNumPathsAvailable() const
	{
		return pathFifo.getNumAvailableForReading();
	}

	bool getPath(PathType& path)
	{
		return pathFifo.pull(path);
	}
private:
	Fifo<PathType> pathFifo;
};

struct PathProducer
{
public:

	PathProducer(SingleChannelSampleFifo<juce::AudioBuffer<float>>& scsf) : leftChannelFifo(&scsf), offsetRMS(-48.0f), orderChoice(0)
	{
		//48000 / 2048 = 23hz, a lot of resolution in the upper end, not a lot in the bottom
		switch (orderChoice)
		{
		case 0:
			leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
			break;
		case 1:
			leftChannelFFTDataGenerator.changeOrder(FFTOrder::order4096);
			break;
		case 2:
			leftChannelFFTDataGenerator.changeOrder(FFTOrder::order8192);
			break;
		default:
			jassertfalse;
			break;
		}
		monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
	}

	void process(juce::Rectangle<float> fftBounds, double sampleRate);
	juce::Path getPath() { return leftChannelFFTPath; }

	int orderChoice;
	float offsetRMS;

private:

	using BlockType = juce::AudioBuffer<float>;
	SingleChannelSampleFifo<BlockType>* leftChannelFifo;

	juce::AudioBuffer<float> monoBuffer;

	FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;

	AnalyzerPathGenerator<juce::Path> pathProducer;

	juce::Path leftChannelFFTPath;
};

struct SpectrogramAndRMSRep : public juce::Component, private juce::Timer
{
public:

	SpectrogramAndRMSRep(Loudness_MeterAudioProcessor& p) : audioPrc(p), 
															forwardFFT(audioPrc.fftOrder), spectrogramImage(juce::Image::ARGB, 512, 512, true),
															leftPathProducer(audioPrc.leftChannelFifo), rightPathProducer(audioPrc.rightChannelFifo)
	{
		startTimerHz(30);//30
	}

	~SpectrogramAndRMSRep()
	{
		stopTimer();
		myBackgroundsSpectr.clear();
		myBackgroundsRMS.clear();
	}

	void paint(juce::Graphics& g) override
	{
		g.fillAll(juce::Colours::black);

		auto responseAreaRMS = getAnalysisAreaRMS();

		auto responseAreaSpectr = getAnalysisAreaSpectr();

		g.setOpacity(1.0f);

		if (isRMS)
		{

			for each(auto backround in myBackgroundsRMS)
			{
				if (rmsGridChoice == backround.first)
					g.drawImage(backround.second, getLocalBounds().toFloat());
			}

			auto leftChannelFFTPath = leftPathProducer.getPath();
			auto rightChannelFFTPath = rightPathProducer.getPath();

			leftChannelFFTPath.applyTransform(juce::AffineTransform().translation(responseAreaRMS.getX(), -10.0f));//-10.0f
			rightChannelFFTPath.applyTransform(juce::AffineTransform().translation(responseAreaRMS.getX(), -10.0f));//-10.0f

			g.setColour(juce::Colours::white);
			g.strokePath(leftChannelFFTPath, juce::PathStrokeType(1.0f));

			g.setColour(juce::Colours::skyblue);
			g.strokePath(rightChannelFFTPath, juce::PathStrokeType(1.0f));

			g.setColour(juce::Colours::orange);
			g.drawRoundedRectangle(responseAreaRMS.toFloat(), 4.0f, 1.0f);
		}
		else
		{
			g.drawImage(spectrogramImage, responseAreaSpectr.toFloat());

			for each(auto background in myBackgroundsSpectr)
			{
				if (spectrGridChoice == background.first)
				{
					g.drawImage(background.second, getLocalBounds().toFloat());
				}
			}
		}

		g.clipRegionIntersects(getLocalBounds());
	}

	void resized() override
	{	
		//RMS area spaces 
		auto renderAreaRMS = getAnalysisAreaRMS();
		auto leftRMS = renderAreaRMS.getX();
		auto rightRMS = renderAreaRMS.getRight();
		auto topRMS = renderAreaRMS.getY();
		auto bottomRMS = renderAreaRMS.getHeight();
		auto widthRMS = renderAreaRMS.getWidth();
		
		//Line Colour
		auto myColour = juce::Colour(255u, 41u, 41u);//Red

		juce::Array<float> myFreqRMSArray[] =
		{
			{ 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f },
			{ 20.f, 60.f, 300.f, 400.f, 600.f, 2000.f, 3000.f, 6000.f, 9000.f, 20000.f },
			{ 20.f, 20000.f },
			{ 20.f, 50.f, 500.f, 5000.f, 20000.f },
			{ 20.f, 100.f, 1000.f, 10000.f, 20000.f },
			{ 20.f, 200.f, 2000.f, 20000.f }
		};

		//Gain Array
		juce::Array<float> gain
		{
			-24.f, -12.f, 0.f, 12.f, 24.f
		};

		myBackgroundsRMS =
		{
			{0, juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true)},
			{1, juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true)},
			{2, juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true)},
			{3, juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true)},
			{4, juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true)},
			{5, juce::Image(juce::Image::PixelFormat::RGB, getWidth(), getHeight(), true)}
		};

		juce::Array<float> dbToBeColoredRMS =
		{
			500.f, 9000.f, 20000.f, 5000.f, 1000.f, 2000.f
		};

		for each (auto background in myBackgroundsRMS)
			RMSGrid(myFreqRMSArray[background.first], gain, background.second, renderAreaRMS, leftRMS, rightRMS, topRMS, bottomRMS, widthRMS, myColour, dbToBeColoredRMS[background.first]);

		//Spectr area spaces 
		auto renderAreaSpectr = getAnalysisAreaSpectr();
		auto leftSpectr = renderAreaSpectr.getX();
		auto rightSpectr = renderAreaSpectr.getRight();
		auto topSpectr = renderAreaSpectr.getY();
		auto bottomSpectr = renderAreaSpectr.getHeight();
		auto widthSpectr = renderAreaSpectr.getWidth();

		//Frequencies Array Spectr
		juce::Array<float> myFreqSpectrArray[] =  
		{
			{ 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f },
			{ 20.f, 60.f, 300.f, 400.f, 600.f, 2000.f, 3000.f, 6000.f, 9000.f, 20000.f },
			{ 20.f, 20000.f },
			{ 20.f, 50.f, 500.f, 5000.f, 20000.f },
			{ 20.f, 100.f, 1000.f, 10000.f, 20000.f },
			{ 20.f, 200.f, 2000.f, 20000.f }
		};

		myBackgroundsSpectr =
		{
			{0, juce::Image(juce::Image::PixelFormat::ARGB, getWidth(), getHeight(), true)},
			{1, juce::Image(juce::Image::PixelFormat::ARGB, getWidth(), getHeight(), true)},
			{2, juce::Image(juce::Image::PixelFormat::ARGB, getWidth(), getHeight(), true)},
			{3, juce::Image(juce::Image::PixelFormat::ARGB, getWidth(), getHeight(), true)},
			{4, juce::Image(juce::Image::PixelFormat::ARGB, getWidth(), getHeight(), true)},
			{5, juce::Image(juce::Image::PixelFormat::ARGB, getWidth(), getHeight(), true)}
		};

		juce::Array<float> dbToBeColoredSpectr = 
		{
			500.f, 9000.f, 20000.f, 5000.f, 1000.f, 2000.f
		};

		for each(auto background in myBackgroundsSpectr)
			spectrGrid(myFreqSpectrArray[background.first], background.second, renderAreaSpectr, leftSpectr, rightSpectr, topSpectr, bottomSpectr, widthSpectr, myColour, dbToBeColoredSpectr[background.first]);
	}

	void RMSGrid(juce::Array<float> freqRMS, juce::Array<float> gain, juce::Image imageRMS, juce::Rectangle<int> renderAreaRMS, int leftRMS, int rightRMS, int topRMS, int bottomRMS, int widthRMS, juce::Colour myColour, float numToBeColored)
	{
		juce::Graphics gRMS(imageRMS);

		//lines representation
		juce::Array<float> xPos;
		for (auto f : freqRMS)
		{
			auto normX = juce::mapFromLog10(f, 20.0f, 20000.0f);
			xPos.add(leftRMS + widthRMS * normX);
		}

		for (auto x : xPos)
		{
			auto myNumberToBeColored = leftRMS + widthRMS * juce::mapFromLog10(numToBeColored, 20.0f, 20000.f);
			gRMS.setColour(x == myNumberToBeColored ? myColour : juce::Colours::dimgrey);
			gRMS.drawVerticalLine(x, topRMS, bottomRMS);
		}

		for (auto gDb : gain)
		{
			auto y = juce::jmap(gDb, -24.0f, 24.0f, float(bottomRMS), float(topRMS));
			gRMS.setColour(gDb == 0.0f ? myColour : juce::Colours::darkgrey);
			gRMS.drawHorizontalLine(y, leftRMS, rightRMS);
		}

		//Values representation
		gRMS.setColour(juce::Colours::lightgrey);
		const int fontHeightRMS = 10;
		gRMS.setFont(fontHeightRMS);

		for (int i = 0; i < freqRMS.size(); ++i)
		{
			auto f = freqRMS[i];
			auto x = xPos[i];

			gRMS.setColour(f == numToBeColored ? myColour : juce::Colours::lightgrey);

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

			auto textWidth = gRMS.getCurrentFont().getStringWidth(str);

			juce::Rectangle<int> r;
			r.setSize(textWidth, fontHeightRMS);
			r.setCentre(x, 0);
			r.setY(1);

			gRMS.drawFittedText(str, r, juce::Justification::centred, 1);
		}

		for (auto gDb : gain)
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

			gRMS.setColour(gDb == 0.0f ? myColour : juce::Colours::lightgrey);

			gRMS.drawFittedText(str, r, juce::Justification::centred, 1);

			str.clear();
			str << (gDb - 24.0f);

			r.setX(1);
			textWidth = gRMS.getCurrentFont().getStringWidth(str);
			r.setSize(textWidth, fontHeightRMS);

			gRMS.drawFittedText(str, r, juce::Justification::centred, 1);
		}
	}

	void spectrGrid(juce::Array<float> freqSpectr, juce::Image spectrImage, juce::Rectangle<int> renderAreaSpectr, int leftSpectr, int rightSpectr, int topSpectr, int bottomSpectr, int widthSpectr, juce::Colour myColour, float numToBeColored)
	{
		juce::Graphics gSpectr(spectrImage);

		//Values representation
		gSpectr.setColour(juce::Colours::lightgrey);
		const int fontHeight = 10;
		gSpectr.setFont(fontHeight);

		juce::Array<float> yPos;
		for(auto fLines : freqSpectr)
		{
			auto normY = juce::mapFromLog10(fLines, 20000.0f, 20.0f);
			yPos.add(topSpectr + bottomSpectr * normY);
		}

		for(auto y : yPos)
		{
			auto myNumberToBeColored = topSpectr + bottomSpectr * juce::mapFromLog10(numToBeColored, 20000.0f, 20.0f);
			gSpectr.setColour(y == myNumberToBeColored ? myColour : juce::Colours::lightgrey);
			gSpectr.drawHorizontalLine(y, leftSpectr, rightSpectr);
		}

		for (int i = 0; i < freqSpectr.size(); ++i)
		{
			auto f = freqSpectr[i];
			auto y = yPos[i];

			gSpectr.setColour(f == numToBeColored ? myColour : juce::Colours::lightgrey);

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

			gSpectr.drawFittedText(str, r, juce::Justification::centred, 1);
		}

	}

	void timerCallback() override
	{
		auto fftBounds = getAnalysisAreaRMS().toFloat();
		auto sampleRate = audioPrc.getSampleRate();

		leftPathProducer.process(fftBounds, sampleRate);
		rightPathProducer.process(fftBounds, sampleRate);

		repaint();

		if (audioPrc.nextFFTBlockReady)
		{
			drawNextLineOfSpectrogram();
			audioPrc.nextFFTBlockReady = false;
			repaint();
		}
	}

	void selGrid(const int choice)
	{
		switch (choice)
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

	juce::Rectangle<int> getRenderAreaRMS()
	{
		auto area = getLocalBounds();


		area.removeFromTop(15);
		area.removeFromBottom(0);
		area.removeFromRight(20);
		area.removeFromLeft(20);

		return area;
	}

	juce::Rectangle<int> getRenderAreaSpectr()
	{
		auto area = getLocalBounds();

		area.removeFromTop(0);
		area.removeFromBottom(-3);
		area.removeFromRight(31);
		area.removeFromLeft(0);

		return area;
	}

	juce::Rectangle<int> getAnalysisAreaRMS()
	{
		auto area = getRenderAreaRMS();

		area.removeFromTop(0);
		area.removeFromBottom(2);

		return area;
	}

	juce::Rectangle<int> getAnalysisAreaSpectr()
	{
		auto area = getRenderAreaSpectr();

		area.removeFromTop(4);
		area.removeFromBottom(4);//4

		return area;
	}

	void drawNextLineOfSpectrogram()
	{
		const int rightHandEdge = spectrogramImage.getWidth() - 1;
		const int imageHeight = spectrogramImage.getHeight();

		spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

		forwardFFT.performFrequencyOnlyForwardTransform(audioPrc.fftData);

		juce::Range<float> maxLevel = juce::FloatVectorOperations::findMinAndMax(audioPrc.fftData, audioPrc.fftSize / 2);
		if (maxLevel.getEnd() == 0.0f)
			maxLevel.setEnd(lvlKnobSpectr);//0.00001f

		for (int i = 1; i < imageHeight; ++i)
		{
			const float skewedProportionY = 1.0f - std::exp(std::log(i / (float)imageHeight) * skPropSpectr);//0.2f
			const int fftDataIndex = juce::jlimit(0, audioPrc.fftSize / 2, (int)(skewedProportionY * audioPrc.fftSize / 2));
			const float level = juce::jmap(audioPrc.fftData[fftDataIndex], 0.0f, maxLevel.getEnd(), 0.0f, lvlOffSpectr);//Original targetRangeMax = 3.9f, needs to be tweaked/tested

			spectrogramImage.setPixelAt(rightHandEdge, i, juce::Colour::fromHSL(level, 1.0f, level, 1.0f));//Colour::fromHSV
		}

	}

	void changeRMSOffset(const float myRMSOffset)
	{
		leftPathProducer.offsetRMS = myRMSOffset;
		rightPathProducer.offsetRMS = myRMSOffset;
	}

	void pathOrderChoice(const int choice)
	{
		leftPathProducer.orderChoice = choice;
		rightPathProducer.orderChoice = choice;
	}

	void switchSpectrParams(float lvlKnobSpectrVar, float skrPropSpectrVar, float lvlOffSpectrVar)
	{
		lvlKnobSpectr = lvlKnobSpectrVar;
		skPropSpectr = skrPropSpectrVar;
		lvlOffSpectr = lvlOffSpectrVar;
	}

	void switchSpectrogram(const int choice)
	{
		switch (choice)
		{
		case 0:
			lvlKnobSpectr = 0.00001f;
			skPropSpectr = 0.2f;
			lvlOffSpectr = 2.9f;
			spectrGridChoice = choice;
			break;
		case 1:
			lvlKnobSpectr = 4.1f;
			skPropSpectr = 0.4f;
			lvlOffSpectr = 3.9f;
			spectrGridChoice = choice;
			break;
		case 2:
			lvlKnobSpectr = 2.1f;
			skPropSpectr = 1.5f;
			lvlOffSpectr = 5.5f;
			spectrGridChoice = choice;
			break;
		case 3:
			lvlKnobSpectr = 0.002f;
			skPropSpectr = 0.8f;
			lvlOffSpectr = 0.2f;
			spectrGridChoice = choice;
			break;
		case 4:
			lvlKnobSpectr = 0.05f;
			skPropSpectr = 2.5f;
			lvlOffSpectr = 4.0f;
			spectrGridChoice = choice;
			break;
		case 5:
			lvlKnobSpectr = 3.5f;
			skPropSpectr = 1.0f;
			lvlOffSpectr = 4.5f;
			spectrGridChoice = choice;
			break;
		default:
			jassertfalse;
			break;
		}
	}

	void switchRMS(const int choice)
	{
		switch(choice)
		{
		case 0:
			rmsGridChoice = choice;
			break;
		case 1:
			rmsGridChoice = choice;
			break;
		case 2:
			rmsGridChoice = choice;
			break;
		case 3:
			rmsGridChoice = choice;
			break;
		case 4:
			rmsGridChoice = choice;
			break;
		case 5:
			rmsGridChoice = choice;
			break;
		default:
			jassertfalse;
			break;
		}
	}

private:
	Loudness_MeterAudioProcessor& audioPrc;

	juce::dsp::FFT forwardFFT;
	juce::Image spectrogramImage;

	std::map<int, juce::Image> myBackgroundsSpectr;
	std::map<int, juce::Image> myBackgroundsRMS;

	PathProducer leftPathProducer, rightPathProducer;

	bool isRMS = false;
	int spectrGridChoice = 0;	
	int rmsGridChoice = 0;
	float lvlKnobSpectr = 0.00001f;
	float skPropSpectr = 0.2f;
	float lvlOffSpectr = 3.9f;
};

//==============================================================================

class Loudness_MeterAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    Loudness_MeterAudioProcessorEditor (Loudness_MeterAudioProcessor&);
    ~Loudness_MeterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
	void knobAttachment(int);
	void selectorAttachment(int);

	SpectrogramAndRMSRep gridRepresentation;

private:
	
    Loudness_MeterAudioProcessor& audioProcessor;

	//Knobs attachment
	static constexpr auto numKnobs = 4;

	juce::String myKnobName[numKnobs]
	{
		"LVLKNOBSPECTR", "SKEWEDPROPYSPECTR", "LVLOFFSETSPECTR", "RMSLINEOFFSET"
	};

	using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	std::vector<std::unique_ptr<Attachment>> myKnobAttachments;

	struct KnobManager : public Component
	{
		KnobManager(juce::Colour c) : backgroundColour(c)
		{
			for (int i = 0; i < numKnobs; ++i)
			{
				auto* knobSlider = new juce::Slider();
				knobSlider->setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
				knobSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 50);
				addAndMakeVisible(myKnobs.add(knobSlider));

				myLabels[i].setColour(juce::Label::ColourIds::textColourId, juce::Colours::ghostwhite);
				myLabels[i].setFont(20.0f);
				myLabels[i].setJustificationType(juce::Justification::centred);
				addAndMakeVisible(myLabels[i]);
			}
		}

		void paint(juce::Graphics& g) override
		{
			g.fillAll(backgroundColour);
		}

		void resized() override
		{
			juce::Rectangle<int> bounds = getLocalBounds();

			//Knob FlexBox system
			setFlexBoxKnob(bounds);

			//Label FlexBox system
			setFlexBoxLabel(bounds);
			
		}

		void setFlexBoxKnob(juce::Rectangle<int> bounds)
		{
			juce::FlexBox knobBox;
			knobBox.flexWrap = juce::FlexBox::Wrap::wrap;
			knobBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

			for (auto *k : myKnobs)
				knobBox.items.add(juce::FlexItem(*k).withMinHeight(50.0f).withMinWidth(50.0f).withFlex(1));

			juce::FlexBox fb;
			fb.flexDirection = juce::FlexBox::Direction::row;

			fb.items.add(juce::FlexItem(knobBox).withFlex(2.5f));

			fb.performLayout(bounds.removeFromBottom(120));
		}

		void setFlexBoxLabel(juce::Rectangle<int> bounds)
		{
			juce::FlexBox labelBox;
			labelBox.flexWrap = juce::FlexBox::Wrap::wrap;
			labelBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

			for (int i = 0; i < numKnobs; ++i)
				labelBox.items.add(juce::FlexItem(myLabels[i]).withMinHeight(50.0f).withMinWidth(50.0f).withFlex(1));

			juce::FlexBox fb2;
			fb2.flexDirection = juce::FlexBox::Direction::row;

			fb2.items.add(juce::FlexItem(labelBox).withFlex(2.5f));

			fb2.performLayout(bounds.removeFromTop(50));
		}

		juce::Colour backgroundColour;
		juce::OwnedArray<juce::Slider> myKnobs;
		juce::Label myLabels[numKnobs]
		{
			{"Level Knob Spectr", "Level Knob Spectr"}, {"SK Proportion Y Spectr", "SK Proportion Y Spectr"}, 
			{"Level Offset Spectr", "Level Offset Spectr"}, {"RMS Offset", "RMS Offset"}
		};
	};

	KnobManager mydBKnobs;
	
	//Spectr selector and attachment	
	static constexpr auto numSelectors = 5;

	juce::String mySelectorNames[numSelectors]
	{
		"GRAFTYPE", "ORDERSWITCH", "COLOURGRIDSWITCH", "GENRE", "GENRERMS"
	};

	using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
	std::vector<std::unique_ptr<ComboBoxAttachment>> mySelectorAttachments;

	struct SelectorManager : public Component
	{
		SelectorManager(juce::Colour c) : backgroundColour(c)
		{
			for(int i = 0; i < numSelectors; ++i)
			{
				auto* myComboBox = new juce::ComboBox();
				myComboBox->addItemList(choices[i], 1);
				addAndMakeVisible(myComboBoxes.add(myComboBox));
			}
		}

		void paint(juce::Graphics& g) override
		{
			g.fillAll(backgroundColour);
		}

		void resized() override
		{
			juce::Rectangle<int> bounds = getLocalBounds();

			juce::FlexBox comboFlexBox;
			comboFlexBox.flexWrap = juce::FlexBox::Wrap::wrap;
			comboFlexBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

			for (auto *cB : myComboBoxes)
				comboFlexBox.items.add(juce::FlexItem(*cB).withMinHeight(50.0f).withMinWidth(50.0f).withFlex(1.f));

			juce::FlexBox fb;
			fb.flexDirection = juce::FlexBox::Direction::row;

			fb.items.add(juce::FlexItem(comboFlexBox).withFlex(2.5f));

			fb.performLayout(bounds);
		}
		
		juce::Colour backgroundColour;
		juce::OwnedArray<juce::ComboBox> myComboBoxes;
		juce::StringArray choices[numSelectors]
		{
			{ "RMS", "Spectrogram" }, { "Order 2048", "Order 4096", "Order 8192" }, { "Green", "Red", "Blue" },
			{ "Spectrogram", "Techno", "House", "IDM", "EDM", "Downtempo" }, { "RMS", "Techno", "House", "IDM", "EDM", "Downtempo" }
		};
	};

	SelectorManager mySelectorManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Loudness_MeterAudioProcessorEditor)
};
