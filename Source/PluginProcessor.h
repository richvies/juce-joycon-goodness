/*
    ==============================================================================

        This file contains the basic framework code for a JUCE plugin processor.

    ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "hidapi.h"
#include "joycon.hpp"

//==============================================================================
/**
*/
class JoyconGoodnessAudioProcessor  : public juce::AudioProcessor, public juce::Timer
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    JoyconGoodnessAudioProcessor();
    ~JoyconGoodnessAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    std::vector<hid_device_info> getHidDevices(void)
    {
        auto info = hid_enumerate(0, 0);
        std::vector<hid_device_info> devices;

        while (nullptr != info)
        {
            juce::String str(info->product_string);
            if (str.isNotEmpty() && (info->vendor_id == 0x057e))
            {
                devices.push_back(*info);
            }
            info = info->next;
        }

        hid_free_enumeration(info);

        return devices;
    }

    bool setHidDevice(hid_device_info info)
    {
        auto dev = hid_open(info.vendor_id, info.product_id, nullptr);
        bool ret = false;

        if (nullptr != dev)
        {
            currentHidInfo = info;

            if (joycon != nullptr)
            {
                joycon->Detach();
                delete joycon;
            }

            joycon = new Joycon(dev, true, true, 0.05f, (info.product_id == Joycon::product_id_left) ? true : false);

            if (true == joycon->Attach())
            {
                ret = true;
                joycon->Begin();
                startTimer(5);
            }
        }

        return ret;
    }

    Joycon* const getJoycon(void)
    {
        return joycon;
    }

    hid_device_info* const getHidDeviceInfo(void)
    {
        return &currentHidInfo;
    }

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JoyconGoodnessAudioProcessor)

    Joycon* joycon = nullptr;
    hid_device_info currentHidInfo;

    void timerCallback() override
    {
        joycon->Update();
    }
};
