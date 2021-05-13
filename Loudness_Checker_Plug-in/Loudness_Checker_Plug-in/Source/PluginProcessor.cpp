/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Loudness_Checker_PluginAudioProcessor::Loudness_Checker_PluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParams())
#endif
{
}

Loudness_Checker_PluginAudioProcessor::~Loudness_Checker_PluginAudioProcessor()
{
}

//==============================================================================
const juce::String Loudness_Checker_PluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Loudness_Checker_PluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Loudness_Checker_PluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Loudness_Checker_PluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Loudness_Checker_PluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Loudness_Checker_PluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Loudness_Checker_PluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Loudness_Checker_PluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Loudness_Checker_PluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void Loudness_Checker_PluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Loudness_Checker_PluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void Loudness_Checker_PluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Loudness_Checker_PluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void Loudness_Checker_PluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


	auto &graftOutputType = *apvts.getRawParameterValue("GRAFTYPE");
	auto &maxRMSdB = *apvts.getRawParameterValue("MAXDBKNOBRMS");
	auto &minRMSdB = *apvts.getRawParameterValue("MINDBKNOWRMS");
	auto &skPropRMS = *apvts.getRawParameterValue("SKEWEDPROPYRMS");
	auto &indexDataRMS = *apvts.getRawParameterValue("FFTDATAINDEXRMS");
	auto &lvlOffRMS = *apvts.getRawParameterValue("LVLOFFSETRMS");
	auto &lvlKnobSpectr = *apvts.getRawParameterValue("LVLKNOBSPECTR");
	auto &skPropSpectr = *apvts.getRawParameterValue("SKEWEDPROPYSPECTR");
	auto &lvlOffSpectr = *apvts.getRawParameterValue("LVLOFFSETSPECTR");

	auto mySpectrData = dynamic_cast<Loudness_Checker_PluginAudioProcessorEditor*>(getActiveEditor());
	if (mySpectrData != nullptr)
	{
		mySpectrData->mySpectrAnComp.processAudioBlock(buffer);//RMS buffer input
		mySpectrData->mySpectrRep.processAudioBlock(buffer);//Spectrogram buffer input
	
		mySpectrData->mySpectrAnComp.mindBValue = minRMSdB.load();
		mySpectrData->mySpectrAnComp.maxdBValue = maxRMSdB.load();
		mySpectrData->mySpectrAnComp.skewedYKnobRMS = skPropRMS.load();
		mySpectrData->mySpectrAnComp.dataIndexKnob = indexDataRMS.load();
		mySpectrData->mySpectrAnComp.lvlOffsetRMS = lvlOffRMS.load();
		mySpectrData->mySpectrRep.lvlKnobSpectr = lvlKnobSpectr.load();
		mySpectrData->mySpectrRep.skewedPropSpectr = skPropSpectr.load();
		mySpectrData->mySpectrRep.lvlOffsetSpectr = lvlOffSpectr.load();
	}
}

//==============================================================================
bool Loudness_Checker_PluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Loudness_Checker_PluginAudioProcessor::createEditor()
{
    return new Loudness_Checker_PluginAudioProcessorEditor (*this);
	//return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void Loudness_Checker_PluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Loudness_Checker_PluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Loudness_Checker_PluginAudioProcessor();
}

//Value Tree
juce::AudioProcessorValueTreeState::ParameterLayout Loudness_Checker_PluginAudioProcessor::createParams()
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	//Representation Switch
	params.push_back(std::make_unique<juce::AudioParameterChoice>("GRAFTYPE", "Graf Type", juce::StringArray{ "RMS", "Spectrogram" }, 0));

	//Max db Knob RMS
	params.push_back(std::make_unique<juce::AudioParameterFloat>("MAXDBKNOBRMS", "Max db Knob RMS", juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 0.0f));

	//Min db Knob RMS
	params.push_back(std::make_unique<juce::AudioParameterFloat>("MINDBKNOWRMS", "Min db Knob RMS", juce::NormalisableRange<float>{-100.0f, -1.0f, 0.1f}, -100.0f));

	//Skewed Proportion Y RMS
	params.push_back(std::make_unique<juce::AudioParameterFloat>("SKEWEDPROPYRMS", "Skewed Proportion Y RMS", juce::NormalisableRange<float>{0.1f, 1.0f, 0.1f}, 0.3f));

	//FFT Data Index RMS
	params.push_back(std::make_unique<juce::AudioParameterFloat>("FFTDATAINDEXRMS", "Fft Data Index RMS", juce::NormalisableRange<float>{0.1f, 1.0f, 0.1f}, 0.5f));

	//Level Offset RMS
	params.push_back(std::make_unique<juce::AudioParameterFloat>("LVLOFFSETRMS", "Level Offset RMS", juce::NormalisableRange<float>{0.0f, 5.0f, 0.1f}, 1.0f));

	//Lvl Knob Spectrogram
	params.push_back(std::make_unique<juce::AudioParameterFloat>("LVLKNOBSPECTR", "Level Knob Spectrogram", juce::NormalisableRange<float>{0.00001f, 5.0f, 0.1f}, 4.1f));

	//Skewed Proportion Y Spectrogram
	params.push_back(std::make_unique<juce::AudioParameterFloat>("SKEWEDPROPYSPECTR", "Skewed Proportion Y Spectrogram", juce::NormalisableRange<float>{0.1f, 1.0f, 0.1f}, 0.4f));

	//Level Offset Spectrogram
	params.push_back(std::make_unique<juce::AudioParameterFloat>("LVLOFFSETSPECTR", "Level Offset Spectrogram", juce::NormalisableRange<float>{0.0f, 5.0f, 0.1f}, 3.9f));

	return{ params.begin(), params.end() };

}