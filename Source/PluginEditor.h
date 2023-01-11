/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class RiffusionVSTAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    RiffusionVSTAudioProcessorEditor (RiffusionVSTAudioProcessor&);
    ~RiffusionVSTAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    RiffusionVSTAudioProcessor& audioProcessor;

    enum class RecordingState
    {
        Idle,
        Recording,
        Generating,
        Playing
    };

    void onGenerateClicked();
    void onRecordClicked();
    void onPlayRecordingClicked();
    void onPlayGenerationClicked();

    juce::TextEditor serverIp;
    juce::TextEditor prompt1Text;
    juce::TextEditor prompt2Text;
    juce::TextEditor seedText;
    juce::Slider alphaSlider;
    juce::Slider strengthSlider;
    juce::Slider denoisingSlider;
    juce::Slider itersSlider;
    juce::TextButton recordButton;
    juce::TextButton playbackRecordingButton;
    juce::TextButton generateButton;
    juce::TextButton playbackGenerationButton;
    juce::DrawableText messageText;
    RecordingState state = RecordingState::Idle;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiffusionVSTAudioProcessorEditor)
};
