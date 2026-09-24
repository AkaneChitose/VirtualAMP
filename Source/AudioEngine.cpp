#include "AudioEngine.h"

// ============================================================
// Constructor
// ============================================================

AudioEngine::AudioEngine()
    : analyserSource(transportSource, *this),
    thumbnailCache(5),
    thumbnail(
        512,
        formatManager,
        thumbnailCache)
{
    formatManager.registerBasicFormats();

    audioDeviceManager.initialise(
        2,
        2,
        nullptr,
        true);

    audioSourcePlayer.setSource(&analyserSource);

    audioDeviceManager.addAudioCallback(
        &audioSourcePlayer);
}

// ============================================================
// Destructor
// ============================================================

AudioEngine::~AudioEngine()
{
    transportSource.stop();

    audioSourcePlayer.setSource(nullptr);

    audioDeviceManager.removeAudioCallback(
        &audioSourcePlayer);

    transportSource.setSource(nullptr);

    readerSource.reset();

    audioDeviceManager.closeAudioDevice();
}

// ============================================================
// FLAC Vorbis Comment reader
// ============================================================

juce::StringPairArray AudioEngine::readFlacMetadata(
    const juce::File& file) const
{
    juce::StringPairArray result;

    if (!file.existsAsFile())
        return result;

    if (!file.hasFileExtension(".flac"))
        return result;

    juce::FileInputStream input(file);

    if (!input.openedOk())
        return result;

    char signature[4]{};

    if (input.read(signature, 4) != 4)
        return result;

    if (std::memcmp(
        signature,
        "fLaC",
        4) != 0)
    {
        return result;
    }

    const auto readU8 =
        [&input]() -> uint8_t
        {
            return static_cast<uint8_t>(
                input.readByte());
        };

    const auto readLittleEndian32 =
        [&readU8]() -> uint32_t
        {
            const uint32_t b0 = readU8();
            const uint32_t b1 = readU8();
            const uint32_t b2 = readU8();
            const uint32_t b3 = readU8();

            return
                b0 |
                (b1 << 8) |
                (b2 << 16) |
                (b3 << 24);
        };

    bool reachedLastBlock = false;

    while (!reachedLastBlock &&
        !input.isExhausted())
    {
        const uint8_t header =
            readU8();

        reachedLastBlock =
            (header & 0x80) != 0;

        const uint8_t blockType =
            header & 0x7F;

        const uint8_t size0 =
            readU8();

        const uint8_t size1 =
            readU8();

        const uint8_t size2 =
            readU8();

        const uint32_t blockSize =
            (static_cast<uint32_t>(size0) << 16) |
            (static_cast<uint32_t>(size1) << 8) |
            static_cast<uint32_t>(size2);

        const auto blockDataStart =
            input.getPosition();

        // ====================================================
        // VORBIS_COMMENT
        // ====================================================

        if (blockType == 4)
        {
            if (blockSize < 8)
            {
                input.setPosition(
                    blockDataStart + blockSize);

                continue;
            }

            const uint32_t vendorLength =
                readLittleEndian32();

            if (vendorLength >
                blockSize - 4)
            {
                break;
            }

            const auto vendorEnd =
                input.getPosition() +
                static_cast<juce::int64>(
                    vendorLength);

            if (!input.setPosition(vendorEnd))
                break;

            if (input.getPosition() + 4 >
                blockDataStart + blockSize)
            {
                break;
            }

            const uint32_t commentCount =
                readLittleEndian32();

            if (commentCount > 10000)
                break;

            for (uint32_t i = 0;
                i < commentCount;
                ++i)
            {
                if (input.getPosition() + 4 >
                    blockDataStart + blockSize)
                {
                    break;
                }

                const uint32_t commentLength =
                    readLittleEndian32();

                if (commentLength == 0)
                    continue;

                if (commentLength >
                    1024u * 1024u)
                {
                    break;
                }

                if (input.getPosition() +
                    static_cast<juce::int64>(
                        commentLength)
                    >
                    blockDataStart + blockSize)
                {
                    break;
                }

                juce::MemoryBlock commentData;

                if (!input.readIntoMemoryBlock(
                    commentData,
                    static_cast<size_t>(
                        commentLength)))
                {
                    break;
                }

                const auto* data =
                    static_cast<const char*>(
                        commentData.getData());

                juce::String comment =
                    juce::String::fromUTF8(
                        data,
                        static_cast<int>(
                            commentData.getSize()));

                const int equalsPosition =
                    comment.indexOfChar('=');

                if (equalsPosition <= 0)
                    continue;

                juce::String key =
                    comment.substring(
                        0,
                        equalsPosition)
                    .trim()
                    .toUpperCase();

                juce::String value =
                    comment.substring(
                        equalsPosition + 1)
                    .trim();

                if (key.isEmpty())
                    continue;

                result.set(
                    key,
                    value);
            }
        }

        // ====================================================
        // Skip current block
        // ====================================================

        const auto nextBlockPosition =
            blockDataStart +
            static_cast<juce::int64>(
                blockSize);

        if (!input.setPosition(
            nextBlockPosition))
        {
            break;
        }
    }

    return result;
}

