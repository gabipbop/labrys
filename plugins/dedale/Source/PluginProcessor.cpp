#include "PluginProcessor.h"

#include "Calibration.h"
#include "PluginEditor.h"

#include <cmath>

namespace cal = dedale::calibration;

namespace
{
constexpr int kSyncHost = 1;

// TEMPO exponentiel : autant de course pour 10 → 20 que pour 300 → 600 pas/min.
juce::NormalisableRange<float> tempoRange()
{
    const auto ratio = cal::kTempoMaxStepsPerMinute / cal::kTempoMinStepsPerMinute;
    return {cal::kTempoMinStepsPerMinute, cal::kTempoMaxStepsPerMinute,
            [ratio](float start, float, float t) { return start * std::pow(ratio, t); },
            [ratio](float start, float, float v) { return std::log(v / start) / std::log(ratio); }};
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout DedaleProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Int = juce::AudioParameterInt;
    using Bool = juce::AudioParameterBool;
    const auto id = [](const char* name) { return juce::ParameterID{name, 1}; };
    const juce::NormalisableRange<float> unit{0.0f, 1.0f};

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<Float>(id("tempo"), "TEMPO", tempoRange(), cal::kTempoDefaultStepsPerMinute));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        id("sync_mode"), "SYNC", juce::StringArray{"INTERNAL", "HOST"}, kSyncHost));
    layout.add(std::make_unique<Int>(id("clock_div"), "DIVISION", 1, 16, 1));
    layout.add(std::make_unique<Float>(id("seq1_corrupt"), "SEQ1 CORRUPT", unit, 0.0f));
    layout.add(std::make_unique<Float>(id("seq2_corrupt"), "SEQ2 CORRUPT", unit, 0.0f));
    layout.add(std::make_unique<Float>(id("seq1_range"), "SEQ1 CV RANGE", unit, 0.3f));
    layout.add(std::make_unique<Float>(id("seq2_range"), "SEQ2 CV RANGE", unit, 0.3f));
    layout.add(std::make_unique<Bool>(id("run"), "RUN/STOP", false));
    layout.add(std::make_unique<Bool>(id("chain"), "CHAIN SEQ", false));
    layout.add(std::make_unique<Int>(id("quant_mode"), "QUANTIZER", 1, 16, cal::kDefaultQuantizerMode + 1));
    return layout;
}

DedaleProcessor::DedaleProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "DEDALE", createParameterLayout())
{
    tempoParam = apvts.getRawParameterValue("tempo");
    syncModeParam = apvts.getRawParameterValue("sync_mode");
    clockDivParam = apvts.getRawParameterValue("clock_div");
    corruptParams = {apvts.getRawParameterValue("seq1_corrupt"), apvts.getRawParameterValue("seq2_corrupt")};
    rangeParams = {apvts.getRawParameterValue("seq1_range"), apvts.getRawParameterValue("seq2_range")};
    runParam = apvts.getRawParameterValue("run");
    chainParam = apvts.getRawParameterValue("chain");
    quantModeParam = apvts.getRawParameterValue("quant_mode");

    seedSequencer(static_cast<std::uint32_t>(juce::Random::getSystemRandom().nextInt()));
}

void DedaleProcessor::seedSequencer(std::uint32_t seed) noexcept
{
    seq.seed(seed);
    dedale::Rng pattern(seed ^ 0xa5a5a5a5u);
    for (int lane = 0; lane < dedale::kNumLanes; ++lane)
        for (int step = 0; step < dedale::kMaxSteps; ++step)
            seq.setStep(lane, step, pattern.uniform() < 0.5f, pattern.bipolarVolts());
    seq.reset();
}

void DedaleProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    internalClock.prepare(sampleRate);
    hostClock.reset();
    for (auto& beep : beeps)
        beep.prepare(sampleRate, samplesPerBlock);
    wasRunning = false;
}

void DedaleProcessor::releaseResources() {}

