#include "PluginProcessor.h"

#include "PluginEditor.h"

DedaleProcessor::DedaleProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void DedaleProcessor::prepareToPlay(double, int) {}

void DedaleProcessor::releaseResources() {}

bool DedaleProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void DedaleProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    midi.clear();
}

juce::AudioProcessorEditor* DedaleProcessor::createEditor()
{
    return new DedaleEditor(*this);
}

void DedaleProcessor::getStateInformation(juce::MemoryBlock&) {}

void DedaleProcessor::setStateInformation(const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DedaleProcessor();
}