// ============================================================
// FLAC Album Art reader
// ============================================================

juce::Image AudioEngine::readFlacAlbumArt(
    const juce::File& file) const
{
    juce::Image result;

    if (!file.existsAsFile())
        return result;

    if (!file.hasFileExtension(".flac"))
        return result;

    juce::FileInputStream input(file);

    if (!input.openedOk())
        return result;

    // ========================================================
    // FLAC signature
    // ========================================================

    char signature[4]{};

    if (input.read(signature, 4) != 4)
        return result;

    if (std::memcmp(
        signature,
        "fLaC",
        4) != 0)
    {
        return result;
    }

    const auto readU8 =
        [&input]() -> uint8_t
        {
            return static_cast<uint8_t>(
                input.readByte());
        };

    const auto readBE24 =
        [&readU8]() -> uint32_t
        {
            const uint32_t b0 = readU8();
            const uint32_t b1 = readU8();
            const uint32_t b2 = readU8();

            return
                (b0 << 16) |
                (b1 << 8) |
                b2;
        };

    const auto readBE32 =
        [&readU8]() -> uint32_t
        {
            const uint32_t b0 = readU8();
            const uint32_t b1 = readU8();
            const uint32_t b2 = readU8();
            const uint32_t b3 = readU8();

            return
                (b0 << 24) |
                (b1 << 16) |
                (b2 << 8) |
                b3;
        };

    const auto readLE32 =
        [&readU8]() -> uint32_t
        {
            const uint32_t b0 = readU8();
            const uint32_t b1 = readU8();
            const uint32_t b2 = readU8();
            const uint32_t b3 = readU8();

            return
                b0 |
                (b1 << 8) |
                (b2 << 16) |
                (b3 << 24);
        };

    bool reachedLastBlock = false;

    while (!reachedLastBlock &&
        !input.isExhausted())
    {
        const uint8_t header =
            readU8();

        reachedLastBlock =
            (header & 0x80) != 0;

        const uint8_t blockType =
            header & 0x7F;

        const uint32_t blockSize =
            readBE24();

        const auto blockStart =
            input.getPosition();

        // ====================================================
        // PICTURE block
        //
        // FLAC metadata block type 6
        // ====================================================

        if (blockType == 6)
        {
            if (blockSize < 32)
                break;

            // ------------------------------------------------
            // Picture type
            // ------------------------------------------------

            const uint32_t pictureType =
                readBE32();

            juce::ignoreUnused(
                pictureType);

            // ------------------------------------------------
            // MIME type
            // ------------------------------------------------

            const uint32_t mimeLength =
                readBE32();

            if (mimeLength > blockSize)
                break;

            if (mimeLength > 1024)
                break;

            juce::MemoryBlock mimeData;

            if (!input.readIntoMemoryBlock(
                mimeData,
                static_cast<size_t>(
                    mimeLength)))
            {
                break;
            }

            // ------------------------------------------------
            // Description
            // ------------------------------------------------

            if (input.getPosition() + 4 >
                blockStart + blockSize)
            {
                break;
            }

            const uint32_t descriptionLength =
                readBE32();

            if (descriptionLength >
                blockSize)
            {
                break;
            }

            if (descriptionLength > 1024 * 1024)
                break;

            juce::MemoryBlock descriptionData;

            if (!input.readIntoMemoryBlock(
                descriptionData,
                static_cast<size_t>(
                    descriptionLength)))
            {
                break;
            }

            // ------------------------------------------------
            // Width
            // Height
            // Color depth
            // Number of colors
            // ------------------------------------------------

            if (input.getPosition() + 16 >
                blockStart + blockSize)
            {
                break;
            }

            const uint32_t width =
                readBE32();

            const uint32_t height =
                readBE32();

            const uint32_t colorDepth =
                readBE32();

            const uint32_t indexedColors =
                readBE32();

            juce::ignoreUnused(
                width,
                height,
                colorDepth,
                indexedColors);

            // ------------------------------------------------
            // Picture data length
            // ------------------------------------------------

            if (input.getPosition() + 4 >
                blockStart + blockSize)
            {
                break;
            }

            const uint32_t pictureDataLength =
                readBE32();

            // ------------------------------------------------
            // Validate image size
            // ------------------------------------------------

            const auto bytesRemaining =
                blockStart +
                static_cast<juce::int64>(
                    blockSize)
                -
                input.getPosition();

            if (pictureDataLength == 0)
                break;

            if (static_cast<juce::int64>(
                pictureDataLength)
                > bytesRemaining)
            {
                break;
            }

            if (pictureDataLength >
                64u * 1024u * 1024u)
            {
                break;
            }

            // ------------------------------------------------
            // Read embedded image
            // ------------------------------------------------

            juce::MemoryBlock imageData;

            if (!input.readIntoMemoryBlock(
                imageData,
                static_cast<size_t>(
                    pictureDataLength)))
            {
                break;
            }

            if (imageData.getSize() == 0)
                break;

            // ------------------------------------------------
            // Decode JPEG / PNG / etc.
            // ------------------------------------------------

            result =
                juce::ImageFileFormat::loadFrom(
                    imageData.getData(),
                    imageData.getSize());

            if (result.isValid())
            {
                DBG(
                    "FLAC ALBUM ART: "
                    + juce::String(
                        result.getWidth())
                    + "x"
                    + juce::String(
                        result.getHeight())
                );

                return result;
            }

            DBG(
                "FLAC PICTURE block found, "
                "but image could not be decoded."
            );
        }

        // ====================================================
        // Skip block
        // ====================================================

        const auto nextBlockPosition =
            blockStart +
            static_cast<juce::int64>(
                blockSize);

        if (!input.setPosition(
            nextBlockPosition))
        {
            break;
        }
    }

    return result;
}

