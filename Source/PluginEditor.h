/*
    ==============================================================================

        This file contains the basic framework code for a JUCE plugin editor.

    ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "hidapi.h"

//==============================================================================
/**
*/
class JoyconGoodnessAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::ComboBox::Listener
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
    void JoyconSubcmd(uint8 sc, uint8 *buf, uint8 len, bool print = true);
    bool Attach(uint8 leds_ = 0x0);

    JoyconGoodnessAudioProcessor& audioProcessor;

    juce::ComboBox hidSelector;
    hid_device* hidDev;
    std::vector<hid_device_info> hidInfo;
    juce::TextButton hidText;
    juce::TextButton hidDbg;
    uint8 global_count;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JoyconGoodnessAudioProcessorEditor)
};
