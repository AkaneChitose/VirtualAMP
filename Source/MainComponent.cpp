#include "MainComponent.h"

// ============================================================
// Constructor
// ============================================================

MainComponent::MainComponent()
{
    // ========================================================
    // Open
    // ========================================================

    openButton.setButtonText("Open");

    openButton.onClick =
        [this]
        {
            addFiles();
        };

    addAndMakeVisible(openButton);

    // ========================================================
    // Audio Settings
    // ========================================================

    settingsButton.setButtonText("Audio Settings");

    settingsButton.onClick =
        [this]
        {
            openAudioSettings();
        };

    addAndMakeVisible(settingsButton);

    // ========================================================
    // WASAPI Mode
    // ========================================================

    wasapiModeBox.addItem(
        "Shared",
        1
    );

    wasapiModeBox.addItem(
        "Shared Low Latency",
        2
    );

    wasapiModeBox.addItem(
        "Exclusive",
        3
    );

    wasapiModeBox.setTextWhenNothingSelected(
        "WASAPI Mode"
    );

    wasapiModeBox.onChange =
        [this]
        {
            setWASAPIMode(
                wasapiModeBox.getSelectedId()
            );
        };

    addAndMakeVisible(
        wasapiModeBox
    );

    // ========================================================
    // Previous
    // ========================================================

    previousButton.setButtonText("<<");

    previousButton.onClick =
        [this]
        {
            playPrevious();
        };

    addAndMakeVisible(previousButton);

    // ========================================================
    // Play / Pause
    // ========================================================

    playButton.setButtonText("Play");

    playButton.onClick =
        [this]
        {
            if (audioEngine.isPlaying())
            {
                audioEngine.pause();
            }
            else
            {
                if (audioEngine.getLength() <= 0.0)
                {
                    if (queueManager.getCurrentFile() != nullptr)
                    {
                        playQueueIndex(
                            queueManager.getCurrentIndex()
                        );
                    }
                }
                else
                {
                    audioEngine.play();
                }
            }

            updatePlayButton();
        };

    addAndMakeVisible(playButton);

    // ========================================================
    // Stop
    // ========================================================

    stopButton.setButtonText("Stop");

    stopButton.onClick =
        [this]
        {
            audioEngine.stop();

            autoNextTriggered = false;

            updatePlayButton();
            updateTimeLabels();
        };

    addAndMakeVisible(stopButton);

    // ========================================================
    // Next
    // ========================================================

    nextButton.setButtonText(">>");

    nextButton.onClick =
        [this]
        {
            playNext();
        };

    addAndMakeVisible(nextButton);

    // ========================================================
    // Mute
    // ========================================================

    muteButton.setButtonText("Mute");

    muteButton.onClick =
        [this]
        {
            const bool newMuteState =
                !audioEngine.isMuted();

            audioEngine.setMuted(
                newMuteState
            );

            updateVolumeUI();
        };

    addAndMakeVisible(muteButton);

    // ========================================================
    // Seek
    // ========================================================

    positionSlider.setRange(
        0.0,
        1.0,
        0.001
    );

    positionSlider.setValue(
        0.0,
        juce::dontSendNotification
    );

    positionSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );

    positionSlider.setTextBoxStyle(
        juce::Slider::NoTextBox,
        false,
        0,
        0
    );

    positionSlider.onValueChange =
        [this]
        {
            if (positionSlider.isMouseButtonDown())
            {
                const auto length =
                    audioEngine.getLength();

                if (length > 0.0)
                {
                    audioEngine.setPosition(
                        positionSlider.getValue()
                        * length
                    );

                    autoNextTriggered = false;
                }
            }
        };

    addAndMakeVisible(positionSlider);

    // ========================================================
    // Volume
    // ========================================================

    volumeSlider.setRange(
        0.0,
        1.0,
        0.001
    );

    volumeSlider.setValue(
        audioEngine.getVolume(),
        juce::dontSendNotification
    );

    volumeSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );

    volumeSlider.setTextBoxStyle(
        juce::Slider::NoTextBox,
        false,
        0,
        0
    );

    volumeSlider.onValueChange =
        [this]
        {
            const float volume =
                static_cast<float>(
                    volumeSlider.getValue()
                    );

            audioEngine.setVolume(
                volume
            );
        };

    addAndMakeVisible(volumeSlider);

    // ========================================================
    // Current Time
    // ========================================================

    currentTimeLabel.setText(
        "00:00",
        juce::dontSendNotification
    );

    currentTimeLabel.setJustificationType(
        juce::Justification::centredRight
    );

    currentTimeLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white.withAlpha(0.70f)
    );

    addAndMakeVisible(currentTimeLabel);

    // ========================================================
    // Total Time
    // ========================================================

    totalTimeLabel.setText(
        "00:00",
        juce::dontSendNotification
    );

    totalTimeLabel.setJustificationType(
        juce::Justification::centredLeft
    );

    totalTimeLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white.withAlpha(0.70f)
    );

    addAndMakeVisible(totalTimeLabel);

    // ========================================================
    // Device
    // ========================================================

    deviceLabel.setJustificationType(
        juce::Justification::centredLeft
    );

    deviceLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white.withAlpha(0.70f)
    );

    addAndMakeVisible(deviceLabel);

    // ========================================================
    // Format
    // ========================================================

    formatLabel.setJustificationType(
        juce::Justification::centredLeft
    );

    formatLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white.withAlpha(0.45f)
    );

    addAndMakeVisible(formatLabel);

    // ========================================================
    // Queue Title
    // ========================================================

    queueTitleLabel.setText(
        "PLAY QUEUE",
        juce::dontSendNotification
    );

    queueTitleLabel.setJustificationType(
        juce::Justification::centredLeft
    );

    queueTitleLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white.withAlpha(0.55f)
    );

    addAndMakeVisible(queueTitleLabel);

    // ========================================================
    // Queue List
    // ========================================================

    queueListBox.setModel(this);

    queueListBox.onReorder =
        [this](int fromIndex, int toIndex)
        {
            if (fromIndex == toIndex)
                return;

            queueManager.move(
                fromIndex,
                toIndex
            );

            updateQueueUI();

            queueListBox.repaint();

            repaint();
        };

    queueListBox.setRowHeight(32);

    queueListBox.setMultipleSelectionEnabled(false);

    queueListBox.setClickingTogglesRowSelection(false);

    queueListBox.setColour(
        juce::ListBox::backgroundColourId,
        juce::Colour::fromRGB(
            18,
            18,
            21
        )
    );

    queueListBox.setColour(
        juce::ListBox::outlineColourId,
        juce::Colours::white.withAlpha(0.05f)
    );

    addAndMakeVisible(queueListBox);

    // ========================================================
    // Remove Queue Item
    // ========================================================

    removeQueueButton.setButtonText("Remove");

    removeQueueButton.onClick =
        [this]
        {
            removeSelectedQueueItem();
        };

    addAndMakeVisible(removeQueueButton);

    // ========================================================
    // Clear Queue
    // ========================================================

    clearQueueButton.setButtonText("Clear");

    clearQueueButton.onClick =
        [this]
        {
            clearQueue();
        };

    addAndMakeVisible(clearQueueButton);

    // ========================================================
    // Window
    // ========================================================

    setSize(
        1200,
        800
    );

    // ========================================================
    // Initial UI
    // ========================================================

    updateVolumeUI();
    updatePlayButton();
    updateQueueUI();
    updateWASAPIModeUI();

    // 60 FPS UI update
    startTimerHz(60);
}

