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

This repo is a modified version of [juicysfplugin](https://github.com/Birch-san/juicysfplugin) by Alex Birch (same codebase source than the one used by ELK) that I stripped of all UI-related logic and UI-mutexes to help understanding where those issues stem from. 
I also rewrote it to avoid JUCE MIDI CC emulation. Everything is declared as VST3 parameter instead, which is closer to Steinberg implementation. 

## Install from release (Elk-pi)
If you are using Elk-pi (distribution for Raspberry Pi), please use the release section of this repository. 
Download the plugin archive and simply extract it with your other VST3. 

## Build from source
You should be using Elk Audio SDK and I strongly recommand using the [docker image](https://github.com/elk-audio/elk-audio-os-builder) which comes will all dependencies. 

### Download 
```sh
git clone --recursive https://github.com/OlivierSch755/juicysf-vst3-elk.git
cd juicysf-vst3-elk/
```

### Compile 
```sh
# You need to source the SDK.
# If using Docker container, the path will be /SDKs/elkpi/environment-setup-cortexa72-elk-linux

source [path-to-cross-compiling-sdk-environment-activation-script]

# Configure
cmake -DCMAKE_BUILD_TYPE=Release -B build -S .

# Build
cmake --build build --parallel --config Release
```

Change Build type to "Debug" if needed for development (see section below)

### Installation
Successful build will produce ``build/JuicySfHeadless_artefacts/Release/VST3/JuicySfHeadless.vst3/``.
Copy this directory in the directory where your Elk box stores its VST3 plugins. 

#### Elk-Pi plugin discovery (Sushi web-ui)
To enable loading this plugin via Sushi web-ui on Elk-Pi, I suggest the following procedure:
- make a copy of ``/usr/share/sushi-webui/installed_plugins.json``.
- add this entry to the copy under ``plugins``.``VST3 Synths``:
```json
"Sflight": {
  "name": "JuicySfHeadless",
  "type": "vst3x",
  "uid": "JuicySfHeadless",
  "path": "/home/mind/plugins/JuicySfHeadless.vst3",
  "role": "instrument"
}
```
replace ``/home/mind/plugins/JuicySfHeadless.vst3`` by the actual location where you uploaded the plugin (or use the same one).
- run ``sushi-webui --plugin-list path_to_your_copy_of/installed_plugins.json`` to load the web-ui with the updated plugin list.

## Limitations and issues

### Soundfont loading
There are still occurrences of soundfont_path string property not being restored in Sushi 1.4.0. It happens **way less frequently** in this version but it still does. 
My observation is that the notification ```ElkVST3Extensions::propertyValueChanged()``` can sometimes be missed by the plugin instance. Which hints at a race condition somewhere (my guess is that the notification may be fired before the plugin listener subscribed).
When this happens, the soundfont loading procedure simply does not run. 

### Host having stale parameter data after restoring
Some parameters update seems to be also missed by Sushi when querying its inner state with gRPC.

### Unclear program discovery procedure
Still under investigation.


### Aftertouch not implemented
I skipped aftertouch while implementing this as I have no device that supports this feature for testing. 
I may need to use Midish and build a small MIDI track to reimplement this properly.
This might prove painful without JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS...  


## Development 

To make a debug build (print loading soundfont attempts, success and failure):
### Compile 
```sh
# Configure
cmake -DCMAKE_BUILD_TYPE=Debug -B build -S .

# Build
cmake --build build --parallel --config Debug
```


## Credits 
This is a rework of Alex Birch plugin.
