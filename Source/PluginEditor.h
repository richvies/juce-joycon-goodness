/*
    ==============================================================================

        This file contains the basic framework code for a JUCE plugin editor.

    ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class JoyconGoodnessAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::ComboBox::Listener, public juce::Timer
{
public:
    JoyconGoodnessAudioProcessorEditor (JoyconGoodnessAudioProcessor&);
    ~JoyconGoodnessAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void mouseDown (const MouseEvent& event) override;
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;

    JoyconGoodnessAudioProcessor& audioProcessor;

    juce::ComboBox hidSelector;
    juce::TextButton hidText;
    juce::TextButton outText;
    std::vector<hid_device_info> hidDevies;
    Joycon* joycon = nullptr;

    void timerCallback() override
    {
        if (nullptr != joycon)
        {
            juce::String str = "";
            str += "pitch: " + juce::String(joycon->getPitchRollYaw().x, 2) + " ";
            str += "roll: " + juce::String(joycon->getPitchRollYaw().y, 2) + " ";
            str += "yaw: " + juce::String(joycon->getPitchRollYaw().z, 2) + " ";
            outText.setButtonText(str);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JoyconGoodnessAudioProcessorEditor)
};