// ============================================================
// Destructor
// ============================================================

MainComponent::~MainComponent()
{
    stopTimer();

    queueListBox.setModel(nullptr);
}

// ============================================================
// Paint
// ============================================================

void MainComponent::paint(
    juce::Graphics& g)
{
    g.fillAll(
        juce::Colour::fromRGB(
            15,
            15,
            17
        )
    );

    drawHeader(g);
    drawAlbumArea(g);
    drawSpectrum(g);
    drawWaveform(g);
    drawPlayerArea(g);
}

// ============================================================
// Resized
// ============================================================

void MainComponent::resized()
{
    const int width = getWidth();

    // =========================================================
    // HEADER
    // =========================================================

    openButton.setBounds(
        20, 17,
        70, 30
    );

    wasapiModeBox.setBounds(
        180, 17,
        190, 30
    );

    settingsButton.setBounds(
        width - 170, 17,
        150, 30
    );


    // =========================================================
    // MAIN CONTENT
    // =========================================================

    const int margin = 24;
    const int contentTop = headerHeight + 20;
    const int columnGap = 24;


    // =========================================================
    // ALBUM
    // =========================================================

    const int albumSize = 250;

    albumArea = juce::Rectangle<int>(
        margin,
        contentTop,
        albumSize,
        albumSize
    );


    // =========================================================
    // TOP SECTION
    // =========================================================

    const int descriptionHeight = 86;

    const int topSectionHeight =
        albumSize + descriptionHeight;


    // =========================================================
    // SPECTRUM
    // =========================================================

    const int spectrumX =
        albumArea.getRight() + columnGap;

    const int spectrumWidth =
        juce::jmax(
            200,
            width - spectrumX - margin
        );

    spectrumArea = juce::Rectangle<int>(
        spectrumX,
        contentTop,
        spectrumWidth,
        topSectionHeight
    );


    // =========================================================
    // WAVEFORM
    // =========================================================

    const int waveformGap = 24;
    const int waveformHeight = 130;

    const int waveformY =
        contentTop
        + topSectionHeight
        + waveformGap;

    waveformArea = juce::Rectangle<int>(
        margin,
        waveformY,
        juce::jmax(
            200,
            width - margin * 2
        ),
        waveformHeight
    );


    // =========================================================
    // PLAYER / QUEUE POSITION
    //
    // FIXED VERTICAL POSITION
    // =========================================================

    const int bottomGap = 24;

    const int bottomY =
        waveformArea.getBottom()
        + bottomGap;


    // =========================================================
    // PLAYER / QUEUE WIDTH
    // =========================================================

    const int totalWidth =
        juce::jmax(
            400,
            width - margin * 2
        );

    const int panelGap = 24;

    // Fixed player width
    constexpr int playerWidth = 430;

    // Queue takes the remaining space
    const int queueWidth =
        juce::jmax(
            200,
            totalWidth - playerWidth - panelGap
        );


    // =========================================================
    // PLAYER
    //
    // FIXED HEIGHT
    // =========================================================

    constexpr int playerHeight = 170;

    playerArea = juce::Rectangle<int>(
        margin,
        bottomY,
        playerWidth,
        playerHeight
    );


    // =========================================================
    // QUEUE
    // =========================================================

    const int queueHeaderHeight = 32;
    const int queueRowHeight = 28;

    constexpr int minimumQueueHeight = 170;
    constexpr int maximumQueueHeight = 400;

    const int numRows =
        juce::jmax(
            1,
            queueManager.getSize()
        );

    const int desiredQueueHeight =
        queueHeaderHeight
        + numRows * queueRowHeight
        + 8;

    const int queueHeight =
        juce::jlimit(
            minimumQueueHeight,
            maximumQueueHeight,
            desiredQueueHeight
        );

    const int queueX =
        playerArea.getRight()
        + panelGap;

    queueArea = juce::Rectangle<int>(
        queueX,
        bottomY,
        queueWidth,
        queueHeight
    );


    // =========================================================
    // QUEUE HEADER
    // =========================================================

    const int queueHeaderY =
        queueArea.getY() + 4;

    queueTitleLabel.setBounds(
        queueArea.getX() + 10,
        queueHeaderY,
        100,
        24
    );

    removeQueueButton.setBounds(
        queueArea.getRight() - 140,
        queueHeaderY,
        65,
        24
    );

    clearQueueButton.setBounds(
        queueArea.getRight() - 65,
        queueHeaderY,
        55,
        24
    );


    // =========================================================
    // QUEUE LIST
    // =========================================================

    queueListBox.setBounds(
        queueArea.getX(),
        queueArea.getY() + queueHeaderHeight,
        queueArea.getWidth(),
        queueArea.getHeight()
        - queueHeaderHeight
    );


    // =========================================================
    // PLAYER
    // =========================================================


    // ---------------------------------------------------------
    // TRANSPORT BUTTONS
    // ---------------------------------------------------------

    const int buttonHeight = 36;
    const int buttonGap = 8;

    const int previousWidth = 52;
    const int playWidth = 72;
    const int stopWidth = 60;
    const int nextWidth = 52;

    const int totalButtonWidth =
        previousWidth
        + playWidth
        + stopWidth
        + nextWidth
        + buttonGap * 3;

    int buttonX =
        playerArea.getCentreX()
        - totalButtonWidth / 2;

    const int buttonY =
        playerArea.getY() + 10;

    previousButton.setBounds(
        buttonX,
        buttonY,
        previousWidth,
        buttonHeight
    );

    buttonX += previousWidth + buttonGap;

    playButton.setBounds(
        buttonX,
        buttonY,
        playWidth,
        buttonHeight
    );

    buttonX += playWidth + buttonGap;

    stopButton.setBounds(
        buttonX,
        buttonY,
        stopWidth,
        buttonHeight
    );

    buttonX += stopWidth + buttonGap;

    nextButton.setBounds(
        buttonX,
        buttonY,
        nextWidth,
        buttonHeight
    );


    // ---------------------------------------------------------
    // SEEK BAR
    // ---------------------------------------------------------

    const int seekY =
        playerArea.getY() + 54;

    const int timeWidth = 48;
    const int seekMargin = 12;

    const int seekX =
        playerArea.getX()
        + seekMargin
        + timeWidth
        + 6;

    const int seekRight =
        playerArea.getRight()
        - seekMargin
        - timeWidth;

    const int seekWidth =
        juce::jmax(
            80,
            seekRight - seekX
        );

    currentTimeLabel.setBounds(
        playerArea.getX() + seekMargin,
        seekY,
        timeWidth,
        26
    );

    positionSlider.setBounds(
        seekX,
        seekY - 3,
        seekWidth,
        32
    );

    totalTimeLabel.setBounds(
        seekX + seekWidth + 6,
        seekY,
        timeWidth,
        26
    );


    // ---------------------------------------------------------
    // VOLUME + MUTE
    //
    // No separate volume label.
    // ---------------------------------------------------------

    const int volumeY =
        playerArea.getY() + 91;

    const int muteWidth = 65;
    const int volumeWidth = 125;
    const int volumeGap = 8;

    const int muteX =
        playerArea.getRight()
        - 12
        - muteWidth;

    const int volumeX =
        muteX
        - volumeGap
        - volumeWidth;

    volumeSlider.setBounds(
        volumeX,
        volumeY - 7,
        volumeWidth,
        34
    );

    muteButton.setBounds(
        muteX,
        volumeY - 2,
        muteWidth,
        28
    );


    // ---------------------------------------------------------
    // DEVICE LABEL
    // ---------------------------------------------------------

    const int infoX =
        playerArea.getX() + 12;

    const int infoWidth =
        playerArea.getWidth() - 24;

    const int deviceY =
        playerArea.getY() + 124;

    deviceLabel.setBounds(
        infoX,
        deviceY,
        infoWidth,
        18
    );


    // ---------------------------------------------------------
    // FORMAT LABEL
    // ---------------------------------------------------------

    const int formatY =
        playerArea.getY() + 143;

    formatLabel.setBounds(
        infoX,
        formatY,
        infoWidth,
        18
    );
}

