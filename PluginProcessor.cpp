/*
 Created by Alex Birch on 10/09/2017.
 Edited by Olivier Schwach on 16/11/2026
*/



#include "PluginProcessor.h"
#include "MidiConstants.h"

//==============================================================================
JuicySfHeadlessAudioProcessor::JuicySfHeadlessAudioProcessor()
     : AudioProcessor (getBusesProperties()), 
	 valueTreeState{
		*this,
		nullptr,
		"JSFH",
		createParameterLayout()
	}
	,fluidSynthModel{valueTreeState}
{
	fluidSynthModel.initialise();
}

JuicySfHeadlessAudioProcessor::~JuicySfHeadlessAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout 
JuicySfHeadlessAudioProcessor::createParameterLayout() {
    // https://stackoverflow.com/a/8469002/5257399

    std::unique_ptr<juce::AudioParameterInt> params[] {
        // SoundFont 2.4 spec section 7.2: zero through 127, or 128.
        std::make_unique<juce::AudioParameterInt>(
            "bank",
            "which bank is selected in the soundfont",
            MidiConstants::midiMinValue,
            128,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("Bank")),
        // note: banks may be sparse, and lack a 0th preset. so defend against this.
        std::make_unique<juce::AudioParameterInt>(
            "preset",
            "which patch (aka patch, program, instrument) is selected in the soundfont",
            MidiConstants::midiMinValue,
            MidiConstants::midiMaxValue,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("Preset")),
        std::make_unique<juce::AudioParameterInt>(
            "modulation",
            "modulation wheel",
            MidiConstants::midiMinValue,
            MidiConstants::midiMaxValue,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("Mod")),
        std::make_unique<juce::AudioParameterInt>(
            "pitch",
            "pitch wheel",
            MidiConstants::midiMinValue,
            16383,
            8192,
            juce::AudioParameterIntAttributes().withLabel("Pitch")),
        std::make_unique<juce::AudioParameterInt>(
            "attack",
            "volume envelope attack time",
            MidiConstants::midiMinValue,
            MidiConstants::midiMaxValue,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("A")),
        std::make_unique<juce::AudioParameterInt>(
            "decay",
            "volume envelope sustain attentuation",
            MidiConstants::midiMinValue,
            MidiConstants::midiMaxValue,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("D")),
        std::make_unique<juce::AudioParameterInt>(
            "sustain",
            "volume envelope decay time",
            MidiConstants::midiMinValue,
            MidiConstants::midiMaxValue,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("S")),
        std::make_unique<juce::AudioParameterInt>(
            "release",
            "volume envelope release time",
            MidiConstants::midiMinValue,
            MidiConstants::midiMaxValue,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("R")),
        std::make_unique<juce::AudioParameterInt>(
            "filterCutOff",
            "low-pass filter cut-off frequency",
            MidiConstants::midiMinValue,
            MidiConstants::midiMaxValue,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("Cut")),
        std::make_unique<juce::AudioParameterInt>(
            "filterResonance",
            "low-pass filter resonance attentuation",
            MidiConstants::midiMinValue,
            MidiConstants::midiMaxValue,
            MidiConstants::midiMinValue,
            juce::AudioParameterIntAttributes().withLabel("Res")),
    };

    return {
        std::make_move_iterator(std::begin(params)),
        std::make_move_iterator(std::end(params))
    };
}

FluidSynthModel& JuicySfHeadlessAudioProcessor::getFluidSynthModel() {
    return fluidSynthModel;
}


//==============================================================================
const juce::String JuicySfHeadlessAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JuicySfHeadlessAudioProcessor::acceptsMidi() const
{
    return true;
}

bool JuicySfHeadlessAudioProcessor::producesMidi() const
{
    return false;
}

bool JuicySfHeadlessAudioProcessor::isMidiEffect() const
{
    return false;
}

double JuicySfHeadlessAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JuicySfHeadlessAudioProcessor::getNumPrograms()
{
    return 128;
}

int JuicySfHeadlessAudioProcessor::getCurrentProgram()
{
	return fluidSynthModel.getCurrentProgram();
}

void JuicySfHeadlessAudioProcessor::setCurrentProgram (int index)
{
    return fluidSynthModel.setCurrentProgram(index);
}

const juce::String JuicySfHeadlessAudioProcessor::getProgramName (int index)
{
	return fluidSynthModel.getProgramName(index);
}

void JuicySfHeadlessAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void JuicySfHeadlessAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
	fluidSynthModel.setSampleRate(static_cast<float>(sampleRate));
}

void JuicySfHeadlessAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool JuicySfHeadlessAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only mono/stereo and input/output must have same layout
    const juce::AudioChannelSet& mainOutput = layouts.getMainOutputChannelSet();
    const juce::AudioChannelSet& mainInput  = layouts.getMainInputChannelSet();

    // input and output layout must either be the same or the input must be disabled altogether
    if (! mainInput.isDisabled() && mainInput != mainOutput)
        return false;

    // do not allow disabling the main buses
    if (mainOutput.isDisabled())
        return false;

    // only allow stereo and mono
    return mainOutput.size() <= 2;
}

juce::AudioProcessor::BusesProperties 
JuicySfHeadlessAudioProcessor::getBusesProperties() {
    return juce::AudioProcessor::BusesProperties()
            .withInput ("Input",  juce::AudioChannelSet::stereo(), false)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true);
}



void JuicySfHeadlessAudioProcessor::processBlock( 
	juce::AudioBuffer<float>& buffer,
	juce::MidiBuffer& midiMessages
) 
{
    jassert (!isUsingDoublePrecision());

    // While a new soundfont is being loaded asynchronously, mute and discard MIDI
    if (isLoading.load()) {
        buffer.clear();
        midiMessages.clear();
        return;
    }

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    fluidSynthModel.processBlock(buffer, midiMessages);

    // (see juce_VST3_Wrapper.cpp for the assertion this would trip otherwise)
    // we are !JucePlugin_ProducesMidiOutput, so clear remaining MIDI messages from our buffer
    midiMessages.clear();
}




//==============================================================================
bool JuicySfHeadlessAudioProcessor::hasEditor() const
{
    return false; 
}

juce::AudioProcessorEditor* JuicySfHeadlessAudioProcessor::createEditor()
{
	return nullptr;
}

//==============================================================================
void JuicySfHeadlessAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xmlState = valueTreeState.copyState().createXml();
	copyXmlToBinary(*xmlState, destData);
}

void JuicySfHeadlessAudioProcessor::setStateInformation (const void* data,
                                                          int sizeInBytes)
{
    auto xmlState = getXmlFromBinary (data, sizeInBytes);

    if (xmlState != nullptr
        && xmlState->hasTagName (valueTreeState.state.getType()))
    {
        valueTreeState.replaceState (juce::ValueTree::fromXml (*xmlState));

        for (auto* parameter : getParameters())
        {
            if (auto* rangedParameter =
                    dynamic_cast<juce::RangedAudioParameter*> (parameter))
            {
                rangedParameter->setValueNotifyingHost (
                    rangedParameter->getValue()
                );
            }
        }
    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JuicySfHeadlessAudioProcessor();
}
