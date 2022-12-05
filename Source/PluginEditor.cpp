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
    hid_init();

    addAndMakeVisible(hidSelector);
    hidSelector.addMouseListener(this, true);
    hidSelector.addListener(this);

    addAndMakeVisible(hidText);

    addAndMakeVisible(outText);

    setSize (800, 600);

    getLocalBounds();
}

JoyconGoodnessAudioProcessorEditor::~JoyconGoodnessAudioProcessorEditor()
{
    if (nullptr != joycon)
        delete joycon;
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
        auto info = hidDevies[(u_long)hidSelector.getSelectedItemIndex()];

        if (true == audioProcessor.setHidDevice(info))
        {
            hidText.setButtonText(juce::String{info.product_string});
            joycon = audioProcessor.getJoycon();
            startTimer(10);
        }
    }
}
