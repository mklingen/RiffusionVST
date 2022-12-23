/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <fstream>

#define JucePlugin_IsSynth 1

//==============================================================================
RiffusionVSTAudioProcessor::RiffusionVSTAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
                       .withOutput ("Output", juce::AudioChannelSet::mono(), true)
                       )
    ,recordingBuffer(1, maxRecordingBufferSize)
{
    wavInterface.reset(new juce::WavAudioFormat());
    wavWriteBuffer.resize(maxRecordingBufferSize * sizeof(uint16_t), 0);
}

RiffusionVSTAudioProcessor::~RiffusionVSTAudioProcessor()
{
    // Kill the thread.
    std::lock_guard<std::mutex> lock(internetRequestMutex);
    if (internetRequestThread.joinable()) {
        internetRequestThread.join();
    }
}

//==============================================================================
const juce::String RiffusionVSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RiffusionVSTAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RiffusionVSTAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RiffusionVSTAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RiffusionVSTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RiffusionVSTAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RiffusionVSTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RiffusionVSTAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RiffusionVSTAudioProcessor::getProgramName (int index)
{
    return {};
}

void RiffusionVSTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void RiffusionVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    currentSampleRate = sampleRate;
    // Reset the sample rate if applicable.
    if (prevSampleRate != currentSampleRate) {
        prevSampleRate = currentSampleRate;
        maxRecordingBufferSize = static_cast<int>(maxRecordingBufferLengthSeconds * currentSampleRate);
        recordingBuffer = juce::AudioBuffer<float>(1, maxRecordingBufferSize);
    }
    hasAnyAudio = true;
}

void RiffusionVSTAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    hasAnyAudio = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RiffusionVSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

bool RiffusionVSTAudioProcessor::appendBlock(const juce::AudioBuffer<float>& input)
{
    // Append a block of data into the recording buffer.
    int numToCopy = std::min(maxRecordingBufferSize - recordingStartPtr, input.getNumSamples());
    recordingBuffer.copyFrom(0, recordingStartPtr, input.getReadPointer(0), numToCopy);
    recordingStartPtr += numToCopy;
    double s = recordingStartPtr / currentSampleRate;
    message = std::to_string(s) + "/" + std::to_string(maxRecordingBufferLengthSeconds);
    return recordingStartPtr < maxRecordingBufferSize;
}

void RiffusionVSTAudioProcessor::startRecording() {
    if (isPlaying) {
        stopPlaying();
    }
    isRecording = true;
    recordingStartPtr = 0;
    recordingBuffer.clear();
}

void RiffusionVSTAudioProcessor::stopRecording() {
    isRecording = false;
    message = "Stopped Recording";
    if (wavWriteBuffer.size() < maxRecordingBufferSize * sizeof(uint16_t)) {
        wavWriteBuffer.resize(maxRecordingBufferSize * sizeof(uint16_t), 0);
    }
    juce::MemoryOutputStream* memStream = new juce::MemoryOutputStream(maxRecordingBufferSize * sizeof(uint16_t));
    std::unique_ptr<juce::AudioFormatWriter> writer(wavInterface->createWriterFor(memStream, outputSampleRate, 1, 16, juce::StringPairArray(), 0));
    if (!writer) {
        delete memStream;
        return;
    }
    writer->writeFromAudioSampleBuffer(recordingBuffer, 0, recordingBuffer.getNumSamples());
    writer->flush();
    base64Wav = juce::Base64::toBase64(memStream->getData(), memStream->getDataSize());
}

void RiffusionVSTAudioProcessor::startPlaying() {
    if (isRecording) {
        stopRecording();
    }
    playbackStartPtr = 0;
    isPlaying = true;
    message = "Started Playing";
}

void RiffusionVSTAudioProcessor::stopPlaying() {
    isPlaying = false;
    message = "Stopped Playing";
}

void RiffusionVSTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    if (isPlaying) {
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            int samplesToEnd = recordingStartPtr - playbackStartPtr;
            if (samplesToEnd <= 0) {
                playbackStartPtr = 0;
                samplesToEnd = recordingBuffer.getNumSamples();
            }
            int minBufferSize = std::min(recordingBuffer.getNumSamples(),
                buffer.getNumSamples());
            int blockSize = std::min(std::min(minBufferSize, recordingStartPtr), samplesToEnd);
            buffer.copyFrom(channel, 0,
                recordingBuffer.getReadPointer(0) + playbackStartPtr,
                blockSize
            );
            playbackStartPtr += blockSize;

        }
    }
    else if (isRecording) {
        if (hasAnyAudio) {
            bool keepGoing = appendBlock(buffer);
            if (!keepGoing) {
                stopRecording();
            }
        }
        else {
            message = "Waiting...";
        }
    }
}

