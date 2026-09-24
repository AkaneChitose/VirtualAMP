#pragma once

#include <JuceHeader.h>

class AudioEngine
{
public:
    // ============================================================
    // Constructor / Destructor
    // ============================================================

    AudioEngine();
    ~AudioEngine();

    // ============================================================
    // File
    // ============================================================

    bool loadFile(const juce::File& file);

    // ============================================================
    // Playback
    // ============================================================

    void play();
    void pause();
    void stop();

    bool isPlaying() const;

    void setPosition(double seconds);

    double getPosition() const;
    double getLength() const;

    double getFileLengthInSeconds(const juce::File& file);
    // ============================================================
    // Volume
    // ============================================================

    void setVolume(float newVolume);
    float getVolume() const;

    void setMuted(bool shouldMute);
    bool isMuted() const;

    // ============================================================
    // Spectrum
    // ============================================================

    void copySpectrumData(
        float* destination,
        int numberOfBins) const;

    // ============================================================
    // Waveform
    // ============================================================

    juce::AudioThumbnail& getThumbnail();

    // ============================================================
    // Track metadata
    // ============================================================

    juce::String getTrackTitle() const;
    juce::String getTrackArtist() const;
    juce::String getTrackAlbum() const;

    // ============================================================
    // Album art
    // ============================================================

    const juce::Image& getAlbumArt() const;

    // ============================================================
    // Audio device
    // ============================================================

    juce::AudioDeviceManager& getDeviceManager();

    juce::String getCurrentDeviceName() const;
    juce::String getCurrentDeviceType() const;

    double getCurrentSampleRate() const;
    int getCurrentBufferSize() const;

    bool isAudioDeviceOpen() const;

    // ============================================================
    // WASAPI
    // ============================================================

    juce::String setWASAPIShared();

    juce::String setWASAPISharedLowLatency();

    juce::String setWASAPIExclusive();

private:

    // ============================================================
    // AnalyserSource
    // ============================================================

    class AnalyserSource
        : public juce::PositionableAudioSource
    {
    public:

        AnalyserSource(
            juce::PositionableAudioSource& sourceToUse,
            AudioEngine& ownerToUse);

        // --------------------------------------------------------
        // AudioSource
        // --------------------------------------------------------

        void prepareToPlay(
            int samplesPerBlockExpected,
            double sampleRate) override;

        void releaseResources() override;

        void getNextAudioBlock(
            const juce::AudioSourceChannelInfo& bufferToFill) override;

        // --------------------------------------------------------
        // PositionableAudioSource
        // --------------------------------------------------------

        void setNextReadPosition(
            juce::int64 newPosition) override;

        juce::int64 getNextReadPosition() const override;

        juce::int64 getTotalLength() const override;

        bool isLooping() const override;

        // --------------------------------------------------------
        // Spectrum
        // --------------------------------------------------------

        void copySpectrumData(
            float* destination,
            int numberOfBins) const;

    private:

        // ========================================================
        // Source
        // ========================================================

        juce::PositionableAudioSource& source;

        AudioEngine& owner;

        // ========================================================
        // FFT
        // ========================================================

        static constexpr int fftSize = 4096;

        juce::dsp::FFT fft{
            static_cast<int>(
                std::log2(fftSize))
        };

        juce::dsp::WindowingFunction<float> window{
            fftSize,
            juce::dsp::WindowingFunction<float>::hann
        };

        // ========================================================
        // FFT buffer
        // ========================================================

        std::array<float, fftSize * 2> fftBuffer{};

        int fftBufferPosition = 0;

        // ========================================================
        // Double spectrum buffer
        // ========================================================

        std::array<float, fftSize / 2> spectrumBufferA{};

        std::array<float, fftSize / 2> spectrumBufferB{};

        std::array<
            std::array<float, fftSize / 2>,
            2
        > spectrumBuffers{
            spectrumBufferA,
            spectrumBufferB
        };

        std::atomic<int> activeSpectrumBuffer{ 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
            AnalyserSource)
    };

    // ============================================================
    // Audio device
    // ============================================================

    juce::AudioDeviceManager audioDeviceManager;

    juce::AudioSourcePlayer audioSourcePlayer;

    // ============================================================
    // Format manager
    // ============================================================

    juce::AudioFormatManager formatManager;

    // ============================================================
    // Reader
    // ============================================================

    std::unique_ptr<
        juce::AudioFormatReader
    > reader;

    // ============================================================
    // Reader source
    // ============================================================

    std::unique_ptr<
        juce::AudioFormatReaderSource
    > readerSource;

    // ============================================================
    // Transport
    // ============================================================

    juce::AudioTransportSource transportSource;

    // ============================================================
    // Analyser
    // IMPORTANT:
    // This is an OBJECT, NOT unique_ptr
    // ============================================================

    AnalyserSource analyserSource;

    // ============================================================
    // Waveform thumbnail
    // ============================================================

    juce::AudioThumbnailCache thumbnailCache;

    juce::AudioThumbnail thumbnail;

    // ============================================================
    // Metadata
    // ============================================================

    juce::String trackTitle{
        "No track loaded"
    };

    juce::String trackArtist{
        "Unknown Artist"
    };

    juce::String trackAlbum{
        "Unknown Album"
    };

    // ============================================================
    // Album art
    // ============================================================

    juce::Image albumArt;

    // ============================================================
    // Volume
    // ============================================================

    float volumeBeforeMute = 1.0f;

    bool muted = false;

    // ============================================================
    // FLAC metadata
    // ============================================================

    juce::StringPairArray readFlacMetadata(
        const juce::File& file) const;

    // ============================================================
    // FLAC album art
    // ============================================================

    juce::Image readFlacAlbumArt(
        const juce::File& file) const;

    // ============================================================
    // WASAPI configuration
    // ============================================================

    juce::String configureWASAPI(
        juce::WASAPIDeviceMode mode);

    // ============================================================
    // Helper
    // ============================================================

    bool isFlacFile(
        const juce::File& file) const;

    // ============================================================
    // Non-copyable
    // ============================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        AudioEngine)
};