// ============================================================
// Load file
// ============================================================

bool AudioEngine::loadFile(
    const juce::File& file)
{
    // ========================================================
    // Validate file
    // ========================================================

    if (!file.existsAsFile())
        return false;

    // ========================================================
    // Stop current playback
    // ========================================================

    transportSource.stop();

    // ========================================================
    // Detach old reader
    // ========================================================

    transportSource.setSource(nullptr);

    readerSource.reset();

    // ========================================================
    // Clear old metadata / album art
    // ========================================================

    trackTitle = "No track loaded";
    trackArtist = "Unknown Artist";
    trackAlbum = "Unknown Album";

    albumArt = juce::Image();

    // ========================================================
    // Create reader
    // ========================================================

    auto* fileReader =
        formatManager.createReaderFor(file);

    if (fileReader == nullptr)
        return false;

    // ========================================================
    // Validate reader
    // ========================================================

    if (fileReader->sampleRate <= 0.0 ||
        fileReader->lengthInSamples <= 0)
    {
        delete fileReader;
        return false;
    }

    // ========================================================
    // Metadata
    // ========================================================

    juce::StringPairArray metadata =
        fileReader->metadataValues;

    // ========================================================
    // FLAC metadata
    // ========================================================

    if (file.hasFileExtension(".flac"))
    {
        const auto flacMetadata =
            readFlacMetadata(file);

        if (flacMetadata.size() > 0)
            metadata = flacMetadata;

        // ====================================================
        // Album Art
        // ====================================================

        albumArt =
            readFlacAlbumArt(file);
    }


    const auto keys =
        metadata.getAllKeys();

    const auto values =
        metadata.getAllValues();

    for (int i = 0;
        i < metadata.size();
        ++i)
    {
        DBG(
            "METADATA ["
            + juce::String(i)
            + "] "
            + keys[i]
            + " = "
            + values[i]
        );
    }

    if (albumArt.isValid())
    {
        DBG(
            "ALBUM ART: "
            + juce::String(
                albumArt.getWidth())
            + "x"
            + juce::String(
                albumArt.getHeight())
        );
    }
    else
    {
        DBG("ALBUM ART: NONE");
    }

    DBG("================================================");

    // ========================================================
    // Title
    // ========================================================

    trackTitle =
        metadata.getValue(
            "TITLE",
            {})
        .trim();

    // ========================================================
    // Artist
    // ========================================================

    trackArtist =
        metadata.getValue(
            "ARTIST",
            {})
        .trim();

    // ========================================================
    // Album
    // ========================================================

    trackAlbum =
        metadata.getValue(
            "ALBUM",
            {})
        .trim();

    // ========================================================
    // Fallback
    // ========================================================

    if (trackTitle.isEmpty())
    {
        trackTitle =
            file.getFileNameWithoutExtension();
    }

    if (trackArtist.isEmpty())
    {
        trackArtist =
            "Unknown Artist";
    }

    if (trackAlbum.isEmpty())
    {
        trackAlbum =
            "Unknown Album";
    }

    // ========================================================
    // Create reader source
    // ========================================================

    readerSource =
        std::make_unique<
        juce::AudioFormatReaderSource>(
            fileReader,
            true);

    // ========================================================
    // Attach reader source
    // ========================================================

    transportSource.setSource(
        readerSource.get(),
        0,
        nullptr,
        fileReader->sampleRate);

    // ========================================================
    // Waveform
    // ========================================================

    thumbnail.clear();

    thumbnail.setSource(
        new juce::FileInputSource(file));

    // ========================================================
    // Reset position
    // ========================================================

    transportSource.setPosition(0.0);

    // ========================================================
    // Restore volume
    // ========================================================

    transportSource.setGain(
        muted
        ? 0.0f
        : volumeBeforeMute);

    return true;
}