// ============================================================
// Timer
// ============================================================

void MainComponent::timerCallback()
{
    // ========================================================
    // Spectrum update
    // ========================================================

    audioEngine.copySpectrumData(
        spectrumData.data(),
        static_cast<int>(
            spectrumData.size()
            )
    );

    // ========================================================
    // Frequency range
    // ========================================================

    constexpr float minFrequency = 20.0f;
    constexpr float maxFrequency = 20000.0f;

    constexpr int numberOfBars = 128;
    constexpr int fftSize = 4096;

    // IMPORTANT:
    // This is only the variable used for the FFT calculation.
    // Do not declare another "sampleRate" later in this scope.

    const double spectrumSampleRate =
        audioEngine.getCurrentSampleRate();

    if (spectrumSampleRate > 0.0)
    {
        const float logMin =
            std::log10(
                minFrequency
            );

        const float logMax =
            std::log10(
                maxFrequency
            );

        for (int i = 0;
            i < numberOfBars;
            ++i)
        {
            // =================================================
            // Logarithmic frequency band
            // =================================================

            const float t0 =
                static_cast<float>(i)
                / static_cast<float>(
                    numberOfBars
                    );

            const float t1 =
                static_cast<float>(i + 1)
                / static_cast<float>(
                    numberOfBars
                    );

            const float frequency0 =
                std::pow(
                    10.0f,
                    logMin
                    + (logMax - logMin) * t0
                );

            const float frequency1 =
                std::pow(
                    10.0f,
                    logMin
                    + (logMax - logMin) * t1
                );

            // =================================================
            // Frequency -> FFT bin
            // =================================================

            const int bin0 =
                juce::jlimit(
                    0,
                    fftSize / 2 - 1,
                    static_cast<int>(
                        std::floor(
                            frequency0
                            * fftSize
                            / spectrumSampleRate
                        )
                        )
                );

            const int bin1 =
                juce::jlimit(
                    bin0 + 1,
                    fftSize / 2,
                    static_cast<int>(
                        std::ceil(
                            frequency1
                            * fftSize
                            / spectrumSampleRate
                        )
                        )
                );

            // =================================================
            // Find strongest magnitude in this band
            // =================================================

            float magnitude = 0.0f;

            for (int bin = bin0;
                bin < bin1;
                ++bin)
            {
                magnitude =
                    juce::jmax(
                        magnitude,
                        spectrumData[
                            juce::jmin(
                                bin,
                                static_cast<int>(
                                    spectrumData.size()
                                    ) - 1
                            )
                        ]
                    );
            }

            // =================================================
            // Linear magnitude -> dBFS
            // =================================================

            constexpr float minDb = -80.0f;

            const float dB =
                juce::Decibels::gainToDecibels(
                    juce::jmax(
                        magnitude,
                        0.0000001f
                    ),
                    minDb
                );

            // ========================================================
            // dBFS -> 0..1
            //
            // -80 dBFS = 0%
            //   0 dBFS = 100%
            // ========================================================

            const float target =
                juce::jlimit(
                    0.0f,
                    1.0f,
                    juce::jmap(
                        dB,
                        minDb,
                        0.0f,
                        0.0f,
                        1.0f
                    )
                );

            // =================================================
            // Smooth
            //
            // Attack  = faster
            // Release = slower
            // =================================================

            const float current =
                smoothedBars[i];

            const float smoothing =
                target > current
                ? 0.35f
                : 0.12f;

            smoothedBars[i] =
                current
                + (target - current)
                * smoothing;
        }
    }
    else
    {
        // ====================================================
        // No valid audio device
        // Smooth bars toward zero
        // ====================================================

        for (auto& bar : smoothedBars)
        {
            bar *= 0.90f;

            if (bar < 0.001f)
                bar = 0.0f;
        }
    }

    // ========================================================
    // Position
    // ========================================================

    const auto length =
        audioEngine.getLength();

    const auto position =
        audioEngine.getPosition();

    if (length > 0.0 &&
        !positionSlider.isMouseButtonDown())
    {
        positionSlider.setValue(
            position / length,
            juce::dontSendNotification
        );
    }

    // ========================================================
    // Auto-next
    // ========================================================

    if (length > 0.0 &&
        !audioEngine.isPlaying() &&
        position >= length - 0.15 &&
        !autoNextTriggered)
    {
        autoNextTriggered = true;

        if (queueManager.hasNext())
        {
            playNext();
        }
    }

    // ========================================================
    // Reset auto-next state
    // ========================================================

    if (audioEngine.isPlaying() &&
        position < length - 0.5)
    {
        autoNextTriggered = false;
    }

    // ========================================================
    // Player state
    // ========================================================

    updatePlayButton();
    updateTimeLabels();

    // ========================================================
    // Volume
    // ========================================================

    if (!volumeSlider.isMouseButtonDown())
    {
        volumeSlider.setValue(
            audioEngine.getVolume(),
            juce::dontSendNotification
        );
    }

    updateVolumeUI();

    // ========================================================
    // WASAPI UI
    // ========================================================

    updateWASAPIModeUI();

    // ========================================================
    // Device
    // ========================================================

    deviceLabel.setText(
        "Device: "
        + audioEngine.getCurrentDeviceName(),
        juce::dontSendNotification
    );

    // ========================================================
    // Format
    // ========================================================

    const auto currentSampleRate =
        audioEngine.getCurrentSampleRate();

    const auto bufferSize =
        audioEngine.getCurrentBufferSize();

    const auto type =
        audioEngine.getCurrentDeviceType();

    juce::String formatText;

    formatText
        << "Driver: "
        << type
        << "    |    "
        << juce::String(
            currentSampleRate,
            0
        )
        << " Hz"
        << "    |    "
        << juce::String(
            bufferSize
        )
        << " samples";

    formatLabel.setText(
        formatText,
        juce::dontSendNotification
    );

    repaint();
}

