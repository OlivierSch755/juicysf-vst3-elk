#include "FluidSynthModel.h"

// Map MIDI controller nb to paramaterID
const std::map<fluid_midi_control_change, juce::String> 
FluidSynthModel::controllerToParam{
    {SOUND_CTRL2	, 	"filterResonance"}, // MIDI CC 71 Timbre/Harmonic Intensity (filter resonance)
    {SOUND_CTRL3	, 	"release"}, 		// MIDI CC 72 Release time
    {SOUND_CTRL4	, 	"attack"}, 			// MIDI CC 73 Attack time
    {SOUND_CTRL5	, 	"filterCutOff"}, 	// MIDI CC 74 Brightness (cutoff frequency, FILTERFC)
    {SOUND_CTRL6	, 	"decay"}, 			// MIDI CC 75 Decay Time
    {SOUND_CTRL10	,	 "sustain"},		// MIDI CC 79 undefined
    {MODULATION_MSB	,	 "modulation"}		// MIDI CC 1 
}; 

// Map  paramaterID to MIDI controller nb
const std::map<juce::String, fluid_midi_control_change>
FluidSynthModel::paramToController{[] {
    std::map<juce::String, fluid_midi_control_change> result;
    for (const auto& entry : controllerToParam){
        result.emplace(entry.second, entry.first);
	}
    return result;
}()};

FluidSynthModel::FluidSynthModel(
	juce::AudioProcessorValueTreeState& valueTreeState
)
: valueTreeState{valueTreeState}
, settings{ nullptr, nullptr }
, synth{ nullptr, nullptr }
, channel{0}
, sfont_id{-1}
{
    valueTreeState.addParameterListener("bank", this);
    valueTreeState.addParameterListener("preset", this);
    valueTreeState.addParameterListener("pitch", this);
	
    for (const auto &[param, controller]: paramToController) {
        valueTreeState.addParameterListener(param, this);
    }
}

FluidSynthModel::~FluidSynthModel() {
    for (const auto &[param, controller]: paramToController) {
        valueTreeState.removeParameterListener(param, this);
    }
    valueTreeState.removeParameterListener("bank", this);
    valueTreeState.removeParameterListener("preset", this);
	valueTreeState.removeParameterListener("pitch", this);
}

void FluidSynthModel::initialise() 
{

	
    // Pass null as driver pointer, fluidsynth will only be rendering blocks 
    const char *audio_driver[] {NULL};
    fluid_audio_driver_register(audio_driver);
    
    settings = { new_fluid_settings(), delete_fluid_settings };
    
#if JUCE_DEBUG
    // fluid_settings_setint(settings.get(), "synth.verbose", 1);
#endif

    synth = { 
		new_fluid_synth( settings.get() ),
		delete_fluid_synth 
	};

    // I can't hear a damned thing
    fluid_synth_set_gain( synth.get(), 2.0 );


	// note: fluid_chan.c#fluid_channel_init_ctrl()
    // > Just like panning, a value of 64 indicates no change for sound ctrls
    // --
    // so, advice is to leave everything at 64
    // and yet, I'm finding that default modulators start at MIN,
    // i.e. we are forced to start at 0 and climb from there
    // --
    // let's zero out every audio param that we manage
    for (const auto &[controller, param]: controllerToParam) {
		fluid_synth_cc(
			synth.get(),
			channel,
			static_cast<int>(controller),
			0
		);
    }
    
    // http://www.synthfont.com/SoundFont_NRPNs.PDF
    float env_amount{20000.0f};
    
    std::unique_ptr<fluid_mod_t, decltype(&delete_fluid_mod)> mod{new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                          static_cast<int>(SOUND_CTRL2), // MIDI CC 71 Timbre/Harmonic Intensity (filter resonance)
                          FLUID_MOD_CC
                          | FLUID_MOD_UNIPOLAR
                          | FLUID_MOD_CONCAVE
                          | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_FILTERQ);
    fluid_mod_set_amount(mod.get(), FLUID_PEAK_ATTENUATION);
    fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
    
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                          static_cast<int>(SOUND_CTRL3), // MIDI CC 72 Release time
                          FLUID_MOD_CC
                          | FLUID_MOD_UNIPOLAR
                          | FLUID_MOD_LINEAR
                          | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_VOLENVRELEASE);
    fluid_mod_set_amount(mod.get(), env_amount);
    fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
    
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                          static_cast<int>(SOUND_CTRL4), // MIDI CC 73 Attack time
                          FLUID_MOD_CC
                          | FLUID_MOD_UNIPOLAR
                          | FLUID_MOD_LINEAR
                          | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_VOLENVATTACK);
    fluid_mod_set_amount(mod.get(), env_amount);
    fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
    
    // soundfont spec says that if cutoff is >20kHz and resonance Q is 0, then no filtering occurs
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                          static_cast<int>(SOUND_CTRL5), // MIDI CC 74 Brightness (cutoff frequency, FILTERFC)
                          FLUID_MOD_CC
                          | FLUID_MOD_LINEAR
                          | FLUID_MOD_UNIPOLAR
                          | FLUID_MOD_POSITIVE);
        fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_FILTERFC);
    fluid_mod_set_amount(mod.get(), -2400.0f);
    fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
    
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                          static_cast<int>(SOUND_CTRL6), // MIDI CC 75 Decay Time
                          FLUID_MOD_CC
                          | FLUID_MOD_UNIPOLAR
                          | FLUID_MOD_LINEAR
                          | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_VOLENVDECAY);
    fluid_mod_set_amount(mod.get(), env_amount);
    fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);
    
    mod = {new_fluid_mod(), delete_fluid_mod};
    fluid_mod_set_source1(mod.get(),
                          static_cast<int>(SOUND_CTRL10), // MIDI CC 79 undefined
                          FLUID_MOD_CC
                          | FLUID_MOD_UNIPOLAR
                          | FLUID_MOD_CONCAVE
                          | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(mod.get(), 0, 0);
    fluid_mod_set_dest(mod.get(), GEN_VOLENVSUSTAIN);
    // fluice_voice.c#fluid_voice_update_param()
    // clamps the range to between 0 and 1000, so we'll copy that
    fluid_mod_set_amount(mod.get(), 1000.0f);
    fluid_synth_add_default_mod(synth.get(), mod.get(), FLUID_SYNTH_ADD);

 
}