juce::URL RiffusionVSTAudioProcessor::buildURL(const RiffusionVSTAudioProcessor::ProcessParams& params) const {
    // Make a bunch of JSON and put it in a POST payload.
    juce::URL url(params.serverAddress);
    url = url.getChildURL("/run_vst/");

    juce::DynamicObject::Ptr jsonObject = new juce::DynamicObject(); // Apparently pointers are owned by var?
    jsonObject->setProperty("alpha", juce::var(params.alpha));
    jsonObject->setProperty("num_inference_steps", juce::var(params.numInferenceSteps));
    auto fillPrompt = [&params](juce::DynamicObject* json, const std::string& prompt)
    {
        json->setProperty("prompt", juce::var(prompt));
        json->setProperty("seed", juce::var(params.seed));
        json->setProperty("denoising", juce::var(params.denoising));
        json->setProperty("guidance", juce::var(params.guidance));
    };
    juce::DynamicObject::Ptr startJson = new juce::DynamicObject();
    fillPrompt(startJson.get(), params.promptA);
    juce::DynamicObject::Ptr endJson = new juce::DynamicObject();
    fillPrompt(endJson.get(), params.promptB);
    jsonObject->setProperty("start", juce::var(startJson.get()));
    jsonObject->setProperty("end", juce::var(endJson.get()));
    // The wav file bytes are literally just dumped into the POST data as a base64 string.
    jsonObject->setProperty("audio", juce::var(base64Wav));
    url = url.withPOSTData(juce::JSON::toString(juce::var(jsonObject.get()), true, 3));
    return url;
}

void RiffusionVSTAudioProcessor::startGenerating(const RiffusionVSTAudioProcessor::ProcessParams& params) {
    std::lock_guard<std::mutex> lock(internetRequestMutex);
    isGenerating = true;
    message = "Waiting...";
    if (internetRequestThread.joinable()) {
        internetRequestThread.join();
    }
    // Background thread that actually sends the request.
    internetRequestThread = std::thread([this, params]()
        {
            juce::String response;
            bool success = getHttpRequest(buildURL(params), &response);
            std::lock_guard<std::mutex> innerLock(internetRequestMutex);
            if (success) {
                message = "Done Generating";
                // Parse JSON from the server.
                juce::var jsonObject = juce::JSON::parse(response);
                if (jsonObject.hasProperty("audio")) {
                    const juce::var& audio = jsonObject["audio"];
                    if (audio.isString()) {
                        juce::String wave64 = audio.toString();
                        juce::MemoryOutputStream buffer(wavWriteBuffer.data(), wavWriteBuffer.size());
                        // Try to convert the payload into actual data we can read. This is a .wav file.
                        if (juce::Base64::convertFromBase64(buffer, wave64)) {
                            // Create a virtual wave file file handle and try to read it.
                            juce::MemoryBlock block(wavWriteBuffer.data(), wavWriteBuffer.size());
                            juce::MemoryInputStream* inputBuffer = new juce::MemoryInputStream(block, false);
         
                            std::unique_ptr<juce::AudioFormatReader> reader(wavInterface->createReaderFor(inputBuffer, true));
                            if (reader) {
                                reader->read(&recordingBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, false);
                            }
                            else {
                                message = "Failed to read memory for WAV file.";
                                delete inputBuffer;
                            }
                        }
                        else {
                            message = "Failed to convert audio data.";
                        }
                    }
                }
            }
            else {
                message = response.toStdString();
            }
            isGenerating = false;
        });
}

void RiffusionVSTAudioProcessor::stopGenerating() {
    std::lock_guard<std::mutex> lock(internetRequestMutex);
    if (internetRequestThread.joinable()) {
        internetRequestThread.join();
    }
    isGenerating = false;
    message = "";
}

bool RiffusionVSTAudioProcessor::getHttpRequest(const juce::URL& url, juce::String* content) {
    // Does the entire HTTP POST request to the server. Returns the content as a string.
    juce::StringPairArray responseHeaders;
    juce::String extraHeader = "Accept: application/json\r\n"
                                "Content-Type: application/json\r\n"
                                "Sec-Fetch-Mode: cors\r\n";
    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
        .withExtraHeaders(extraHeader)
        .withConnectionTimeoutMs(timeoutRequestMs)
        .withStatusCode(&statusCode)
        .withNumRedirectsToFollow(32);
    std::unique_ptr<juce::InputStream> stream = url.createInputStream(options);
    if (stream != nullptr) {
        *content = stream->readEntireStreamAsString();
        return true;
    }

    if (statusCode != 0) {
        *content = "Failed to connect, status code = " + juce::String(statusCode);
        return false;
    }

    *content = "Failed to connect!";
    return false;
}

//==============================================================================
bool RiffusionVSTAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RiffusionVSTAudioProcessor::createEditor()
{
    return new RiffusionVSTAudioProcessorEditor (*this);
}

//==============================================================================
void RiffusionVSTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void RiffusionVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RiffusionVSTAudioProcessor();
}