// ============================================================
// Update Play Button
// ============================================================

void MainComponent::updatePlayButton()
{
    if (audioEngine.isPlaying())
        playButton.setButtonText("Pause");
    else
        playButton.setButtonText("Play");
}

// ============================================================
// Update Volume UI
// ============================================================

void MainComponent::updateVolumeUI()
{
    if (audioEngine.isMuted())
    {
        muteButton.setButtonText(
            "Unmute"
        );
    }
    else
    {
        muteButton.setButtonText(
            "Mute"
        );
    }
}

// ============================================================
// Set WASAPI Mode
// ============================================================

void MainComponent::setWASAPIMode(
    int mode)
{
    juce::String result;

    switch (mode)
    {
    case 1:
        result =
            audioEngine.setWASAPIShared();
        break;

    case 2:
        result =
            audioEngine.setWASAPISharedLowLatency();
        break;

    case 3:
        result =
            audioEngine.setWASAPIExclusive();
        break;

    default:
        return;
    }

    if (result.isNotEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "WASAPI",
            result
        );

        updateWASAPIModeUI();

        return;
    }

    updateWASAPIModeUI();
}

// ============================================================
// Update WASAPI Mode UI
// ============================================================

void MainComponent::updateWASAPIModeUI()
{
    const auto type =
        audioEngine.getCurrentDeviceType();

    if (type.equalsIgnoreCase(
        "Windows Audio"))
    {
        wasapiModeBox.setSelectedId(
            1,
            juce::dontSendNotification
        );
    }
    else if (type.equalsIgnoreCase(
        "Windows Audio (Low Latency Mode)"))
    {
        wasapiModeBox.setSelectedId(
            2,
            juce::dontSendNotification
        );
    }
    else if (type.equalsIgnoreCase(
        "Windows Audio (Exclusive Mode)"))
    {
        wasapiModeBox.setSelectedId(
            3,
            juce::dontSendNotification
        );
    }
}