// ============================================================
// Play
// ============================================================

void AudioEngine::play()
{
    transportSource.start();
}

// ============================================================
// Pause
// ============================================================

void AudioEngine::pause()
{
    transportSource.stop();
}

// ============================================================
// Stop
// ============================================================

void AudioEngine::stop()
{
    transportSource.stop();

    transportSource.setPosition(
        0.0);
}

// ============================================================
// Is playing
// ============================================================

bool AudioEngine::isPlaying() const
{
    return transportSource.isPlaying();
}

// ============================================================
// Set position
// ============================================================

void AudioEngine::setPosition(
    double seconds)
{
    transportSource.setPosition(
        juce::jmax(
            0.0,
            juce::jmin(
                seconds,
                getLength())));
}

// ============================================================
// Get position
// ============================================================

double AudioEngine::getPosition() const
{
    return transportSource.getCurrentPosition();
}

// ============================================================
// Get length
// ============================================================

double AudioEngine::getLength() const
{
    return transportSource.getLengthInSeconds();
}

// ============================================================
// Get file length
// ============================================================

double AudioEngine::getFileLengthInSeconds(
    const juce::File& file)
{
    if (!file.existsAsFile())
        return 0.0;

    std::unique_ptr<juce::AudioFormatReader> fileReader(
        formatManager.createReaderFor(file));

    if (fileReader == nullptr)
        return 0.0;

    if (fileReader->sampleRate <= 0.0)
        return 0.0;

    return static_cast<double>(
        fileReader->lengthInSamples)
        / fileReader->sampleRate;
}

// ============================================================
// Volume
// ============================================================

void AudioEngine::setVolume(
    float volume)
{
    volume =
        juce::jlimit(
            0.0f,
            1.0f,
            volume);

    volumeBeforeMute =
        volume;

    if (!muted)
    {
        transportSource.setGain(
            volume);
    }
}

// ============================================================
// Get volume
// ============================================================

float AudioEngine::getVolume() const
{
    return volumeBeforeMute;
}

// ============================================================
// Mute
// ============================================================

void AudioEngine::setMuted(
    bool shouldMute)
{
    muted = shouldMute;

    if (muted)
    {
        transportSource.setGain(
            0.0f);
    }
    else
    {
        transportSource.setGain(
            volumeBeforeMute);
    }
}

// ============================================================
// Is muted
// ============================================================

bool AudioEngine::isMuted() const
{
    return muted;
}

// ============================================================
// Spectrum
// ============================================================

void AudioEngine::copySpectrumData(
    float* destination,
    int numberOfBins) const
{
    analyserSource.copySpectrumData(
        destination,
        numberOfBins);
}

// ============================================================
// Waveform
// ============================================================

juce::AudioThumbnail&
AudioEngine::getThumbnail()
{
    return thumbnail;
}

