#pragma once

#include <JuceHeader.h>
#include "AudioEngine.h"
#include "QueueManager.h"

// ============================================================
// Main Component
// ============================================================

class MainComponent : public juce::Component,
    private juce::Timer,
    private juce::ListBoxModel
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:

    // ========================================================
    // Timer
    // ========================================================

    void timerCallback() override;

    // ========================================================
    // Drawing
    // ========================================================

    void drawHeader(juce::Graphics& g);
    void drawAlbumArea(juce::Graphics& g);
    void drawSpectrum(juce::Graphics& g);
    void drawWaveform(juce::Graphics& g);
    void drawPlayerArea(juce::Graphics& g);

    // ========================================================
    // File / Audio
    // ========================================================

    void openFile();
    void addFiles();
    void openAudioSettings();

    // ========================================================
    // WASAPI
    // ========================================================

    void setWASAPIMode(int mode);
    void updateWASAPIModeUI();

    // ========================================================
    // Playback
    // ========================================================

    void playQueueIndex(int index);
    void playNext();
    void playPrevious();

    // ========================================================
    // UI
    // ========================================================

    void updatePlayButton();
    void updateTimeLabels();
    void updateVolumeUI();
    void updateQueueUI();

    // ========================================================
    // Queue
    // ========================================================

    void removeSelectedQueueItem();
    void clearQueue();

    // ========================================================
    // ListBoxModel
    // ========================================================

    int getNumRows() override;

    void paintListBoxItem(
        int rowNumber,
        juce::Graphics& g,
        int width,
        int height,
        bool rowIsSelected
    ) override;

    void selectedRowsChanged(
        int lastRowSelected
    ) override;

    void listBoxItemDoubleClicked(
        int row,
        const juce::MouseEvent& event
    ) override;

    void deleteKeyPressed(
        int lastRowSelected
    ) override;

    // ========================================================
    // Header
    // ========================================================

    juce::TextButton openButton;
    juce::TextButton addButton;
    juce::TextButton settingsButton;

    // ========================================================
    // WASAPI
    // ========================================================

    juce::ComboBox wasapiModeBox;

    // ========================================================
    // Transport
    // ========================================================

    juce::TextButton previousButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton nextButton;

    // ========================================================
    // Volume
    // ========================================================

    juce::TextButton muteButton;
    juce::Slider volumeSlider;

    // ========================================================
    // Seek
    // ========================================================

    juce::Slider positionSlider;

    // ========================================================
    // Time
    // ========================================================

    juce::Label currentTimeLabel;
    juce::Label totalTimeLabel;

    // ========================================================
    // Device
    // ========================================================

    juce::Label deviceLabel;
    juce::Label formatLabel;

    // ========================================================
    // Queue UI
    // ========================================================

    juce::Label queueTitleLabel;

    // ========================================================
    // Custom Queue ListBox
    // ========================================================

    class QueueListBox : public juce::ListBox,
        private juce::MouseListener
    {
    public:
        QueueListBox()
        {
            // Bắt mouse event từ cả các row/component con
            getViewport()->addMouseListener(
                this,
                true
            );
        }

        ~QueueListBox() override
        {
            getViewport()->removeMouseListener(this);
        }

        std::function<void(int, int)> onReorder;

    private:

        // ========================================================
        // Mouse Down
        // ========================================================

        void mouseDown(
            const juce::MouseEvent& event
        ) override
        {
            const auto localEvent =
                event.getEventRelativeTo(this);

            if (!localEvent.mods.isLeftButtonDown())
                return;

            dragRow =
                getRowContainingPosition(
                    localEvent.x,
                    localEvent.y
                );

            dragStartY = localEvent.y;
            dragging = false;
        }

        // ========================================================
        // Mouse Drag
        // ========================================================

        void mouseDrag(
            const juce::MouseEvent& event
        ) override
        {
            if (dragRow < 0)
                return;

            if (!event.mods.isLeftButtonDown())
                return;

            const auto localEvent =
                event.getEventRelativeTo(this);

            const int distance =
                std::abs(
                    localEvent.y - dragStartY
                );

            // Chỉ bắt đầu drag khi chuột di chuyển đủ xa
            if (distance >= 8)
                dragging = true;
        }

        // ========================================================
        // Mouse Up
        // ========================================================

        void mouseUp(
            const juce::MouseEvent& event
        ) override
        {
            if (dragging)
            {
                const auto localEvent =
                    event.getEventRelativeTo(this);

                const int targetRow =
                    getRowContainingPosition(
                        localEvent.x,
                        localEvent.y
                    );

                if (auto* model = getModel())
                {
                    const int numRows =
                        model->getNumRows();

                    if (targetRow >= 0 &&
                        targetRow < numRows &&
                        dragRow != targetRow &&
                        onReorder != nullptr)
                    {
                        onReorder(
                            dragRow,
                            targetRow
                        );
                    }
                }
            }

            dragRow = -1;
            dragStartY = 0;
            dragging = false;
        }

        // ========================================================
        // Drag State
        // ========================================================

        int dragRow = -1;
        int dragStartY = 0;
        bool dragging = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
            QueueListBox
        )
    };

    QueueListBox queueListBox;

    juce::TextButton removeQueueButton;
    juce::TextButton clearQueueButton;

    // ========================================================
    // Track information
    // ========================================================

    juce::String trackTitle{
        "No track loaded"
    };

    juce::String trackArtist{
        "Unknown Artist"
    };

    juce::String trackAlbum{
        "Unknown Album"
    };

    // ========================================================
    // Engine
    // ========================================================

    AudioEngine audioEngine;

    QueueManager queueManager;

    // ========================================================
    // Spectrum
    // ========================================================

    std::array<float, 2048> spectrumData{};

    std::array<float, 128> smoothedBars{};

    // ========================================================
    // Layout
    // ========================================================

    juce::Rectangle<int> albumArea;

    juce::Rectangle<int> spectrumArea;

    juce::Rectangle<int> queueArea;

    juce::Rectangle<int> waveformArea;

    juce::Rectangle<int> playerArea;

    static constexpr int headerHeight = 64;

    // ========================================================
    // Auto-next state
    // ========================================================

    bool autoNextTriggered = false;

    // ========================================================
    // Queue Drag State
    // ========================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        MainComponent
    );
};