// ============================================================
// Update Queue UI
// ============================================================

void MainComponent::updateQueueUI()
{
    queueListBox.updateContent();

    const int currentIndex =
        queueManager.getCurrentIndex();

    if (currentIndex >= 0 &&
        currentIndex < queueManager.getSize())
    {
        queueListBox.selectRow(
            currentIndex,
            juce::dontSendNotification
        );

        queueListBox.scrollToEnsureRowIsOnscreen(
            currentIndex
        );
    }
    else
    {
        queueListBox.deselectAllRows();
    }

    repaint();
}

// ============================================================
// Format Time
// ============================================================

static juce::String formatTime(
    double seconds)
{
    if (!std::isfinite(seconds) ||
        seconds < 0.0)
    {
        seconds = 0.0;
    }

    const int totalSeconds =
        static_cast<int>(
            seconds
            );

    const int minutes =
        totalSeconds / 60;

    const int remainingSeconds =
        totalSeconds % 60;

    return juce::String(minutes)
        .paddedLeft('0', 2)
        + ":"
        + juce::String(
            remainingSeconds
        ).paddedLeft('0', 2);
}

// ============================================================
// Update Time
// ============================================================

void MainComponent::updateTimeLabels()
{
    currentTimeLabel.setText(
        formatTime(
            audioEngine.getPosition()
        ),
        juce::dontSendNotification
    );

    totalTimeLabel.setText(
        formatTime(
            audioEngine.getLength()
        ),
        juce::dontSendNotification
    );
}

// ============================================================
// Header
// ============================================================

void MainComponent::drawHeader(
    juce::Graphics& g)
{
    g.setColour(
        juce::Colour::fromRGB(
            25,
            25,
            28
        )
    );

    g.fillRect(
        0,
        0,
        getWidth(),
        headerHeight
    );

    g.setColour(
        juce::Colours::white
    );

    g.setFont(
        juce::FontOptions(
            22.0f
        ).withStyle(
            "Bold"
        )
    );

    g.drawText(
        "VirtualAMP",
        400,
        0,
        200,
        headerHeight,
        juce::Justification::centredLeft
    );

    g.setColour(
        juce::Colours::white.withAlpha(
            0.06f
        )
    );

    g.fillRect(
        0,
        headerHeight - 1,
        getWidth(),
        1
    );
}

// ============================================================
// Album Area
// ============================================================

void MainComponent::drawAlbumArea(
    juce::Graphics& g)
{
    if (albumArea.isEmpty())
        return;

    // ========================================================
    // Outer background
    // ========================================================

    g.setColour(
        juce::Colour::fromRGB(
            28,
            28,
            32
        )
    );

    g.fillRoundedRectangle(
        albumArea.toFloat(),
        12.0f
    );

    // ========================================================
    // Album image area
    // ========================================================

    auto inner =
        albumArea.reduced(20);

    const auto& albumArt =
        audioEngine.getAlbumArt();

    if (albumArt.isValid())
    {
        g.setColour(
            juce::Colours::white
        );

        g.drawImageWithin(
            albumArt,
            inner.getX(),
            inner.getY(),
            inner.getWidth(),
            inner.getHeight(),
            juce::RectanglePlacement::centred
            | juce::RectanglePlacement::fillDestination,
            false
        );
    }
    else
    {
        g.setColour(
            juce::Colour::fromRGB(
                38,
                38,
                44
            )
        );

        g.fillRoundedRectangle(
            inner.toFloat(),
            8.0f
        );

        g.setColour(
            juce::Colours::white.withAlpha(
                0.16f
            )
        );

        g.setFont(
            juce::FontOptions(
                72.0f
            )
        );

        g.drawText(
            "Album",
            inner,
            juce::Justification::centred
        );
    }

    // ========================================================
    // Track Title
    // ========================================================

    const int textX =
        albumArea.getX();

    const int textY =
        albumArea.getBottom() + 14;

    g.setColour(
        juce::Colours::white
    );

    g.setFont(
        juce::FontOptions(
            18.0f
        ).withStyle(
            "Bold"
        )
    );

    g.drawText(
        trackTitle,
        textX,
        textY,
        albumArea.getWidth(),
        26,
        juce::Justification::centred,
        true
    );

    // ========================================================
    // Artist
    // ========================================================

    g.setColour(
        juce::Colours::white.withAlpha(
            0.60f
        )
    );

    g.setFont(
        juce::FontOptions(
            14.0f
        )
    );

    g.drawText(
        trackArtist,
        textX,
        textY + 28,
        albumArea.getWidth(),
        22,
        juce::Justification::centred,
        true
    );

    // ========================================================
    // Album
    // ========================================================

    g.setColour(
        juce::Colours::white.withAlpha(
            0.40f
        )
    );

    g.drawText(
        trackAlbum,
        textX,
        textY + 50,
        albumArea.getWidth(),
        22,
        juce::Justification::centred,
        true
    );
}