// ============================================================
// Metadata getters
// ============================================================

juce::String
AudioEngine::getTrackTitle() const
{
    return trackTitle;
}

juce::String
AudioEngine::getTrackArtist() const
{
    return trackArtist;
}

juce::String
AudioEngine::getTrackAlbum() const
{
    return trackAlbum;
}

// ============================================================
// Album Art
// ============================================================

const juce::Image&
AudioEngine::getAlbumArt() const
{
    return albumArt;
}

// ============================================================
// Audio device manager
// ============================================================

juce::AudioDeviceManager&
AudioEngine::getDeviceManager()
{
    return audioDeviceManager;
}

// ============================================================
// Current device name
// ============================================================

juce::String
AudioEngine::getCurrentDeviceName() const
{
    if (auto* device =
        audioDeviceManager.getCurrentAudioDevice())
    {
        return device->getName();
    }

    return "No Audio Device";
}

// ============================================================
// Current device type
// ============================================================

juce::String
AudioEngine::getCurrentDeviceType() const
{
    if (auto* device =
        audioDeviceManager.getCurrentAudioDevice())
    {
        return device->getTypeName();
    }

    return "No Device Type";
}

// ============================================================
// Current sample rate
// ============================================================

double
AudioEngine::getCurrentSampleRate() const
{
    if (auto* device =
        audioDeviceManager.getCurrentAudioDevice())
    {
        return device->getCurrentSampleRate();
    }

    return 0.0;
}

// ============================================================
// Current buffer size
// ============================================================

int
AudioEngine::getCurrentBufferSize() const
{
    if (auto* device =
        audioDeviceManager.getCurrentAudioDevice())
    {
        return device->getCurrentBufferSizeSamples();
    }

    return 0;
}

// ============================================================
// Is audio device open
// ============================================================

bool
AudioEngine::isAudioDeviceOpen() const
{
    return
        audioDeviceManager.getCurrentAudioDevice()
        != nullptr;
}

// ============================================================
// Configure WASAPI
// ============================================================

juce::String AudioEngine::configureWASAPI(
    juce::WASAPIDeviceMode mode)
{
    juce::AudioIODeviceType* wasapiType =
        nullptr;

    const auto& deviceTypes =
        audioDeviceManager.getAvailableDeviceTypes();

    for (auto* type : deviceTypes)
    {
        if (type == nullptr)
            continue;

        const auto typeName =
            type->getTypeName();

        auto* requestedType =
            juce::AudioIODeviceType::
            createAudioIODeviceType_WASAPI(mode);

        if (requestedType != nullptr)
        {
            if (requestedType->getTypeName() ==
                typeName)
            {
                wasapiType = type;

                delete requestedType;

                break;
            }

            delete requestedType;
        }
    }

    if (wasapiType == nullptr)
        return "Requested WASAPI mode is not available";

    audioDeviceManager.setCurrentAudioDeviceType(
        wasapiType->getTypeName(),
        true);

    auto setup =
        audioDeviceManager.getAudioDeviceSetup();

    const auto outputDevices =
        wasapiType->getDeviceNames(false);

    if (outputDevices.isEmpty())
        return "No WASAPI output device found";

    if (!outputDevices.contains(
        setup.outputDeviceName))
    {
        setup.outputDeviceName =
            outputDevices[0];
    }

    setup.inputDeviceName.clear();

    setup.useDefaultInputChannels =
        false;

    setup.useDefaultOutputChannels =
        true;

    if (setup.sampleRate <= 0.0)
        setup.sampleRate = 44100.0;

    if (setup.bufferSize <= 0)
        setup.bufferSize = 512;

    return audioDeviceManager.setAudioDeviceSetup(
        setup,
        true);
}

// ============================================================
// WASAPI Shared
// ============================================================

juce::String
AudioEngine::setWASAPIShared()
{
    return configureWASAPI(
        juce::WASAPIDeviceMode::shared);
}

// ============================================================
// WASAPI Shared Low Latency
// ============================================================

juce::String
AudioEngine::setWASAPISharedLowLatency()
{
    return configureWASAPI(
        juce::WASAPIDeviceMode::sharedLowLatency);
}

// ============================================================
// WASAPI Exclusive
// ============================================================

juce::String
AudioEngine::setWASAPIExclusive()
{
    return configureWASAPI(
        juce::WASAPIDeviceMode::exclusive);
}

// ============================================================
// AnalyserSource
// ============================================================

