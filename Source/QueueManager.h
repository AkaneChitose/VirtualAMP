#pragma once

#include <JuceHeader.h>

class QueueManager
{
public:
    QueueManager() = default;
    ~QueueManager() = default;

    // ============================================================
    // Add
    // ============================================================

    void add(const juce::File& file)
    {
        if (!file.existsAsFile())
            return;

        if (!files.contains(file))
            files.add(file);

        if (currentIndex < 0)
            currentIndex = 0;
    }

    void add(const juce::Array<juce::File>& newFiles)
    {
        for (const auto& file : newFiles)
            add(file);
    }

    // ============================================================
    // Remove
    // ============================================================

    void remove(int index)
    {
        if (!juce::isPositiveAndBelow(index, files.size()))
            return;

        files.remove(index);

        if (files.isEmpty())
        {
            currentIndex = -1;
            return;
        }

        if (index < currentIndex)
        {
            --currentIndex;
        }
        else if (index == currentIndex)
        {
            if (currentIndex >= files.size())
                currentIndex = files.size() - 1;
        }
    }

    // ============================================================
    // Clear
    // ============================================================

    void clear()
    {
        files.clear();
        currentIndex = -1;
    }

    // ============================================================
    // Move
    // ============================================================

    void move(int fromIndex, int toIndex)
    {
        if (!juce::isPositiveAndBelow(fromIndex, files.size()))
            return;

        if (toIndex < 0 || toIndex >= files.size())
            return;

        if (fromIndex == toIndex)
            return;

        const auto file = files[fromIndex];

        files.remove(fromIndex);
        files.insert(toIndex, file);

        if (currentIndex == fromIndex)
        {
            currentIndex = toIndex;
        }
        else if (fromIndex < currentIndex &&
            toIndex >= currentIndex)
        {
            --currentIndex;
        }
        else if (fromIndex > currentIndex &&
            toIndex <= currentIndex)
        {
            ++currentIndex;
        }
    }

    // ============================================================
    // Current
    // ============================================================

    int getCurrentIndex() const
    {
        return currentIndex;
    }

    void setCurrentIndex(int index)
    {
        if (juce::isPositiveAndBelow(index, files.size()))
            currentIndex = index;
    }

    // ============================================================
    // Information
    // ============================================================

    int getSize() const
    {
        return files.size();
    }

    bool isEmpty() const
    {
        return files.isEmpty();
    }

    const juce::File& getFile(int index) const
    {
        jassert(juce::isPositiveAndBelow(index, files.size()));

        return files[index];
    }

    const juce::File* getCurrentFile() const
    {
        if (!juce::isPositiveAndBelow(currentIndex, files.size()))
            return nullptr;

        return &files[currentIndex];
    }

    // ============================================================
    // Navigation
    // ============================================================

    bool hasNext() const
    {
        return currentIndex >= 0
            && currentIndex + 1 < files.size();
    }

    bool hasPrevious() const
    {
        return currentIndex > 0
            && currentIndex < files.size();
    }

    bool next()
    {
        if (!hasNext())
            return false;

        ++currentIndex;
        return true;
    }

    bool previous()
    {
        if (!hasPrevious())
            return false;

        --currentIndex;
        return true;
    }

private:

    juce::Array<juce::File> files;

    int currentIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        QueueManager
    )
};