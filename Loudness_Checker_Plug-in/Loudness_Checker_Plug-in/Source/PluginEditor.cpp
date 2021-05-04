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
    : AudioProcessorEditor (&p), audioProcessor (p)
{
	setOpaque(true);

	mindBSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
	mindBSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 50);
	addAndMakeVisible(mindBSlider);

	maxdBSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
	maxdBSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 50);
	addAndMakeVisible(maxdBSlider);

	using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	mindBSliderAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "MINDBKNOWRMS", mindBSlider);
	maxdBSliderAttachment = std::make_unique<Attachment>(audioProcessor.apvts, "MAXDBKNOBRMS", maxdBSlider);

	addAndMakeVisible(&mySpectrAnComp);
	addAndMakeVisible(&mySpectrRep);

    setSize (700, 500);
}

Loudness_Checker_PluginAudioProcessorEditor::~Loudness_Checker_PluginAudioProcessorEditor()
{
}

//==============================================================================
void Loudness_Checker_PluginAudioProcessorEditor::paint (juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkgrey);

}

void Loudness_Checker_PluginAudioProcessorEditor::resized()
{

	//const auto paddingX = 5;
	//const auto paddingY = 35;
	//const auto paddingY2 = 300;
	//const auto width = 300;
	//const auto height = 200;
	
	//mySpectrAnComp.setBounds(paddingX, paddingY, getWidth(), height);
	//mySpectrRep.setBounds(paddingX, paddingY2, getWidth(), height);
	//mySpectrRep.setBounds(0, 0, getWidth(), getHeight());

	//auto area = getLocalBounds();

	//mySpectrAnComp.setBounds(area.removeFromTop(height));
	//mySpectrRep.setBounds(area.removeFromBottom(height));

	juce::Grid grid;

	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;

	grid.templateRows = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };
	grid.templateColumns = { Track(Fr(1)), Track(Fr(1)) };

	grid.items = { juce::GridItem(mySpectrAnComp), juce::GridItem(mindBSlider), juce::GridItem(maxdBSlider), juce::GridItem(mySpectrRep) };

	grid.performLayout(getLocalBounds());
}

void Loudness_Checker_PluginAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
	isClicked = true;
	repaint();
}

void Loudness_Checker_PluginAudioProcessorEditor::printRMS(juce::Grid grid)
{
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;
	
	grid.templateRows = { Track(Fr(1)) };
	grid.templateColumns = { Track(Fr(1)) };

	grid.items = { juce::GridItem(mySpectrAnComp) };

	grid.performLayout(getLocalBounds());
}

void Loudness_Checker_PluginAudioProcessorEditor::printSpectr(juce::Grid grid)
{
	using Track = juce::Grid::TrackInfo;
	using Fr = juce::Grid::Fr;

	grid.templateRows = { Track(Fr(1)) };
	grid.templateColumns = { Track(Fr(1)) };

	grid.items = { juce::GridItem(mySpectrRep) };

	grid.performLayout(getLocalBounds());
}