AudioEngine::AnalyserSource::AnalyserSource(
    juce::PositionableAudioSource& sourceToUse,
    AudioEngine& ownerToUse)
    : source(sourceToUse),
    owner(ownerToUse)
{
}

// ============================================================
// Analyser prepare
// ============================================================

void AudioEngine::AnalyserSource::prepareToPlay(
    int samplesPerBlockExpected,
    double sampleRate)
{
    source.prepareToPlay(
        samplesPerBlockExpected,
        sampleRate);

    fftBuffer.fill(0.0f);

    for (auto& buffer : spectrumBuffers)
        buffer.fill(0.0f);

    fftBufferPosition = 0;
}

// ============================================================
// Analyser release
// ============================================================

void AudioEngine::AnalyserSource::releaseResources()
{
    source.releaseResources();
}

// ============================================================
// Analyser audio block
// ============================================================

void AudioEngine::AnalyserSource::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill)
{
    source.getNextAudioBlock(
        bufferToFill);

    if (bufferToFill.buffer == nullptr)
        return;

    const int numSamples =
        bufferToFill.numSamples;

    const int startSample =
        bufferToFill.startSample;

    const int numChannels =
        bufferToFill.buffer->getNumChannels();

    for (int sample = 0;
        sample < numSamples;
        ++sample)
    {
        float monoSample = 0.0f;

        for (int channel = 0;
            channel < numChannels;
            ++channel)
        {
            monoSample +=
                bufferToFill.buffer
                ->getSample(
                    channel,
                    startSample + sample);
        }

        if (numChannels > 0)
        {
            monoSample /=
                static_cast<float>(
                    numChannels);
        }

        fftBuffer[fftBufferPosition] =
            monoSample;

        ++fftBufferPosition;

        if (fftBufferPosition >= fftSize)
        {
            auto* fftData =
                fftBuffer.data();

            window.multiplyWithWindowingTable(
                fftData,
                fftSize);

            fft.performRealOnlyForwardTransform(
                fftData);

            const int writeBuffer =
                1 -
                activeSpectrumBuffer.load(
                    std::memory_order_relaxed);

            auto& destination =
                spectrumBuffers[writeBuffer];

            for (int i = 0;
                i < fftSize / 2;
                ++i)
            {
                const float real =
                    fftData[i * 2];

                const float imag =
                    fftData[i * 2 + 1];

                const float magnitude =
                    std::sqrt(
                        real * real +
                        imag * imag);

                // ========================================================
                // Normalize FFT magnitude
                //
                // FFT output is not in 0..1.
                // Normalize by FFT size so that the spectrum
                // represents the actual signal amplitude more naturally.
                // ========================================================

                const float normalizedMagnitude =
                    magnitude
                    / static_cast<float>(fftSize);

                destination[i] =
                    normalizedMagnitude;
            }

            activeSpectrumBuffer.store(
                writeBuffer,
                std::memory_order_release);

            fftBufferPosition = 0;
        }
    }
}

// ============================================================
// Analyser set position
// ============================================================

void AudioEngine::AnalyserSource::setNextReadPosition(
    juce::int64 newPosition)
{
    source.setNextReadPosition(
        newPosition);
}

// ============================================================
// Analyser get position
// ============================================================

juce::int64
AudioEngine::AnalyserSource::getNextReadPosition() const
{
    return source.getNextReadPosition();
}

// ============================================================
// Analyser total length
// ============================================================

juce::int64
AudioEngine::AnalyserSource::getTotalLength() const
{
    return source.getTotalLength();
}

// ============================================================
// Analyser looping
// ============================================================

bool AudioEngine::AnalyserSource::isLooping() const
{
    return source.isLooping();
}

// ============================================================
// Copy spectrum
// ============================================================

void AudioEngine::AnalyserSource::copySpectrumData(
    float* destination,
    int numberOfBins) const
{
    if (destination == nullptr ||
        numberOfBins <= 0)
    {
        return;
    }

    const int bufferIndex =
        activeSpectrumBuffer.load(
            std::memory_order_acquire);

    const auto& sourceBuffer =
        spectrumBuffers[bufferIndex];

    const int binsToCopy =
        juce::jmin(
            numberOfBins,
            fftSize / 2);

    std::memcpy(
        destination,
        sourceBuffer.data(),
        sizeof(float) *
        static_cast<size_t>(
            binsToCopy));
}