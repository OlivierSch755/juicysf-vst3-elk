#pragma once

#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "FluidSynthModel.h"
#include "ElkVST3Extensions.h"

//==============================================================================
class JuicySfHeadlessAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    JuicySfHeadlessAudioProcessor();
    ~JuicySfHeadlessAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

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


	// Project specifics implementation
	//==============================================================================
	FluidSynthModel& getFluidSynthModel();
	std::atomic<bool> isLoading{false};
	juce::VST3ClientExtensions* getVST3ClientExtensions() override { return &vst3Extensions; }
	
private:
    //==============================================================================
	
	
	
	juce::AudioProcessorValueTreeState valueTreeState;
	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
	
	FluidSynthModel fluidSynthModel;
	
	static BusesProperties getBusesProperties();
	
	ElkVST3Extensions vst3Extensions{this};
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuicySfHeadlessAudioProcessor)
};
