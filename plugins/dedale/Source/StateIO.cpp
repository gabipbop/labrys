#include "StateIO.h"

namespace dedale::stateio
{
const juce::Identifier kVersion{"stateVersion"}, kSequencer{"SEQUENCER"}, kCables{"CABLES"},
    kSettings{"SETTINGS"};

namespace
{
const juce::Identifier kLane{"LANE"}, kBuffer{"BUFFER"};

juce::String bitsToString(const std::array<bool, kMaxSteps>& bits)
{
    juce::String text;
    for (const auto bit : bits)
        text << (bit ? '1' : '0');
    return text;
}

void bitsFromString(const juce::String& text, std::array<bool, kMaxSteps>& bits)
{
    for (std::size_t i = 0; i < bits.size() && static_cast<int>(i) < text.length(); ++i)
        bits[i] = text[static_cast<int>(i)] == '1';
}

juce::String voltsToString(const std::array<float, kMaxSteps>& volts)
{
    juce::StringArray tokens;
    for (const auto v : volts)
        tokens.add(juce::String(v, 9)); // assez de décimales pour relire exactement le même float
    return tokens.joinIntoString(" ");
}

void voltsFromString(const juce::String& text, std::array<float, kMaxSteps>& volts)
{
    const auto tokens = juce::StringArray::fromTokens(text, " ", "");
    for (std::size_t i = 0; i < volts.size() && static_cast<int>(i) < tokens.size(); ++i)
        volts[i] = tokens[static_cast<int>(i)].getFloatValue();
}
} // namespace

juce::ValueTree toTree(const SequencerState& s)
{
    juce::ValueTree tree(kSequencer);
    tree.setProperty("chained", s.chained, nullptr);
    tree.setProperty("quant", s.quantMode, nullptr);
    tree.setProperty("root", s.rootSemitones, nullptr);
    tree.setProperty("unipolar", s.unipolar, nullptr);
    for (std::size_t i = 0; i < s.lanes.size(); ++i)
    {
        juce::ValueTree lane(kLane);
        lane.setProperty("bits", bitsToString(s.lanes[i].bit), nullptr);
        lane.setProperty("volts", voltsToString(s.lanes[i].voltage), nullptr);
        lane.setProperty("length", s.lanes[i].length, nullptr);
        lane.setProperty("playHead", s.lanes[i].playHead, nullptr);
        lane.setProperty("writeOffset", s.lanes[i].writeOffset, nullptr);
        lane.setProperty("rotation", s.rotations[i], nullptr);
        lane.setProperty("held", static_cast<double>(s.heldVolts[i]), nullptr);
        lane.setProperty("playCurrentNext", s.playCurrentStepNext[i], nullptr);
        tree.appendChild(lane, nullptr);
    }

    juce::ValueTree buffer(kBuffer);
    buffer.setProperty("valid", s.buffer.valid, nullptr);
    buffer.setProperty("chained", s.buffer.chained, nullptr);
    buffer.setProperty("quant", s.buffer.quantMode, nullptr);
    for (std::size_t i = 0; i < s.lanes.size(); ++i)
    {
        juce::ValueTree lane(kLane);
        lane.setProperty("bits", bitsToString(s.buffer.bit[i]), nullptr);
        lane.setProperty("volts", voltsToString(s.buffer.voltage[i]), nullptr);
        lane.setProperty("length", s.buffer.length[i], nullptr);
        lane.setProperty("writeOffset", s.buffer.writeOffset[i], nullptr);
        buffer.appendChild(lane, nullptr);
    }
    tree.appendChild(buffer, nullptr);
    return tree;
}

SequencerState sequencerFromTree(const juce::ValueTree& tree, SequencerState s)
{
    if (!tree.hasType(kSequencer))
        return s;
    s.chained = tree.getProperty("chained", s.chained);
    s.quantMode = tree.getProperty("quant", s.quantMode);
    s.rootSemitones = tree.getProperty("root", s.rootSemitones);
    s.unipolar = tree.getProperty("unipolar", s.unipolar);

    std::size_t i = 0;
    for (const auto& lane : tree)
    {
        if (!lane.hasType(kLane) || i >= s.lanes.size())
            continue;
        bitsFromString(lane.getProperty("bits").toString(), s.lanes[i].bit);
        voltsFromString(lane.getProperty("volts").toString(), s.lanes[i].voltage);
        s.lanes[i].length = lane.getProperty("length", s.lanes[i].length);
        s.lanes[i].playHead = lane.getProperty("playHead", s.lanes[i].playHead);
        s.lanes[i].writeOffset = lane.getProperty("writeOffset", s.lanes[i].writeOffset);
        s.rotations[i] = lane.getProperty("rotation", s.rotations[i]);
        s.heldVolts[i] = static_cast<float>(
            static_cast<double>(lane.getProperty("held", static_cast<double>(s.heldVolts[i]))));
        s.playCurrentStepNext[i] = lane.getProperty("playCurrentNext", s.playCurrentStepNext[i]);
        ++i;
    }

    if (const auto buffer = tree.getChildWithName(kBuffer); buffer.isValid())
    {
        s.buffer.valid = buffer.getProperty("valid", false);
        s.buffer.chained = buffer.getProperty("chained", false);
        s.buffer.quantMode = buffer.getProperty("quant", 0);
        i = 0;
        for (const auto& lane : buffer)
        {
            if (!lane.hasType(kLane) || i >= s.lanes.size())
                continue;
            bitsFromString(lane.getProperty("bits").toString(), s.buffer.bit[i]);
            voltsFromString(lane.getProperty("volts").toString(), s.buffer.voltage[i]);
            s.buffer.length[i] = lane.getProperty("length", kMaxSteps);
            s.buffer.writeOffset[i] = lane.getProperty("writeOffset", 0);
            ++i;
        }
    }
    return s;
}

juce::ValueTree toTree(const Cables& cables)
{
    juce::StringArray tokens;
    for (const auto source : cables)
        tokens.add(juce::String(source));
    juce::ValueTree tree(kCables);
    tree.setProperty("sources", tokens.joinIntoString(" "), nullptr);
    return tree;
}

Cables cablesFromTree(const juce::ValueTree& tree)
{
    Cables cables;
    cables.fill(-1);
    const auto tokens = juce::StringArray::fromTokens(tree.getProperty("sources").toString(), " ", "");
    for (std::size_t i = 0; i < cables.size() && static_cast<int>(i) < tokens.size(); ++i)
    {
        const auto source = tokens[static_cast<int>(i)].getIntValue();
        const auto usable = source >= 0 && source < kNumOutputs && static_cast<In>(i) != In::midi;
        cables[i] = usable ? source : -1;
    }
    return cables;
}

juce::ValueTree toTree(const GlobalSettings& s)
{
    juce::ValueTree tree(kSettings);
    tree.setProperty("resetRecallsBuffer", s.resetRecallsBuffer, nullptr);
    tree.setProperty("unipolarCv", s.unipolarCv, nullptr);
    tree.setProperty("analogDrift", s.analogDrift, nullptr);
    tree.setProperty("vcwZeroColour", s.vcwZeroColour, nullptr);
    tree.setProperty("midiOut", s.midiOut, nullptr);
    tree.setProperty("oversampling", s.oversampling, nullptr);
    tree.setProperty("midiChannel", s.midiChannel, nullptr);
    return tree;
}

GlobalSettings settingsFromTree(const juce::ValueTree& tree, GlobalSettings s)
{
    if (!tree.hasType(kSettings))
        return s;
    s.resetRecallsBuffer = tree.getProperty("resetRecallsBuffer", s.resetRecallsBuffer);
    s.unipolarCv = tree.getProperty("unipolarCv", s.unipolarCv);
    s.analogDrift = tree.getProperty("analogDrift", s.analogDrift);
    s.vcwZeroColour = tree.getProperty("vcwZeroColour", s.vcwZeroColour);
    s.midiOut = tree.getProperty("midiOut", s.midiOut);
    s.oversampling = tree.getProperty("oversampling", s.oversampling);
    s.midiChannel = tree.getProperty("midiChannel", s.midiChannel);
    return unpack(pack(s)); // ramène les valeurs hors bornes dans les bornes
}
} // namespace dedale::stateio
