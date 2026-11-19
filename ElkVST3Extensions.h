#pragma once

#include <atomic>
#include <cstring>
#include <string>

#include <juce_audio_processors/juce_audio_processors.h>
#include <elk_vst3_extensions/elk_vst3_extensions.h>

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wnon-virtual-dtor")

class JuicySfHeadlessAudioProcessor;

class ElkVST3Extensions final : public juce::VST3ClientExtensions,
                                  public elk::IElkControllerExtension,
                                  public elk::IElkProcessorExtension
{
public:
    explicit ElkVST3Extensions(JuicySfHeadlessAudioProcessor* processor);

    // VST3ClientExtensions
    void setIHostApplication(Steinberg::FUnknown* host) override;
    Steinberg::int32 queryIEditController(const Steinberg::TUID tuid, void** obj) override;
    Steinberg::int32 queryIAudioProcessor(const Steinberg::TUID tuid, void** obj) override;

    // IElkControllerExtension
    Steinberg::int32   PLUGIN_API getPropertyCount() override;
    Steinberg::tresult PLUGIN_API getPropertyInfo(Steinberg::int32 index, elk::PropertyInfo& info) override;
    Steinberg::tresult PLUGIN_API getPropertyValue(Steinberg::int32 id, elk::PropertyValue& value) override;
    Steinberg::tresult PLUGIN_API setPropertyValue(Steinberg::int32 id, const elk::PropertyValue& value) override;

    // IElkProcessorExtension
    void PLUGIN_API propertyValueChanged(Steinberg::int32 id, const elk::PropertyValue& value) override;
    void PLUGIN_API asyncWorkCompleted(Steinberg::int32 requestId, Steinberg::int32 returnValue) override;

    // FUnknown
    Steinberg::uint32 PLUGIN_API addRef() override;
    Steinberg::uint32 PLUGIN_API release() override;
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID tuid, void** obj) override;

    static Steinberg::int32 asyncWorkCallback(void* data, Steinberg::uint16 requestId);

private:
    static void copyStringToVst3String(const char* src, Steinberg::Vst::String128 dest);

    elk::PropertyInfo propertyInfo;
    std::string propertyValue;       // current value (controller side)
    std::string pendingFontPath;     // path to load (set in propertyValueChanged)
    std::atomic<Steinberg::uint32> refCount{1};

    JuicySfHeadlessAudioProcessor* audioProcessor;
    elk::IElkHostExtension* hostExtension{nullptr};
};

JUCE_END_IGNORE_WARNINGS_GCC_LIKE
