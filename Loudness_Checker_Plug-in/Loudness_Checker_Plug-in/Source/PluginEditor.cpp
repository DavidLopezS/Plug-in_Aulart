/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Loudness_Checker_PluginAudioProcessorEditor::Loudness_Checker_PluginAudioProcessorEditor (Loudness_Checker_PluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), mydBKnobs(juce::Colours::darkgrey)
{
	setOpaque(true);

	juce::StringArray choices{ "RMS", "Spectrogram" };
	spectrRMSSelector.addItemList(choices, 1);
	addAndMakeVisible(spectrRMSSelector);

	spectrRMSSelectorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "GRAFTYPE", spectrRMSSelector);

	addAndMakeVisible(&mydBKnobs);
	
	for(int i = 0; i < numKnobs; ++i)
		knobAttachment(i);

	addAndMakeVisible(&mySpectrAnComp);
	addAndMakeVisible(&mySpectrRep);

    setSize (900, 500); 
}

Loudness_Checker_PluginAudioProcessorEditor::~Loudness_Checker_PluginAudioProcessorEditor()
{
	myAttachments.clear();
}

//==============================================================================
void Loudness_Checker_PluginAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkgrey);
}

void Loudness_Checker_PluginAudioProcessorEditor::resized()
{

	//const auto paddingX = 5;
	//const auto paddingY = 35;
	//const auto paddingY2 = 300;
	//const auto width = 300;
	//const auto height = 175;
	
	//mySpectrAnComp.setBounds(paddingX, paddingY, getWidth(), height);
	//mySpectrRep.setBounds(paddingX, paddingY2, getWidth(), height);
	//mySpectrRep.setBounds(0, 0, getWidth(), getHeight());

	//auto area = getLocalBounds();

	//mySpectrAnComp.setBounds(area.removeFromTop(height));
	//mySpectrRep.setBounds(area.removeFromBottom(height));

	juce::Grid grid;

	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;

	grid.templateRows = { Track(Fr(2)), Track(Fr(1))/*, Track(Fr(2)), Track(Fr(1)) */};
	grid.templateColumns = { Track(Fr(1)) };

	grid.items = { juce::GridItem(mySpectrAnComp), juce::GridItem(spectrRMSSelector)/*, juce::GridItem(mySpectrRep), juce::GridItem(mydBKnobs)*/ };

	grid.performLayout(getLocalBounds());
}

void Loudness_Checker_PluginAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
	isClicked = true;
	repaint();
	mySpectrAnComp.repaint();
	mySpectrRep.repaint();
}

void Loudness_Checker_PluginAudioProcessorEditor::knobAttachment(int knobId)
{
	auto &myKnob = *mydBKnobs.myKnobs[knobId];

	using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	myAttachments.push_back(std::make_unique<Attachment>(audioProcessor.apvts, myKnobName[knobId], myKnob));

	//myKnobLabels[knobId].setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
	//myKnobLabels[knobId].setFont(15.0f);
	//myKnobLabels[knobId].setJustificationType(juce::Justification::centred);
	//addAndMakeVisible(myKnobLabels[knobId]);
}
