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
    addAndMakeVisible(hidDbg);

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
    hidDbg.setBoundsRelative(.0f, .1f, .3f, .3f);
}

void JoyconGoodnessAudioProcessorEditor::mouseDown (const MouseEvent& event)
{
    if (event.eventComponent == &hidSelector)
    {
        int idx = 1;
        struct hid_device_info* info = hid_enumerate(0, 0);

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

        std::for_each(hidInfo.begin(), hidInfo.end(), [this, idx](hid_device_info &i) mutable {this->hidSelector.addItem(juce::String{i.product_string}, idx++);});
    }
}

void JoyconGoodnessAudioProcessorEditor::comboBoxChanged (ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &hidSelector)
    {
        hid_device_info* info = &hidInfo.at((unsigned long)hidSelector.getSelectedItemIndex());

        hidDev = hid_open(info->vendor_id, info->product_id, nullptr);
        if (nullptr != hidDev)
        {
            hidText.setButtonText(juce::String{info->product_string});
            Attach(1);

            uint8 a[1];
            for (uint8 i = 0; i < 16; i++)
            {
                a[0] = (i % 16) + 1;
                JoyconSubcmd(0x30, a, 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

void JoyconGoodnessAudioProcessorEditor::JoyconSubcmd(uint8 sc, uint8 *buf, uint8 len, bool print)
{
    uint8 report[49] = {0};
    uint8 response[49] = {0};
    uint8 defaultBuf[] = { 0x0, 0x1, 0x40, 0x40, 0x0, 0x1, 0x40, 0x40 };
    const uint8 totLen = len + 11;

    std::memcpy(report+2, defaultBuf, sizeof(defaultBuf));
    std::memcpy(report+11, buf, len);

    report[0] = 0x1;
    report[1] = global_count;
    report[10] = sc;

    if (global_count == 0xf) {
        global_count = 0; }
    else {
        ++global_count; }

    if (print) 
    { 
        char str[512];
        char hex[512];
        int n = 0;

        for (uint8 i = 0; i < totLen; i++)
        {
            n += std::snprintf(hex + n, sizeof(hex), "%02xx", report[i]); 
        }
        std::snprintf(str, sizeof(str), "Subcommand 0x%02x sent. Data: %s", sc, hex); 

        hidDbg.setButtonText(str);
    };
    
    hid_write(hidDev, report, totLen);
    int res = hid_read_timeout(hidDev, response, 49, 50);

    if (res < 1) {
        hidDbg.setButtonText("No Response");
    }
}

bool JoyconGoodnessAudioProcessorEditor:: Attach(uint8 leds_)
{
    uint8 a[1] = { 0x0 };

    // Input report mode
    JoyconSubcmd(0x3, a, 1, false);
    a[0] = 0x1;
    // dump_calibration_data();
    // Connect
    a[0] = 0x01;
    JoyconSubcmd(0x1, a, 1);
    a[0] = 0x02;
    JoyconSubcmd(0x1, a, 1);
    a[0] = 0x03;
    JoyconSubcmd(0x1, a, 1);
    a[0] = leds_;
    JoyconSubcmd(0x30, a, 1);
    a[0] = 0x1;
    JoyconSubcmd(0x40, a, 1, true);
    JoyconSubcmd(0x3, a, 1, true);
    JoyconSubcmd(0x48, a, 1, true);

    return 0;
}
