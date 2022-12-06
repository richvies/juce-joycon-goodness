/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JoyconGoodnessAudioProcessorEditor::JoyconGoodnessAudioProcessorEditor (JoyconGoodnessAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible(hidSelector);
    hidSelector.addMouseListener(this, true);
    hidSelector.addListener(this);

    addAndMakeVisible(hidText);

    addAndMakeVisible(outText);

    setSize (800, 600);

    getLocalBounds();

    joyconAttached();
}

JoyconGoodnessAudioProcessorEditor::~JoyconGoodnessAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void JoyconGoodnessAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
}

void JoyconGoodnessAudioProcessorEditor::resized()
{
    hidSelector.setBoundsRelative(.0f, .0f, .3f, .1f);
    hidText.setBoundsRelative(.3f, .0f, .3f, .1f);
    outText.setBoundsRelative(0.f, .2f, .3f, .1f);
}

void JoyconGoodnessAudioProcessorEditor::mouseDown (const MouseEvent& event)
{
    if (event.eventComponent == &hidSelector)
    {
        hidDevies = audioProcessor.getHidDevices();
        auto idx = 1;

        hidSelector.clear();
        for (auto iter = hidDevies.begin(); iter < hidDevies.end(); iter++)
        {
            hidSelector.addItem(juce::String{iter->product_string}, idx++);
        }
    }
}

void JoyconGoodnessAudioProcessorEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &hidSelector)
    {
        auto id = hidSelector.getSelectedId();

        if (id)
        {
            auto info = hidDevies[(u_long)id - 1];

            if (true == audioProcessor.setHidDevice(info))
            {
                joyconAttached();
            }
        }
    }
}
