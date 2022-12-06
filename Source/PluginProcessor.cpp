/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
JoyconGoodnessAudioProcessor::JoyconGoodnessAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       )
{
    hid_init();
}

JoyconGoodnessAudioProcessor::~JoyconGoodnessAudioProcessor()
{
    stopTimer();
    if (nullptr != joycon)
    {
        joycon->Detach();
        delete joycon;
    }
    hid_exit();
}

//==============================================================================
const juce::String JoyconGoodnessAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JoyconGoodnessAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JoyconGoodnessAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JoyconGoodnessAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JoyconGoodnessAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JoyconGoodnessAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int JoyconGoodnessAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JoyconGoodnessAudioProcessor::setCurrentProgram (int index)
{
    (void)index;
}

const juce::String JoyconGoodnessAudioProcessor::getProgramName (int index)
{
    (void)index;
    return {};
}

void JoyconGoodnessAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    (void)index;
    (void)newName;
}

//==============================================================================
void JoyconGoodnessAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    (void)sampleRate;
    (void)samplesPerBlock;
}

void JoyconGoodnessAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool JoyconGoodnessAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    (void)layouts;
    return true;
}

void JoyconGoodnessAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    midiMessages.clear();
    if (nullptr != joycon)
    {
        midiMessages.addEvent(juce::MidiMessage::controllerEvent(1,  23, (int)(joycon->getPitchRollYaw().x * 20.f)), 0);
        midiMessages.addEvent(juce::MidiMessage::controllerEvent(1,  24, (int)(joycon->getPitchRollYaw().y * 20.f)), 0);
        midiMessages.addEvent(juce::MidiMessage::controllerEvent(1,  25, (int)(joycon->getPitchRollYaw().z * 20.f)), 0);
    }
}

//==============================================================================
bool JoyconGoodnessAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JoyconGoodnessAudioProcessor::createEditor()
{
    return new JoyconGoodnessAudioProcessorEditor (*this);
}

//==============================================================================
void JoyconGoodnessAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    (void)destData;
}

void JoyconGoodnessAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    (void)data;
    (void)sizeInBytes;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JoyconGoodnessAudioProcessor();
}