bool DedaleProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void DedaleProcessor::pullParameters() noexcept
{
    const auto running = runParam->load() > 0.5f;
    seq.setRunning(running);
    if (running && !wasRunning)
        internalClock.reset(); // le premier pas tombe dès l'appui sur RUN
    wasRunning = running;

    if (const auto mode = juce::roundToInt(quantModeParam->load()) - 1; mode != lastQuantMode)
        seq.setQuantizerMode(lastQuantMode = mode);
    if (const auto chain = chainParam->load() > 0.5f ? 1 : 0; chain != lastChain)
        seq.setChain((lastChain = chain) == 1);

    for (std::size_t lane = 0; lane < corruptParams.size(); ++lane)
    {
        seq.setCorrupt(static_cast<int>(lane), corruptParams[lane]->load());
        seq.setCvRange(static_cast<int>(lane), rangeParams[lane]->load());
    }
}

// Sync hôte : seulement quand SYNC = HOST et que le transport tourne ; sinon l'horloge interne prend le
// relais.
bool DedaleProcessor::startHostClock() noexcept
{
    if (juce::roundToInt(syncModeParam->load()) == kSyncHost)
    {
        if (auto* head = getPlayHead())
        {
            if (const auto position = head->getPosition(); position.hasValue() && position->getIsPlaying())
            {
                const auto ppq = position->getPpqPosition();
                const auto bpm = position->getBpm();
                if (ppq.hasValue() && bpm.hasValue())
                {
                    hostClock.beginBlock(*ppq, *bpm / 60.0 / getSampleRate());
                    return true;
                }
            }
        }
    }
    hostClock.reset();
    return false;
}

void DedaleProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    pullParameters();
    const auto useHost = startHostClock();
    const auto division = juce::roundToInt(clockDivParam->load());
    const auto stepsPerMinute = static_cast<double>(tempoParam->load());

    const auto stereo = buffer.getNumChannels() > 1;
    auto* left = buffer.getWritePointer(0);
    auto* right = stereo ? buffer.getWritePointer(1) : nullptr;
    auto event = midi.cbegin();

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // Note On : transpose la fondamentale du quantizer ; à l'arrêt, déclenche aussi un trigger (§8.7).
        auto manualTrigger = false;
        for (; event != midi.cend() && (*event).samplePosition <= i; ++event)
        {
            if (const auto message = (*event).getMessage(); message.isNoteOn())
            {
                seq.setRootSemitones(message.getNoteNumber() - cal::kMidiRootNote);
                manualTrigger = !seq.isRunning();
            }
        }

        // CLOCK 2 est normalisé sur CLOCK 1 : les deux lanes reçoivent le même tick (jacks en phase 4).
        const auto internalTick = internalClock.processSample(stepsPerMinute);
        const auto tick = useHost ? hostClock.processSample(division) : internalTick;

        std::array<float, dedale::kNumLanes> voice{};
        for (std::size_t lane = 0; lane < voice.size(); ++lane)
        {
            const auto laneIndex = static_cast<int>(lane);
            if ((tick && seq.tick(laneIndex)) || manualTrigger)
                beeps[lane].trigger();
            voice[lane] = beeps[lane].processSample(seq.cv(laneIndex));
        }

        if (stereo)
        {
            left[i] = voice[0] * cal::kBeepPan + voice[1] * (1.0f - cal::kBeepPan);
            right[i] = voice[0] * (1.0f - cal::kBeepPan) + voice[1] * cal::kBeepPan;
        }
        else
        {
            left[i] = 0.5f * (voice[0] + voice[1]);
        }
    }

    midi.clear();
}

juce::AudioProcessorEditor* DedaleProcessor::createEditor()
{
    return new DedaleEditor(*this);
}

// Phase 2 : seuls les paramètres sont sauvegardés. Séquence, buffer et réglages globaux : phase 6 (§10).
void DedaleProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (const auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void DedaleProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary(data, sizeInBytes);
        xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DedaleProcessor();
}