const juce::StringArray FluidSynthModel::programChangeParams{"bank", "preset"};

void FluidSynthModel::parameterChanged(const juce::String& parameterID, float newValue) {
	
	
	if(sfont_id == -1){
		return;
	}
	
	// DBG("parameterChanged: " + parameterID + " = " + juce::String(newValue));

	/* 	
		Banks and preset are a convenient abstraction provided by fluidsynth.
		But they actually both represent a single MIDI-like programChange within the soundfont.
		This is why they have a special treatment here, as changing any of the two value needs to 
		trigger the same function call : fluid_synth_program_select(...);
	*/
    if (programChangeParams.contains(parameterID)) {
        int bank, preset;
        {
            juce::RangedAudioParameter *param{valueTreeState.getParameter("bank")};
            jassert(dynamic_cast<juce::AudioParameterInt*>(param) != nullptr);
            juce::AudioParameterInt* castParam{dynamic_cast<juce::AudioParameterInt*>(param)};
            bank = castParam->get();
        }
        {
            juce::RangedAudioParameter *param{valueTreeState.getParameter("preset")};
            jassert(dynamic_cast<juce::AudioParameterInt*>(param) != nullptr);
            juce::AudioParameterInt* castParam{dynamic_cast<juce::AudioParameterInt*>(param)};
            preset = castParam->get();
        }
        int bankOffset{fluid_synth_get_bank_offset(synth.get(), sfont_id)};
        fluid_synth_program_select(
            synth.get(),
            channel,
            sfont_id,
            static_cast<unsigned int>(bankOffset + bank),
            static_cast<unsigned int>(preset)
		);
    }
	else if(parameterID == "pitch"){
		fluid_synth_pitch_bend(
			synth.get(),
			channel,
			juce::roundToInt(newValue)
		);
	}
	else if (
		auto it{paramToController.find(parameterID)};
         it != end(paramToController)
	){
		int controllerNumber{static_cast<int>(it->second)};

		fluid_synth_cc(
			synth.get(),
			channel,
			controllerNumber,
			juce::roundToInt(newValue)
		);
	}
}

void FluidSynthModel::restoreParameters()
{
    for (auto* param : valueTreeState.processor.getParameters() )
    {
        auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param);

        if (rangedParam == nullptr)
            continue;

        parameterChanged(
            rangedParam->paramID,
            rangedParam->convertFrom0to1(rangedParam->getValue())
        );
    }
}

