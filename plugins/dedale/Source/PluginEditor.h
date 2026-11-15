#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class DedaleProcessor;

// Panneau provisoire. Le vrai panneau arrive en phase 5.
class DedaleEditor final : public juce::AudioProcessorEditor
{
public:
    explicit DedaleEditor(DedaleProcessor& processor);

    void paint(juce::Graphics& g) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DedaleEditor)
};
