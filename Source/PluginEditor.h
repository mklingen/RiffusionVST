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
class RiffusionVSTAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::ChangeListener
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
    class LambdaTimer : public juce::Timer {
        public:
            LambdaTimer(std::function<void()> lambda);
            virtual void timerCallback() override;
            std::function<void()> m_lambda;
    };
    LambdaTimer updateTimer;
    void onUpdate();
    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    struct AudioThumbnailWidget {
        juce::AudioThumbnail thumbnail;
        juce::Rectangle<int> bounds;
        void paint(juce::Graphics& g);
    };
    // Waveform of the recording buffer.
    AudioThumbnailWidget recordingThumbnail;
    // Waveform of the generation buffer.
    AudioThumbnailWidget generatedThumbnail;
    // This is just a number indicating a kind of change counter to the thumbnails. Gets
    // incremented every time we want to change the thumbnail.
    int thumbHash = 0;
    void updateThumbnails();

    void reconcileUIState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiffusionVSTAudioProcessorEditor)
};
