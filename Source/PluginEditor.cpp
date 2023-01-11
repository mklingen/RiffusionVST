/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RiffusionVSTAudioProcessorEditor::RiffusionVSTAudioProcessorEditor (RiffusionVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 500);
    serverIp.setText("http://127.0.0.1:3000");
    prompt1Text.setText("prompt 1");
    prompt2Text.setText("prompt 2");
    generateButton.setButtonText("Generate New");
    generateButton.onClick = [this]() {
        onGenerateClicked();
    };

    recordButton.setButtonText("Record");
    recordButton.onClick = [this]() {
        onRecordClicked();
    };

    playbackRecordingButton.setButtonText("Play Recorded");
    playbackRecordingButton.onClick = [this]() {
        onPlayRecordingClicked();
    };

    playbackGenerationButton.setButtonText("Play Generated");
    playbackGenerationButton.onClick = [this]() {
        onPlayGenerationClicked();
    };

    alphaSlider.setTextValueSuffix(" Blend");
    alphaSlider.setValue(0.5);
    alphaSlider.setRange(0.0, 1.0, 0.1);
    strengthSlider.setTextValueSuffix(" Prompt Strength");
    strengthSlider.setValue(7.0);
    strengthSlider.setRange(0.0, 25.0, 0.1);
    denoisingSlider.setTextValueSuffix(" Denoising");
    denoisingSlider.setValue(0.7);
    denoisingSlider.setRange(0.0, 1.0, 0.05);
    seedText.setText("seed");
    itersSlider.setTextValueSuffix(" Iters");
    itersSlider.setRange(1, 100, 1.0);
    itersSlider.setValue(50);
    messageText.setText("");
    messageText.setColour(juce::Colour(255, 255, 255));
    messageText.setJustification(juce::Justification::centred);

    addAndMakeVisible(&serverIp);
    addAndMakeVisible(&prompt1Text);
    addAndMakeVisible(&prompt2Text);
    addAndMakeVisible(&alphaSlider);
    addAndMakeVisible(&strengthSlider);
    addAndMakeVisible(&denoisingSlider);
    addAndMakeVisible(&itersSlider);
    addAndMakeVisible(&seedText);
    addAndMakeVisible(&recordButton);
    addAndMakeVisible(&playbackRecordingButton);
    addAndMakeVisible(&generateButton);
    addAndMakeVisible(&playbackGenerationButton);
    addAndMakeVisible(&messageText);
}

void RiffusionVSTAudioProcessorEditor::onGenerateClicked() {
    if (state != RecordingState::Generating) {
        state = RecordingState::Generating;
        recordButton.setEnabled(false);
        playbackRecordingButton.setEnabled(false);
        playbackGenerationButton.setEnabled(false);
        generateButton.setButtonText("Stop");
        RiffusionVSTAudioProcessor::ProcessParams params;
        params.alpha = alphaSlider.getValue();
        params.denoising = denoisingSlider.getValue();
        params.guidance = strengthSlider.getValue();
        params.numInferenceSteps = static_cast<int>(itersSlider.getValue());
        params.promptA = prompt1Text.getText().toStdString();
        params.promptB = prompt2Text.getText().toStdString();
        params.serverAddress = serverIp.getText().toStdString();
        params.seed = static_cast<int>(std::hash<std::string>()(seedText.getText().toStdString()));
        audioProcessor.startGenerating(params);
    }
    else {
        state = RecordingState::Idle;
        recordButton.setEnabled(true);
        playbackRecordingButton.setEnabled(true);
        playbackGenerationButton.setEnabled(true);
        generateButton.setButtonText("Generate");
    }
}

void RiffusionVSTAudioProcessorEditor::onRecordClicked() {
    if (state == RecordingState::Recording) {
        state = RecordingState::Idle;
        recordButton.setButtonText("Record");
        playbackRecordingButton.setEnabled(true);
        playbackGenerationButton.setEnabled(true);
        generateButton.setEnabled(true);
        audioProcessor.stopRecording();
    } else {
        state = RecordingState::Recording;
        recordButton.setButtonText("Stop");
        playbackRecordingButton.setEnabled(false);
        playbackGenerationButton.setEnabled(false);
        generateButton.setEnabled(false);
        audioProcessor.startRecording();
    }
}

