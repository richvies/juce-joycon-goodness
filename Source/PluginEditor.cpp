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
    global_count = 0;

    addAndMakeVisible(hidSelector);
    hidSelector.addMouseListener(this, true);
    hidSelector.addListener(this);

    addAndMakeVisible(hidText);

    setSize (800, 600);
}

JoyconGoodnessAudioProcessorEditor::~JoyconGoodnessAudioProcessorEditor()
{
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
}

void JoyconGoodnessAudioProcessorEditor::mouseDown (const MouseEvent& event)
{
    if (event.eventComponent == &hidSelector)
    {
        auto info = hid_enumerate(0, 0);

        hidSelector.clear();
        hidInfo.clear();

        while (nullptr != info)
        {
            juce::String str(info->product_string);
            if (str.isNotEmpty() && (info->vendor_id == 0x057e))
            {
                hidInfo.push_back(*info);
            }
            info = info->next;
        }
        hid_free_enumeration(info);

        auto idx = 1;
        for (auto iter = hidInfo.begin(); iter < hidInfo.end(); iter++)
        {
            hidSelector.addItem(juce::String{iter->product_string}, idx++);
        }
    }
}

void JoyconGoodnessAudioProcessorEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &hidSelector)
    {
        auto info = &hidInfo[(u_long)hidSelector.getSelectedItemIndex()];
        auto dev = hid_open(info->vendor_id, info->product_id, nullptr);

        if (nullptr != dev)
        {
            hidText.setButtonText(juce::String{info->product_string});

            if (joycon != nullptr)
            {
                joycon->Detach();
                delete joycon;
            }

            joycon = new Joycon(dev, true, true, 0.05f, (info->product_id == Joycon::product_id_left) ? true : false);
            if (true == joycon->Attach())
            {
                joycon->Begin();
            }
        }
    }
}
