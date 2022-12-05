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

    void timerCallback() override
    {
        if (nullptr != audioProcessor.getJoycon())
        {
            juce::String str = "";
            str += "pitch: " + juce::String(audioProcessor.getJoycon()->getPitchRollYaw().x, 2) + " ";
            str += "roll: " + juce::String(audioProcessor.getJoycon()->getPitchRollYaw().y, 2) + " ";
            str += "yaw: " + juce::String(audioProcessor.getJoycon()->getPitchRollYaw().z, 2) + " ";
            outText.setButtonText(str);
        }
    }

    void joyconAttached()
    {
        if (nullptr != audioProcessor.getJoycon())
        {
            hidText.setButtonText(juce::String{audioProcessor.getHidDeviceInfo()->product_string});
            startTimer(10);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JoyconGoodnessAudioProcessorEditor)
};