// ============================================================
// Spectrum
// ============================================================

void MainComponent::drawSpectrum(
    juce::Graphics& g)
{
    if (spectrumArea.isEmpty())
        return;

    // ========================================================
    // Background
    // ========================================================

    g.setColour(
        juce::Colour::fromRGB(
            20,
            20,
            24
        )
    );

    g.fillRoundedRectangle(
        spectrumArea.toFloat(),
        12.0f
    );

    // ========================================================
    // Graph area
    // ========================================================

    auto graphArea =
        spectrumArea.reduced(
            16,
            18
        );

    // Space for frequency labels
    graphArea.removeFromBottom(26);

    // ========================================================
    // Horizontal reference lines
    // ========================================================

    g.setColour(
        juce::Colours::white.withAlpha(
            0.055f
        )
    );

    for (int i = 1;
        i < 5;
        ++i)
    {
        const float y =
            graphArea.getBottom()
            - (
                static_cast<float>(i)
                / 5.0f
                * graphArea.getHeight()
                );

        g.drawHorizontalLine(
            static_cast<int>(y),
            static_cast<float>(
                graphArea.getX()
                ),
            static_cast<float>(
                graphArea.getRight()
                )
        );
    }

    // ========================================================
    // Spectrum bars
    // ========================================================

    constexpr int numberOfBars = 128;

    const float barWidth =
        static_cast<float>(
            graphArea.getWidth()
            )
        / static_cast<float>(
            numberOfBars
            );

    for (int i = 0;
        i < numberOfBars;
        ++i)
    {
        const float value =
            juce::jmax(
                0.0f,
                smoothedBars[i]
            );

        const float barHeight =
            juce::jmin(
                value * static_cast<float>(graphArea.getHeight()),
                static_cast<float>(graphArea.getHeight())
            );

        const float x =
            graphArea.getX()
            + i * barWidth;

        const float y =
            graphArea.getBottom()
            - barHeight;

        const float width =
            juce::jmax(
                1.0f,
                barWidth - 1.2f
            );

        // ====================================================
        // Bar
        // ====================================================

        g.setColour(
            juce::Colour::fromRGB(
                70,
                180,
                255
            )
        );

        g.fillRoundedRectangle(
            x,
            y,
            width,
            juce::jmax(
                1.0f,
                barHeight
            ),
            1.5f
        );
    }

    // ========================================================
    // Frequency labels
    // ========================================================

    constexpr float minFrequency = 20.0f;
    constexpr float maxFrequency = 20000.0f;

    const float logMin =
        std::log10(
            minFrequency
        );

    const float logMax =
        std::log10(
            maxFrequency
        );

    struct FrequencyLabel
    {
        float frequency;
        const char* text;
    };

    constexpr FrequencyLabel labels[] =
    {
        { 20.0f,    "20" },
        { 50.0f,    "50" },
        { 100.0f,   "100" },
        { 200.0f,   "200" },
        { 500.0f,   "500" },
        { 1000.0f,  "1k" },
        { 2000.0f,  "2k" },
        { 5000.0f,  "5k" },
        { 10000.0f, "10k" },
        { 20000.0f, "20k" }
    };

    g.setFont(
        juce::FontOptions(
            11.0f
        )
    );

    for (const auto& label : labels)
    {
        const float t =
            (
                std::log10(
                    label.frequency
                )
                - logMin
                )
            / (
                logMax
                - logMin
                );

        const float x =
            graphArea.getX()
            + t
            * graphArea.getWidth();

        // ====================================================
        // Vertical guide
        // ====================================================

        g.setColour(
            juce::Colours::white.withAlpha(
                0.045f
            )
        );

        g.drawVerticalLine(
            static_cast<int>(x),
            static_cast<float>(
                graphArea.getY()
                ),
            static_cast<float>(
                graphArea.getBottom()
                )
        );

        // ====================================================
        // Label
        // ====================================================

        g.setColour(
            juce::Colours::white.withAlpha(
                0.60f
            )
        );

        g.drawText(
            label.text,
            static_cast<int>(
                x - 20.0f
                ),
            graphArea.getBottom() + 5,
            40,
            16,
            juce::Justification::centred,
            false
        );
    }

    // ========================================================
    // Title
    // ========================================================

    g.setColour(
        juce::Colours::white.withAlpha(
            0.35f
        )
    );

    g.setFont(
        juce::FontOptions(
            11.0f
        )
    );

    g.drawText(
        "SPECTRUM  •  20 Hz — 20 kHz",
        spectrumArea.getX() + 16,
        spectrumArea.getY() + 6,
        220,
        16,
        juce::Justification::left,
        false
    );
}

// ============================================================
// Waveform
// ============================================================

