/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Loudness_MeterAudioProcessor::Loudness_MeterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParams()), fFft(fftSize), 
						  fWindow(fFft.getSize() + 1, juce::dsp::WindowingFunction<float>::hann, false)
#endif
{
}

Loudness_MeterAudioProcessor::~Loudness_MeterAudioProcessor()
{
}

//==============================================================================
const juce::String Loudness_MeterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Loudness_MeterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Loudness_MeterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Loudness_MeterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Loudness_MeterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Loudness_MeterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Loudness_MeterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Loudness_MeterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Loudness_MeterAudioProcessor::getProgramName (int index)
{
    return {};
}

void Loudness_MeterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Loudness_MeterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	leftChannelFifo.prepare(samplesPerBlock);
	rightChannelFifo.prepare(samplesPerBlock);

	spectrChannelFifo.prepare(samplesPerBlock);
}

void Loudness_MeterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Loudness_MeterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Loudness_MeterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();


    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	leftChannelFifo.update(buffer);
	rightChannelFifo.update(buffer);

	spectrChannelFifo.update(buffer);

	//auto hopSize = buffer.getNumSamples() / 2;

	if (buffer.getNumChannels() > 0)
	{
		const float* channelData = buffer.getReadPointer(0);
		for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
			pushNextSampleIntoFifo(channelData[sample]);

		STFT(buffer, buffer.getNumSamples() / 2);
	}

	auto &graftOutputType = *apvts.getRawParameterValue("GRAFTYPE");
	auto &orderSwitch = *apvts.getRawParameterValue("ORDERSWITCH");
	auto &colourGridSwitch = *apvts.getRawParameterValue("COLOURGRIDSWITCH");
	auto &rmsLevelOffset = *apvts.getRawParameterValue("RMSLINEOFFSET");
	auto &genreSelectorSpectr = *apvts.getRawParameterValue("GENRE");	
	auto &genreSelectorRMS = *apvts.getRawParameterValue("GENRERMS");
	auto &lvlKnobSpectr = *apvts.getRawParameterValue("LVLKNOBSPECTR");
	auto &skPropSpectr = *apvts.getRawParameterValue("SKEWEDPROPYSPECTR");	
	auto &lvlOffSpectr = *apvts.getRawParameterValue("LVLOFFSETSPECTR");

	auto myRepData = dynamic_cast<Loudness_MeterAudioProcessorEditor*>(getActiveEditor());
	if (myRepData != nullptr)
	{
		myRepData->gridRepresentation.selGrid(graftOutputType);
		myRepData->gridRepresentation.changeRMSOffset(rmsLevelOffset);
		myRepData->gridRepresentation.pathOrderChoice(orderSwitch);
		myRepData->gridRepresentation.switchSpectrParams(lvlKnobSpectr, skPropSpectr, lvlOffSpectr);
		myRepData->gridRepresentation.switchSpectrogram(genreSelectorSpectr);
		myRepData->gridRepresentation.switchRMS(genreSelectorRMS);
	}
}

void Loudness_MeterAudioProcessor::pushNextSampleIntoFifo(float sample) noexcept
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

void Loudness_MeterAudioProcessor::STFT(const juce::AudioSampleBuffer & signal, size_t hop)
{
	//using Spectrum = std::vector<float>;

	//const float * data = signal.getReadPointer(0);
	//const size_t dataCount = signal.getNumSamples();

	//ptrdiff_t fftSize = fFft.getSize();

	//ptrdiff_t numHops = 1 + static_cast<long>((dataCount - fftSize) / hop);

	//size_t numRows = 1 + (fftSize / 2);

	//std::vector<float> fftBuffer(fftSize * 2);

	//while(data > 0)
	//{
	//
	//	memcpy(fftBuffer.data(), data, fftSize * sizeof(float));

	//	fWindow.multiplyWithWindowingTable(fftBuffer.data(), fftSize);	
	//	fFft.performFrequencyOnlyForwardTransform(fftBuffer.data());



	//	data += hop;
	//}
}

//==============================================================================
bool Loudness_MeterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Loudness_MeterAudioProcessor::createEditor()
{
    return new Loudness_MeterAudioProcessorEditor (*this);
}

//==============================================================================
void Loudness_MeterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Loudness_MeterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Loudness_MeterAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout Loudness_MeterAudioProcessor::createParams()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	//Representation Switch
	params.push_back(std::make_unique<juce::AudioParameterChoice>("GRAFTYPE", "Graf Type", juce::StringArray{ "RMS", "Spectrogram" }, 1));

	//Order Switch
	params.push_back(std::make_unique<juce::AudioParameterChoice>("ORDERSWITCH", "Order Switch", juce::StringArray{ "Order 2048", "Order 4096", "Order 8192" }, 0));

	//Grid Colour Swap
	params.push_back(std::make_unique<juce::AudioParameterChoice>("COLOURGRIDSWITCH", "Colour Grid Switch", juce::StringArray{ "Green", "Red", "Blue" }, 0));

	//Genre Selector
	params.push_back(std::make_unique <juce::AudioParameterChoice>("GENRE", "Genre", juce::StringArray{ "Spectrogram", "Techno", "House", "IDM", "EDM", "Downtempo" }, 0));	//Genre Selector
	
	//Genre Selector
	params.push_back(std::make_unique <juce::AudioParameterChoice>("GENRERMS", "Genre RMS", juce::StringArray{ "RMS", "Techno", "House", "IDM", "EDM", "Downtempo" }, 0));

	//Lvl Knob Spectrogram
	params.push_back(std::make_unique<juce::AudioParameterFloat>("LVLKNOBSPECTR", "Level Knob Spectrogram", juce::NormalisableRange<float>{0.00001f, 15.0f, 0.1f}, 4.1f));

	//Skewed Proportion Y Spectrogram
	params.push_back(std::make_unique<juce::AudioParameterFloat>("SKEWEDPROPYSPECTR", "Skewed Proportion Y Spectrogram", juce::NormalisableRange<float>{0.1f, 1.0f, 0.1f}, 0.4f));

	//Level Offset Spectrogram
	params.push_back(std::make_unique<juce::AudioParameterFloat>("LVLOFFSETSPECTR", "Level Offset Spectrogram", juce::NormalisableRange<float>{0.0f, 50.0f, 1.f}, 3.9f));

	//RMS Line Offset
	params.push_back(std::make_unique<juce::AudioParameterFloat>("RMSLINEOFFSET", "RMS Line Offser", juce::NormalisableRange<float>{-200.0f, -1.0f, 1.0f}, -48.0f));

	return{ params.begin(), params.end() };

}