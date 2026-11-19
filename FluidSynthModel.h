/*
 Created by Alex Birch on 10/09/2017.
 Edited by Olivier Schwach on 16/11/2026
*/


#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <fluidsynth.h>
#include <memory>
#include "MidiConstants.h"

class FluidSynthModel
: public juce::AudioProcessorValueTreeState::Listener 
{
public:
    FluidSynthModel( juce::AudioProcessorValueTreeState& valueTreeState );
    ~FluidSynthModel();
    void initialise();
	void setSampleRate(float sampleRate);
	void unloadAndLoadFont(const juce::String &absPath);
	void loadFont(const juce::String &absPath);
	
	int getCurrentProgram();
	void setCurrentProgram(int index);
	const juce::String getProgramName(int index);
	
	void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    //==============================================================================
	
    virtual void parameterChanged (const juce::String& parameterID, float newValue) override;
	
    //==============================================================================
	void restoreParameters();
	
private:

	static const juce::StringArray programChangeParams;

    // there's no bimap in the standard library!
    static const std::map<fluid_midi_control_change, juce::String> controllerToParam;
    static const std::map<juce::String, fluid_midi_control_change> paramToController;

	juce::AudioProcessorValueTreeState& valueTreeState;
	
    std::unique_ptr<fluid_settings_t, decltype(&delete_fluid_settings)> settings;
    std::unique_ptr<fluid_synth_t, decltype(&delete_fluid_synth)> synth;
	
	int channel;
	int sfont_id;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FluidSynthModel)
};