void MainComponent::drawWaveform(
    juce::Graphics& g)
{
    if (waveformArea.isEmpty())
        return;

    g.setColour(
        juce::Colour::fromRGB(
            10,
            10,
            12
        )
    );

    g.fillRoundedRectangle(
        waveformArea.toFloat(),
        12.0f
    );

    g.setColour(
        juce::Colours::white.withAlpha(
            0.45f
        )
    );

    g.setFont(
        juce::FontOptions(
            12.0f
        )
    );

    g.drawText(
        "WAVEFORM",
        waveformArea.getX() + 14,
        waveformArea.getY() + 10,
        100,
        20,
        juce::Justification::left
    );

    auto& thumbnail =
        audioEngine.getThumbnail();

    if (thumbnail.getTotalLength() <= 0.0)
        return;

    auto graphArea =
        waveformArea
        .reduced(10)
        .withTrimmedTop(25);

    g.setColour(
        juce::Colour::fromRGB(
            100,
            150,
            210
        )
    );

    thumbnail.drawChannels(
        g,
        graphArea,
        0.0,
        thumbnail.getTotalLength(),
        1.0f
    );

    const auto length =
        audioEngine.getLength();

    if (length > 0.0)
    {
        const auto position =
            audioEngine.getPosition();

        const auto ratio =
            juce::jlimit(
                0.0,
                1.0,
                position / length
            );

        const auto x =
            graphArea.getX()
            + ratio
            * graphArea.getWidth();

        g.setColour(
            juce::Colours::white
        );

        g.fillRect(
            static_cast<int>(x),
            graphArea.getY(),
            2,
            graphArea.getHeight()
        );
    }
}

// ============================================================
// Player Area
// ============================================================

void MainComponent::drawPlayerArea(
    juce::Graphics& g)
{
    if (playerArea.isEmpty())
        return;

    g.setColour(
        juce::Colour::fromRGB(
            24,
            24,
            27
        )
    );

    g.fillRoundedRectangle(
        playerArea.toFloat(),
        12.0f
    );

    g.setColour(
        juce::Colours::white.withAlpha(
            0.35f
        )
    );

    g.setFont(
        juce::FontOptions(
            12.0f
        )
    );

    //g.drawText(
    //    "VOLUME",
    //    playerArea.getRight() - 300,
    //    playerArea.getY() + 38,
    //    70,
    //    20,
    //    juce::Justification::left
    //);
}

// ============================================================
// Open File
// ============================================================

void MainComponent::openFile()
{
    auto chooser =
        std::make_shared<
        juce::FileChooser
        >(
            "Open audio file",
            juce::File{},
            "*.wav;*.mp3;*.flac;*.ogg;*.aiff;*.aif"
        );

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles,

        [this, chooser](
            const juce::FileChooser& fileChooser)
        {
            const auto file =
                fileChooser.getResult();

            if (!file.existsAsFile())
                return;

            if (!audioEngine.loadFile(file))
                return;

            // =================================================
            // Update queue
            // =================================================

            queueManager.clear();

            audioEngine.stop();

            queueManager.add(file);

            queueManager.setCurrentIndex(0);

            // =================================================
            // Metadata
            // =================================================

            trackTitle =
                audioEngine.getTrackTitle();

            trackArtist =
                audioEngine.getTrackArtist();

            trackAlbum =
                audioEngine.getTrackAlbum();

            autoNextTriggered = false;

            audioEngine.play();

            updateQueueUI();
            updatePlayButton();
            updateTimeLabels();
            updateVolumeUI();

            repaint();
        }
    );
}

// ============================================================
// Add Files
// ============================================================

void MainComponent::addFiles()
{
    auto chooser =
        std::make_shared<juce::FileChooser>(
            "Add audio files to queue",
            juce::File{},
            "*.wav;*.mp3;*.flac;*.ogg;*.aiff;*.aif"
        );

    chooser->launchAsync(
        juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::canSelectMultipleItems,

        [this, chooser](
            const juce::FileChooser& fileChooser)
        {
            const auto files =
                fileChooser.getResults();

            if (files.isEmpty())
                return;

            const bool queueWasEmpty =
                queueManager.isEmpty();

            // Add tất cả file vào Queue
            queueManager.add(files);

            // Cập nhật UI trước
            updateQueueUI();
            queueListBox.repaint();
            repaint();

            // Nếu Queue trước đó rỗng,
            // phát file đầu tiên
            if (queueWasEmpty &&
                queueManager.getSize() > 0)
            {
                queueManager.setCurrentIndex(0);

                playQueueIndex(0);
            }
        }
    );
}

// ============================================================
// Play Queue Index
// ============================================================

void MainComponent::playQueueIndex(
    int index)
{
    if (!juce::isPositiveAndBelow(
        index,
        queueManager.getSize()))
    {
        return;
    }

    const auto file =
        queueManager.getFile(index);

    if (!audioEngine.loadFile(file))
        return;

    queueManager.setCurrentIndex(index);

    // ========================================================
    // Metadata
    // ========================================================

    trackTitle =
        audioEngine.getTrackTitle();

    trackArtist =
        audioEngine.getTrackArtist();

    trackAlbum =
        audioEngine.getTrackAlbum();

    autoNextTriggered = false;

    audioEngine.play();

    updateQueueUI();
    updatePlayButton();
    updateTimeLabels();
    updateVolumeUI();

    repaint();
}

// ============================================================
// Play Next
// ============================================================

void MainComponent::playNext()
{
    if (!queueManager.hasNext())
    {
        audioEngine.stop();

        updatePlayButton();
        updateTimeLabels();

        return;
    }

    if (queueManager.next())
    {
        playQueueIndex(
            queueManager.getCurrentIndex()
        );
    }
}