void FluidSynthModel::setSampleRate(float sampleRate) 
{
	
    if (!synth) {
		DBG("Cannot set sample rate, there is no synth...");
        return;
    }
    fluid_synth_set_sample_rate(synth.get(), sampleRate);
}

void FluidSynthModel::unloadAndLoadFont(const juce::String &absPath) 
{
	
	 DBG("sf: " + absPath);
	
    // in the base case, there is no font loaded
    if (fluid_synth_sfcount(synth.get()) > 0) {
        // if -1 is returned, that indicates failure
        // not really sure how to handle "fail to unload"
        fluid_synth_sfunload(synth.get(), sfont_id, 1);
        sfont_id = -1;
    }
    loadFont(absPath);
}

void FluidSynthModel::loadFont(const juce::String& absPath)
{
    if (absPath.isEmpty())
    {
        DBG("No SoundFont path provided");
        return;
    }

	// DBG("Loading Soundfont");
    sfont_id = fluid_synth_sfload(
        synth.get(),
        absPath.toRawUTF8(),
        1
    );

    if (sfont_id == FLUID_FAILED)
        DBG("SoundFont loading FAILED: " + absPath);
    else{
        DBG("SoundFont loaded successfully. ID: " + juce::String(sfont_id));
	}
}


int FluidSynthModel::getCurrentProgram()
{
    juce::RangedAudioParameter *param{valueTreeState.getParameter("preset")};
    jassert(dynamic_cast<juce::AudioParameterInt*>(param) != nullptr);
    juce::AudioParameterInt* castParam{dynamic_cast<juce::AudioParameterInt*>(param)};
    return castParam->get();
}

void FluidSynthModel::setCurrentProgram(int index)
{
    juce::RangedAudioParameter *param{valueTreeState.getParameter("preset")};
    jassert(dynamic_cast<juce::AudioParameterInt*>(param) != nullptr);
    juce::AudioParameterInt* castParam{dynamic_cast<juce::AudioParameterInt*>(param)};
    *castParam = index;
}

const juce::String FluidSynthModel::getProgramName(int index)
{
     fluid_sfont_t* sfont{
         sfont_id == -1
         ? nullptr
         : fluid_synth_get_sfont_by_id(synth.get(), sfont_id)
     };
     if (!sfont) {
         juce::String presetName{"Preset "};
         return presetName << index;
     }
     juce::RangedAudioParameter *param{valueTreeState.getParameter("bank")};
     jassert(dynamic_cast<juce::AudioParameterInt*>(param) != nullptr);
     juce::AudioParameterInt* castParam{dynamic_cast<juce::AudioParameterInt*>(param)};
     int bank{castParam->get()};
    
     fluid_preset_t *preset{
		 fluid_sfont_get_preset(
			 sfont,
			 bank,
			 index
		 )
	 };
		 
     if (!preset) {
         juce::String presetName{"Preset "};
         return presetName << index;
     }
     return {fluid_preset_get_name(preset)};
}




void FluidSynthModel::processBlock(
	juce::AudioBuffer<float>& buffer,
	juce::MidiBuffer& midiMessages
) 
{

	for (const auto metadata : midiMessages) {
		const auto& m = metadata.getMessage();
		const int time = metadata.samplePosition;
        if (m.isNoteOn()) {
            fluid_synth_noteon(
                synth.get(),
                channel,
                m.getNoteNumber(),
                m.getVelocity());
        } else if (m.isNoteOff()) {
            fluid_synth_noteoff(
                synth.get(),
                channel,
                m.getNoteNumber());
        }
		else if (m.isSysEx()) {
            fluid_synth_sysex(
                synth.get(),
                reinterpret_cast<const char*>(m.getSysExData()),
                m.getSysExDataSize(),
                nullptr, // no response pointer because we have no interest in handling response currently
                nullptr, // no response_len pointer because we have no interest in handling response currently
                nullptr, // no handled pointer because we have no interest in handling response currently
                static_cast<int>(false));
        }
    }

    fluid_synth_process(
        synth.get(),
        buffer.getNumSamples(),
        0,
        nullptr,
        buffer.getNumChannels(),
        const_cast<float**>(buffer.getArrayOfWritePointers())
	);
}


