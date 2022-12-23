# RiffusionVST
 A VST plugin for Riffusion based on JUCE

## Notes on Use
This is an experimental VST plugin for interfacing with Riffusion (https://github.com/riffusion/riffusion). You must be able to run Riffusion locally using mklingen's special branch (https://github.com/mklingen/riffusion-inference/). Then, this will generate a VST3 plugin for you, and a standalone app.

As of writing, this DOES NOT WORK with standard riffusion servers, you must be using mklingen's special branch.

![screenshot in trackiton waveform](screenshot.png)
> The above is a screenshot of the plugin running in trackiton waveform.

## Install
1. Download mklingen's special branch, and run the vst server after the lengthy install steps, getting torch setup, conda, etc. https://github.com/mklingen/riffusion-inference
2. Download and install [juce](https://juce.com/get-juce/download), including Projucer.
3. Build this plugin in visual studio after using projucer.
4. A plugin will be generated in the `Builds` folder.
5. Add the plugin to your favorite DAW.
6. Run the Riffusion server locally (or, if you have some powerful build machine somewhere, run it there).
7. Point the plugin at the IP address of your riffusion server with port 3000 (if running locally, you won't have to do anything).

## Use
Once everything is running, you can first press "RECORD" to record up to 5 seconds of audio into the VSTs input. You can then hit "generate" to generate audio based on the recording, then "play" to loop the generated audio. To record it, pipe the output of the VST to a send line in your DAW.