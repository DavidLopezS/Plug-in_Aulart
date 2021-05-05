/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI\SpectrumAnalyzerComponent.h"
#include "UI\SpectrogramRepresentation.h"


//==============================================================================
/**
*/
class Loudness_Checker_PluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    Loudness_Checker_PluginAudioProcessorEditor (Loudness_Checker_PluginAudioProcessor&);
    ~Loudness_Checker_PluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
	void mouseDown(const juce::MouseEvent&) override;

	SpectrumAnalyzerComponent mySpectrAnComp;
	SpectrogramRepresentation mySpectrRep;

private:
	
	bool isClicked = true;

	using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	std::unique_ptr<Attachment> mindBSliderAttachment;
	std::unique_ptr<Attachment> maxdBSliderAttachment;
	std::unique_ptr<Attachment> lvlKnobSpectrAttachment;
	std::unique_ptr<Attachment> skPropSpectrAttachment;
	std::unique_ptr<Attachment> lvlOffSpectrAttachment;
	
    Loudness_Checker_PluginAudioProcessor& audioProcessor;

	struct KnobManager : public Component
	{
		KnobManager(juce::Colour c) : backgroundColour(c)
		{
			for(int i = 0; i < 5; ++i)
			{
				auto* knobSlider = new juce::Slider();
				knobSlider->setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
				knobSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 50);
				addAndMakeVisible(myKnobs.add(knobSlider));
			}
		}

		void paint (juce::Graphics& g) override
		{
			g.fillAll(backgroundColour);
		}

		void resized() override
		{
			juce::FlexBox knobBox;
			knobBox.flexWrap = juce::FlexBox::Wrap::wrap;
			knobBox.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

			for (auto *k : myKnobs)
				knobBox.items.add(juce::FlexItem(*k).withMinHeight(50.0f).withMinWidth(50.0f).withFlex(1));

			//----------------------------------------------------------------------------------------------

			juce::FlexBox fb;
			fb.flexDirection = juce::FlexBox::Direction::row;
			
			fb.items.add(juce::FlexItem(knobBox).withFlex(2.5f));

			fb.performLayout(getLocalBounds().toFloat());
		}

		juce::Colour backgroundColour;
		juce::OwnedArray<juce::Slider> myKnobs;
	};

	KnobManager mydBKnobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Loudness_Checker_PluginAudioProcessorEditor)
};
