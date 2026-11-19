# Abstract

I found myself struggling with the current implementation of [fluidsynth](https://github.com/elk-community/juicysfplugin) in Elk Audio. 

Despite my best efforts, I could not manage to get a deterministic behavior out of the provided plugin.
My main two issues are: 
- Soundfont path restoration will fail silently sometimes. 
- Most parameters are not reapplied on session restore from Sushi. 


My understanding of the issues is the following:
- Elk uses a custom extension to achieve VST3 string property control from the host.
- The control flow is traversing many layers (VST3 JUCE wrapper -> Audioprocessor -> Fluidsynth) and sometimes fails in unpredictable ways. 
- Data flowing in the opposing direction (plugin -> host) can also be stalled, mostly when it comes to parameters and programs.
- The plugin uses JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS which creates 16x130 parameters (despite using only 8 of them), Sushi does not know how to hide them and most of its visualization tools will eagerly load them all (despite them all being empty). This makes debugging really painful.
- There are a lot of GUI logic and MAC specific logic in the codebase with some mutexes that do not help debugging either. 

Also at the time of this writing (november 2026), Elk implementation of juicysfplugin simply does not compile on ELK using the latest SDK and the provided instructions. 

## What this is exactly

This repo is a modified version of [juicysfplugin](https://github.com/Birch-san/juicysfplugin) by Alex Birch (same codebase source than the one used by ELK) that I stripped of all UI-related logic and mutexes to help understanding where those issues stem from. 
I also rewrote it to avoid JUCE MIDI CC emulation. Everything is declared as VST3 parameter instead, which is closer to Steinberg implementation. 

## Build
You should be using Elk Audio SDK and I strongly recommand using the [docker image](https://github.com/elk-audio/elk-audio-os-builder) which comes will all dependencies. 

```sh
# You need to source the SDK.
# If using Docker container, the path will be /SDKs/elkpi/environment-setup-cortexa72-elk-linux

source [path-to-cross-compiling-sdk-environment-activation-script]

# Configure
cmake -DCMAKE_BUILD_TYPE=Release -B build -S .

# Build
cmake --build build --parallel --config Release
```

Change Build type to "Debug" if needed for development. 




## Limitations and issues
There are still occurrences of soundfont_path string property not being restored in Sushi 1.4.0. It happens **way less frequently** in this version but it still does. 
I could track the issue to that function ```ElkVST3Extensions::propertyValueChanged(Steinberg::int32 propertyId, const elk::PropertyValue& value)```. My observation is that this notification is not received by the plugin when SF restoration fails. Which hints at a race condition somewhere (my guess is that the notification is fired before the plugin listener subscribed)

Some parameters update seems to be also missed by Sushi when querying its inner state with gRPC.

I may have broken aftertouch in the process. 



## Credits 
This is a rework of Alex Birch plugin.