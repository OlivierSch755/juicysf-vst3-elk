#include "ElkVST3Extensions.h"
#include "PluginProcessor.h"

//==============================================================================
// ElkVST3Extensions implementation

ElkVST3Extensions::ElkVST3Extensions(JuicySfHeadlessAudioProcessor* processor)
    : audioProcessor(processor)
{
    propertyInfo.id = 1000;
    copyStringToVst3String("soundfont_path", propertyInfo.name);
    copyStringToVst3String("Soundfont Path", propertyInfo.label);
    propertyInfo.flags = elk::PropertyInfo::kAudioThreadNotify;
}

void ElkVST3Extensions::setIHostApplication(Steinberg::FUnknown* host)
{
    if (host == nullptr) { hostExtension = nullptr; return; }
    void* obj = nullptr;
    if (host->queryInterface(elk::IElkHostExtension_iid, &obj) == Steinberg::kResultOk)
        hostExtension = static_cast<elk::IElkHostExtension*>(obj);
    else
        hostExtension = nullptr;
}

Steinberg::int32 ElkVST3Extensions::queryIEditController(const Steinberg::TUID tuid, void** obj)
{
    if (std::memcmp(tuid, elk::IElkControllerExtension_iid, sizeof(Steinberg::TUID)) == 0)
    {
        addRef();
        *obj = static_cast<elk::IElkControllerExtension*>(this);
        return Steinberg::kResultOk;
    }
    *obj = nullptr;
    return Steinberg::kNoInterface;
}

Steinberg::int32 ElkVST3Extensions::queryIAudioProcessor(const Steinberg::TUID tuid, void** obj)
{
    if (std::memcmp(tuid, elk::IElkProcessorExtension_iid, sizeof(Steinberg::TUID)) == 0)
    {
        addRef();
        *obj = static_cast<elk::IElkProcessorExtension*>(this);
        return Steinberg::kResultOk;
    }
    *obj = nullptr;
    return Steinberg::kNoInterface;
}

Steinberg::int32 ElkVST3Extensions::getPropertyCount()
{
    return 1;
}

Steinberg::tresult ElkVST3Extensions::getPropertyInfo(Steinberg::int32 index, elk::PropertyInfo& info)
{
    if (index != 0) return Steinberg::kResultFalse;
    info = propertyInfo;
    return Steinberg::kResultOk;
}

Steinberg::tresult ElkVST3Extensions::getPropertyValue(Steinberg::int32 id, elk::PropertyValue& value)
{
    if (id != static_cast<Steinberg::int32>(propertyInfo.id)) return Steinberg::kResultFalse;
    value.value = propertyValue.c_str();
    value.length = static_cast<Steinberg::int32>(propertyValue.size());
    return Steinberg::kResultOk;
}

Steinberg::tresult ElkVST3Extensions::setPropertyValue(Steinberg::int32 id, const elk::PropertyValue& value)
{
    if (id != static_cast<Steinberg::int32>(propertyInfo.id)) return Steinberg::kResultFalse;
    propertyValue.clear();
    if (value.value != nullptr && value.length > 0)
        propertyValue.append(value.value, static_cast<size_t>(value.length));
    return Steinberg::kResultOk;
}

void ElkVST3Extensions::propertyValueChanged(Steinberg::int32 propertyId,
                                               const elk::PropertyValue& value)
{
    if (propertyId != static_cast<Steinberg::int32>(propertyInfo.id)) return;

    pendingFontPath.clear();
    if (value.value != nullptr && value.length > 0)
        pendingFontPath.append(value.value, static_cast<size_t>(value.length));

    // Signal processBlock() to mute/discard before async work starts
    audioProcessor->isLoading.store(true);

    if (hostExtension != nullptr)
    {
        Steinberg::int32 requestId;
        hostExtension->requestAsyncWork(asyncWorkCallback, this, requestId);
    }
}

Steinberg::int32 ElkVST3Extensions::asyncWorkCallback(void* data, Steinberg::uint16 /*requestId*/)
{
    auto* ext = static_cast<ElkVST3Extensions*>(data);
    if (ext == nullptr || ext->audioProcessor == nullptr) return Steinberg::kResultFalse;


    auto& model = ext->audioProcessor->getFluidSynthModel();

    model.unloadAndLoadFont(juce::String(ext->pendingFontPath));
    model.restoreParameters();
	
    return Steinberg::kResultOk;
}

void ElkVST3Extensions::asyncWorkCompleted(Steinberg::int32 /*requestId*/,
                                             Steinberg::int32 /*returnValue*/)
{
    if (audioProcessor != nullptr)
        audioProcessor->isLoading.store(false);
}

Steinberg::uint32 ElkVST3Extensions::addRef()
{
    return ++refCount;
}

Steinberg::uint32 ElkVST3Extensions::release()
{
    return --refCount;
}

Steinberg::tresult ElkVST3Extensions::queryInterface(const Steinberg::TUID /*tuid*/, void** obj)
{
    *obj = nullptr;
    return Steinberg::kNoInterface;
}

void ElkVST3Extensions::copyStringToVst3String(const char* src, Steinberg::Vst::String128 dest)
{
    std::memset(dest, 0, sizeof(Steinberg::Vst::String128));
    for (size_t i = 0; i < 127 && src[i] != '\0'; ++i)
        dest[i] = static_cast<Steinberg::Vst::TChar>(src[i]);
}

//==============================================================================