void RiffusionVSTAudioProcessorEditor::onPlayRecordingClicked() {
    if (state != RecordingState::Playing) {
        state = RecordingState::Playing;
        recordButton.setEnabled(false);
        generateButton.setEnabled(false);
        playbackGenerationButton.setEnabled(false);
        playbackRecordingButton.setEnabled(true);
        playbackRecordingButton.setButtonText("Stop");
        audioProcessor.startPlaying(RiffusionVSTAudioProcessor::PlayState::PlayingRecorded);
    }
    else {
        state = RecordingState::Idle;
        recordButton.setEnabled(true);
        generateButton.setEnabled(true);
        playbackGenerationButton.setEnabled(true);
        playbackRecordingButton.setEnabled(true);
        playbackRecordingButton.setButtonText("Play");
        audioProcessor.stopPlaying();
    }
}
void RiffusionVSTAudioProcessorEditor::onPlayGenerationClicked() {
    if (state != RecordingState::Playing) {
        state = RecordingState::Playing;
        recordButton.setEnabled(false);
        generateButton.setEnabled(false);
        playbackRecordingButton.setEnabled(false);
        playbackGenerationButton.setButtonText("Stop");
        audioProcessor.startPlaying(RiffusionVSTAudioProcessor::PlayState::PlayingGenerated);
    }
    else {
        state = RecordingState::Idle;
        recordButton.setEnabled(true);
        generateButton.setEnabled(true);
        playbackRecordingButton.setEnabled(true);
        playbackGenerationButton.setButtonText("Play");
        audioProcessor.stopPlaying();
    }
}

RiffusionVSTAudioProcessorEditor::~RiffusionVSTAudioProcessorEditor()
{
}

//==============================================================================
void RiffusionVSTAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(19.0f);
    g.drawText("Riffusion VST", 0, 0, 300, 250, 30, juce::Justification::left);
    g.setFont(15.0f);
    g.drawFittedText("Server IP: ", serverIp.getPosition().x - 120, serverIp.getPosition().y, 100, 30, juce::Justification::right, 1);
    messageText.setText(audioProcessor.message);

    if (!audioProcessor.getIsRecording() && state == RecordingState::Recording) {
        state = RecordingState::Idle;
        recordButton.setButtonText("Record");
        playbackRecordingButton.setEnabled(true);
        generateButton.setEnabled(true);
    }
    else if (audioProcessor.getPlayState() == RiffusionVSTAudioProcessor::PlayState::NotPlaying &&
               state == RecordingState::Playing) {
        state = RecordingState::Idle;
        recordButton.setEnabled(true);
        generateButton.setEnabled(true);
        playbackRecordingButton.setButtonText("Play Recorded");
        playbackGenerationButton.setButtonText("Play Generated");
    }
    else if (!audioProcessor.getIsGenerating() && state == RecordingState::Generating) {
        state = RecordingState::Idle;
        recordButton.setEnabled(true);
        playbackRecordingButton.setEnabled(true);
        playbackGenerationButton.setEnabled(true);
        generateButton.setEnabled(true);
        generateButton.setButtonText("Generate New");
    }
}

void RiffusionVSTAudioProcessorEditor::resized()
{
    constexpr int xPadding = 15;
    constexpr int yPadding = 15;
    constexpr int elementHeight = 30;
    constexpr  int minWidth = 125;
    const int w = getWidth();
    const int h = getHeight();
    const int r = w - xPadding / 2;
    constexpr  int l = xPadding / 2;
    constexpr  int row_padding = 4;
    auto row = [=](int r_idx) { return yPadding + r_idx * (elementHeight + row_padding); };
    int curr_row_idx = 0;
    auto next_row = [&curr_row_idx, &row]() {
        int rowPx = row(curr_row_idx);
        curr_row_idx++; 
        return rowPx;
    };
    // sets the position and size of the slider with arguments (x, y, width, height)
    serverIp.setBounds(w - xPadding - minWidth, next_row(), minWidth, elementHeight);
    seedText.setBounds(l, next_row(), r, elementHeight);
    prompt1Text.setBounds(l, next_row(), r, elementHeight);
    prompt2Text.setBounds(l, next_row(), r, elementHeight);
    alphaSlider.setBounds(l, next_row(), r, elementHeight);
    strengthSlider.setBounds(l, next_row(), r, elementHeight);
    denoisingSlider.setBounds(l, next_row(), r, elementHeight);
    itersSlider.setBounds(l, next_row(), r, elementHeight);
    int recording_row = next_row();
    recordButton.setBounds(l, recording_row, r / 2, elementHeight);
    playbackRecordingButton.setBounds(l + r / 2, recording_row, r / 2, elementHeight);
    int gen_row = next_row();
    generateButton.setBounds(l, gen_row, r / 2, elementHeight);
    playbackGenerationButton.setBounds(l + r / 2, gen_row, r / 2, elementHeight);
    messageText.setBoundingBox(juce::Parallelogram(juce::Rectangle<float>(l, next_row(), r, elementHeight)));
}