// ============================================================
// Play Previous
// ============================================================

void MainComponent::playPrevious()
{
    const auto length =
        audioEngine.getLength();

    const auto position =
        audioEngine.getPosition();

    if (length > 0.0 &&
        position > 3.0)
    {
        audioEngine.setPosition(0.0);

        autoNextTriggered = false;

        updateTimeLabels();

        return;
    }

    if (!queueManager.hasPrevious())
    {
        audioEngine.setPosition(0.0);

        autoNextTriggered = false;

        updateTimeLabels();

        return;
    }

    if (queueManager.previous())
    {
        playQueueIndex(
            queueManager.getCurrentIndex()
        );
    }
}

// ============================================================
// Remove Selected Queue Item
// ============================================================

void MainComponent::removeSelectedQueueItem()
{
    const int selectedRow =
        queueListBox.getSelectedRow();

    if (!juce::isPositiveAndBelow(
        selectedRow,
        queueManager.getSize()))
    {
        return;
    }

    const bool removingCurrent =
        selectedRow
        == queueManager.getCurrentIndex();

    queueManager.remove(selectedRow);

    if (queueManager.isEmpty())
    {
        audioEngine.stop();

        trackTitle =
            "No track loaded";

        trackArtist =
            "Unknown Artist";

        trackAlbum =
            "Unknown Album";

        autoNextTriggered = false;
    }
    else if (removingCurrent)
    {
        const auto current =
            queueManager.getCurrentIndex();

        if (current >= 0)
        {
            playQueueIndex(current);
        }
    }

    updateQueueUI();
    updatePlayButton();
    updateTimeLabels();

    repaint();
}

// ============================================================
// Clear Queue
// ============================================================

void MainComponent::clearQueue()
{
    queueManager.clear();

    audioEngine.stop();

    trackTitle =
        "No track loaded";

    trackArtist =
        "Unknown Artist";

    trackAlbum =
        "Unknown Album";

    autoNextTriggered = false;

    updateQueueUI();
    updatePlayButton();
    updateTimeLabels();

    repaint();
}

// ============================================================
// Queue - Number Of Rows
// ============================================================

int MainComponent::getNumRows()
{
    return queueManager.getSize();
}

// ============================================================
// Queue - Paint Item
// ============================================================

void MainComponent::paintListBoxItem(
    int rowNumber,
    juce::Graphics& g,
    int width,
    int height,
    bool rowIsSelected)
{
    if (!juce::isPositiveAndBelow(
        rowNumber,
        queueManager.getSize()))
    {
        return;
    }

    const auto file =
        queueManager.getFile(rowNumber);

    const bool isCurrent =
        rowNumber == queueManager.getCurrentIndex();

    // ========================================================
    // Background
    // ========================================================

    if (isCurrent)
    {
        g.setColour(
            juce::Colours::white.withAlpha(0.10f)
        );

        g.fillRect(
            0,
            0,
            width,
            height
        );
    }
    else if (rowIsSelected)
    {
        g.setColour(
            juce::Colours::white.withAlpha(0.05f)
        );

        g.fillRect(
            0,
            0,
            width,
            height
        );
    }

    // ========================================================
    // Track number
    // ========================================================

    g.setColour(
        isCurrent
        ? juce::Colours::white
        : juce::Colours::white.withAlpha(0.35f)
    );

    g.setFont(
        juce::FontOptions(12.0f)
    );

    g.drawText(
        juce::String(rowNumber + 1),
        8,
        0,
        28,
        height,
        juce::Justification::centred
    );

    // ========================================================
    // Filename
    // ========================================================

    g.setColour(
        isCurrent
        ? juce::Colours::white
        : juce::Colours::white.withAlpha(0.75f)
    );

    g.setFont(
        juce::FontOptions(13.0f)
        .withStyle(
            isCurrent ? "Bold" : "Plain"
        )
    );

    // Reserve space for duration on the right.
    const int durationWidth = 60;

    g.drawText(
        file.getFileNameWithoutExtension(),
        42,
        0,
        width - 54,
        height,
        juce::Justification::centredLeft,
        true
    );
}

// ============================================================
// Queue - Selection Changed
// ============================================================

void MainComponent::selectedRowsChanged(
    int lastRowSelected)
{
    juce::ignoreUnused(
        lastRowSelected
    );
}

// ============================================================
// Queue - Double Click
// ============================================================

void MainComponent::listBoxItemDoubleClicked(
    int row,
    const juce::MouseEvent& event)
{
    juce::ignoreUnused(
        event
    );

    playQueueIndex(row);
}

// ============================================================
// Queue - Delete Key
// ============================================================

void MainComponent::deleteKeyPressed(
    int lastRowSelected)
{
    juce::ignoreUnused(
        lastRowSelected
    );

    removeSelectedQueueItem();
}

// ============================================================
// Audio Settings
// ============================================================

void MainComponent::openAudioSettings()
{
    auto* settings =
        new juce::AudioDeviceSelectorComponent(
            audioEngine.getDeviceManager(),

            0,
            0,

            2,
            2,

            true,
            false,
            true,
            false
        );

    settings->setSize(
        600,
        500
    );

    juce::DialogWindow::LaunchOptions options;

    options.content.setOwned(
        settings
    );

    options.dialogTitle =
        "VirtualAMP Audio Settings";

    options.dialogBackgroundColour =
        juce::Colour::fromRGB(
            30,
            30,
            30
        );

    options.escapeKeyTriggersCloseButton =
        true;

    options.useNativeTitleBar =
        true;

    options.resizable =
        true;

    options.launchAsync();
}