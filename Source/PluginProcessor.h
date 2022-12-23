/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class RiffusionVSTAudioProcessor : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
    , public juce::AudioProcessorARAExtension
#endif
{
public:
    struct ProcessParams
    {
        std::string serverAddress;
        std::string promptA;
        std::string promptB;
        float alpha;
        float denoising;
        float guidance;
        int seed;
        int numInferenceSteps;
    };

    //==============================================================================
    RiffusionVSTAudioProcessor();
    ~RiffusionVSTAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Appends a new block of data to the recording buffer. Returns true if we've reached
    // the max buffer size, or false otherwise.
    bool appendBlock(const juce::AudioBuffer<float>& input);
    
    // Start and stop recording live audio.
    void startRecording();
    void stopRecording();
    // Start and stop playing whatever is in the buffer.
    void startPlaying();
    void stopPlaying();
    
    bool getIsRecording() const { return isRecording;  }
    bool getIsPlaying() const { return isPlaying; }
    bool getIsGenerating() const { return isGenerating; }
    // Start and stop the generation proccess.
    void startGenerating(const ProcessParams& params);
    void stopGenerating();

    // Message displayed in the bottom.
    std::string message = "";

private:
    juce::URL buildURL(const ProcessParams& params) const;
    bool getHttpRequest(const juce::URL& url, juce::String* content);
    // If true, generation thread is running.
    bool isGenerating = false;
    // Background thread that handles the generation.
    std::thread internetRequestThread;
    // Mutex for the recording buffer overwritten by generation thread.
    std::mutex internetRequestMutex;
    // If false, haven't even setup audio channels yet.
    bool hasAnyAudio = false;
    // Maintain sample rate. If the DAW messes with the sample rate,
    // we might still be able to get something working by communicating
    // with Riffusion in 44100 only.
    double prevSampleRate = 44100;
    double currentSampleRate = 44100;
    const double outputSampleRate = 44100;
    // Timeout to riffusion request.
    const int timeoutRequestMs = 60000;
    // If true, we are recording live audio.
    bool isRecording = false;
    // If true, we are playing back generated audio.
    bool isPlaying = false;
    double maxRecordingBufferLengthSeconds = 5.00;
    int maxRecordingBufferSize = 220500; // 5.00 seconds at 44100 hz.
    // Single buffer of samples that we are recording to.
    juce::AudioBuffer<float> recordingBuffer;
    // Buffer of data that the generated waveform is written to.
    std::vector<uint8_t> wavWriteBuffer;
    // A string containing base64 encoded ascii data of the wav file. It is literally
    // the bytes of a wav file written in base 64 format.
    juce::String base64Wav;
    // Interface for reading and writing wav files.
    std::unique_ptr<juce::WavAudioFormat> wavInterface;
    // Sample where we are currently vomiting wav data into the buffer.
    int recordingStartPtr = 0;
    // Sample where we are currently outputting the audio data from the buffer.
    int playbackStartPtr = 0;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiffusionVSTAudioProcessor)
};
