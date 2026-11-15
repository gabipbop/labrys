#include "PluginProcessor.h"

#include "Calibration.h"
#include "PluginEditor.h"

#include <cmath>

namespace cal = dedale::calibration;

namespace
{
constexpr int kSyncHost = 1;

// Plage exponentielle : autant de course pour chaque octave (fréquences, temps, tempo).
juce::NormalisableRange<float> exponentialRange(float minimum, float maximum)
{
    const auto ratio = maximum / minimum;
    return {minimum, maximum, [ratio](float start, float, float t) { return start * std::pow(ratio, t); },
            [ratio](float start, float, float v) { return std::log(v / start) / std::log(ratio); }};
}

struct ContinuousSpec
{
    const char* id;
    const char* name;
    float minimum;
    float maximum;
    float defaultValue;
    bool exponential = false;
};

// Paramètres continus du §10, dans l'ordre de DedaleProcessor::Continuous.
constexpr std::array<ContinuousSpec, DedaleProcessor::numContinuous> kContinuous{{
    {"vco_freq", "VCO FREQUENCY", 20.0f, 5000.0f, 110.0f, true},
    {"vco_eg1_amt", "VCO EG1 AMT", -1.0f, 1.0f, 0.0f},
    {"vco_seq1_amt", "VCO SEQ1 AMT", 0.0f, 1.0f, 1.0f},
    {"mvco_freq", "MOD VCO FREQUENCY", 0.1f, 1300.0f, 55.0f, true},
    {"mvco_eg1_amt", "MOD VCO EG1 AMT", -1.0f, 1.0f, 0.0f},
    {"mvco_seq2_amt", "MOD VCO SEQ2 AMT", 0.0f, 1.0f, 0.0f},
    {"mvco_fm_amt", "FM AMT", 0.0f, 1.0f, 0.0f},
    {"mix_vco", "VCO LVL", 0.0f, 1.0f, 0.5f},
    {"mix_ring", "RING MOD LVL", 0.0f, 1.0f, 0.0f},
    {"mix_mvco", "MOD VCO LVL", 0.0f, 1.0f, 0.0f},
    {"mix_noise", "NOISE LVL", 0.0f, 1.0f, 0.0f},
    {"noise_tone", "NOISE TONE", 0.0f, 1.0f, 0.5f},
    {"vcw_fold", "VCW FOLD", 0.0f, 1.0f, 0.3f},
    {"vcw_bias", "VCW BIAS", -1.0f, 1.0f, 0.0f},
    {"vcw_eg1_amt", "VCW EG1/CV AMT", -1.0f, 1.0f, 0.3f},
    {"vcw_seq1_amt", "VCW SEQ1 AMT", 0.0f, 1.0f, 0.0f},
    {"vcf_cutoff", "VCF CUTOFF", 20.0f, 20000.0f, 800.0f, true},
    {"vcf_eg1_amt", "VCF EG1/CV AMT", -1.0f, 1.0f, 0.3f},
    {"vcf_seq2_amt", "VCF SEQ2 AMT", 0.0f, 1.0f, 0.0f},
    {"vcf_res", "RESONANCE", 0.0f, 1.0f, 0.2f},
    {"vcf_mode", "FILTER MODE", 0.0f, 1.0f, 0.0f},
    {"blend", "BLEND", 0.0f, 1.0f, 0.5f},
    {"eg1_decay", "EG1 DECAY", 0.01f, 10.0f, 0.3f, true},
    {"eg2_decay", "EG2 DECAY", 0.01f, 10.0f, 0.4f, true},
    {"eg_trig_mix", "EG TRIG MIX", 0.0f, 1.0f, 0.0f},
    {"volume", "VOLUME", 0.0f, 1.0f, 0.7f},
    {"umix1_lvl", "U MIX 1 LVL", 0.0f, 1.0f, 0.0f},
}};
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout DedaleProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Int = juce::AudioParameterInt;
    using Bool = juce::AudioParameterBool;
    const auto id = [](const char* name) { return juce::ParameterID{name, 1}; };
    const juce::NormalisableRange<float> unit{0.0f, 1.0f};

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& spec : kContinuous)
    {
        const auto range = spec.exponential ? exponentialRange(spec.minimum, spec.maximum)
                                            : juce::NormalisableRange<float>{spec.minimum, spec.maximum};
        layout.add(std::make_unique<Float>(id(spec.id), spec.name, range, spec.defaultValue));
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        id("order"), "ORDER", juce::StringArray{"PARALLEL", "VCW>VCF", "VCF>VCW"}, 0));

    layout.add(std::make_unique<Float>(
        id("tempo"), "TEMPO", exponentialRange(cal::kTempoMinStepsPerMinute, cal::kTempoMaxStepsPerMinute),
        cal::kTempoDefaultStepsPerMinute));
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
    for (std::size_t i = 0; i < kContinuous.size(); ++i)
        continuousParams[i] = apvts.getRawParameterValue(kContinuous[i].id);
    orderParam = apvts.getRawParameterValue("order");
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
    engine.sequencer().seed(seed);
    dedale::Rng pattern(seed ^ 0xa5a5a5a5u);
    for (int lane = 0; lane < dedale::kNumLanes; ++lane)
        for (int step = 0; step < dedale::kMaxSteps; ++step)
            engine.sequencer().setStep(lane, step, pattern.uniform() < 0.5f, pattern.bipolarVolts());
    engine.sequencer().reset();
}

void DedaleProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    maxChunk = juce::jmax(1, samplesPerBlock);
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        1, static_cast<size_t>(cal::kOversamplingStages),
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampling->initProcessing(static_cast<size_t>(maxChunk));
    scratch.setSize(1, maxChunk);

    internalClock.prepare(sampleRate);
    hostClock.reset();
    engine.prepare(sampleRate, static_cast<int>(oversampling->getOversamplingFactor()), maxChunk);
    wasRunning = false;

    smoothingCoefficient =
        1.0f - std::exp(-1.0f / (cal::kParamSmoothingSeconds * static_cast<float>(sampleRate)));
    for (std::size_t i = 0; i < smoothed.size(); ++i)
        smoothed[i] = continuousParams[i]->load(); // pas de glissement au démarrage
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
    engine.sequencer().setRunning(running);
    if (running && !wasRunning)
        internalClock.reset(); // le premier pas tombe dès l'appui sur RUN
    wasRunning = running;

    if (const auto mode = juce::roundToInt(quantModeParam->load()) - 1; mode != lastQuantMode)
        engine.sequencer().setQuantizerMode(lastQuantMode = mode);
    if (const auto chain = chainParam->load() > 0.5f ? 1 : 0; chain != lastChain)
        engine.sequencer().setChain((lastChain = chain) == 1);

    for (std::size_t lane = 0; lane < corruptParams.size(); ++lane)
    {
        engine.sequencer().setCorrupt(static_cast<int>(lane), corruptParams[lane]->load());
        engine.sequencer().setCvRange(static_cast<int>(lane), rangeParams[lane]->load());
    }
    for (std::size_t i = 0; i < targets.size(); ++i)
        targets[i] = continuousParams[i]->load();
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
    if (oversampling == nullptr)
        return;

    pullParameters();
    const auto useHost = startHostClock();

    // Par tranches d'au plus la taille de bloc annoncée : le suréchantillonneur est dimensionné pour elle.
    auto* output = buffer.getWritePointer(0);
    auto event = midi.cbegin();
    for (int offset = 0; offset < buffer.getNumSamples(); offset += maxChunk)
        renderChunk(output + offset, juce::jmin(maxChunk, buffer.getNumSamples() - offset), event,
                    midi.cend(), offset, useHost);

    for (int channel = 1; channel < buffer.getNumChannels(); ++channel)
        buffer.copyFrom(channel, 0, buffer, 0, 0, buffer.getNumSamples()); // signal mono dupliqué (§1)
    midi.clear();
}

// Contrôle (séquenceur, enveloppes, triggers) à la fréquence de base ; audio dans le bloc suréchantillonné
// (§12).
void DedaleProcessor::renderChunk(float* output, int numSamples, juce::MidiBufferIterator& event,
                                  const juce::MidiBufferIterator& lastEvent, int offset,
                                  bool useHost) noexcept
{
    const auto division = juce::roundToInt(clockDivParam->load());
    const auto stepsPerMinute = static_cast<double>(tempoParam->load());
    const auto order = static_cast<dedale::Order>(juce::roundToInt(orderParam->load()));
    const auto factor = static_cast<int>(oversampling->getOversamplingFactor());

    scratch.clear(0, 0, numSamples);
    juce::dsp::AudioBlock<float> base(scratch.getArrayOfWritePointers(), 1, static_cast<size_t>(numSamples));
    auto oversampled = oversampling->processSamplesUp(base);
    auto* fast = oversampled.getChannelPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        // Note On : transpose la fondamentale du quantizer ; à l'arrêt, déclenche aussi les enveloppes
        // (§8.7).
        auto manualTrigger = false;
        for (; event != lastEvent && (*event).samplePosition <= offset + i; ++event)
        {
            if (const auto message = (*event).getMessage(); message.isNoteOn())
            {
                engine.sequencer().setRootSemitones(message.getNoteNumber() - cal::kMidiRootNote);
                manualTrigger = !engine.sequencer().isRunning();
            }
        }

        for (std::size_t p = 0; p < smoothed.size(); ++p)
            smoothed[p] += (targets[p] - smoothed[p]) * smoothingCoefficient;

        const auto internalTick = internalClock.processSample(stepsPerMinute);
        const auto tick = useHost ? hostClock.processSample(division) : internalTick;

        dedale::EngineParams params;
        params.eg1Decay = smoothed[eg1Decay];
        params.eg2Decay = smoothed[eg2Decay];
        params.egTrigMix = smoothed[egTrigMix];
        auto& v = params.voice;
        v.vcoFreq = smoothed[vcoFreq];
        v.vcoEg1Amt = smoothed[vcoEg1Amt];
        v.vcoSeq1Amt = smoothed[vcoSeq1Amt];
        v.mvcoFreq = smoothed[mvcoFreq];
        v.mvcoEg1Amt = smoothed[mvcoEg1Amt];
        v.mvcoSeq2Amt = smoothed[mvcoSeq2Amt];
        v.fmAmt = smoothed[mvcoFmAmt];
        v.mixVco = smoothed[mixVco];
        v.mixRing = smoothed[mixRing];
        v.mixMvco = smoothed[mixMvco];
        v.mixNoise = smoothed[mixNoise];
        v.noiseTone = smoothed[noiseTone];
        v.fold = smoothed[vcwFold];
        v.bias = smoothed[vcwBias];
        v.vcwEg1Amt = smoothed[vcwEg1Amt];
        v.vcwSeq1Amt = smoothed[vcwSeq1Amt];
        v.cutoff = smoothed[vcfCutoff];
        v.vcfEg1Amt = smoothed[vcfEg1Amt];
        v.vcfSeq2Amt = smoothed[vcfSeq2Amt];
        v.resonance = smoothed[vcfRes];
        v.filterMode = smoothed[vcfMode];
        v.order = order;
        v.blend = smoothed[blend];
        v.volume = smoothed[volume];
        v.umix1Level = smoothed[umix1Lvl];

        engine.processControl(params, tick, manualTrigger);
        for (int k = 0; k < factor; ++k)
            fast[i * factor + k] = engine.processAudio();
    }

    juce::dsp::AudioBlock<float> result(&output, 1, static_cast<size_t>(numSamples));
    oversampling->processSamplesDown(result);
}

juce::AudioProcessorEditor* DedaleProcessor::createEditor()
{
    return new DedaleEditor(*this);
}

// Seuls les paramètres sont sauvegardés pour l'instant. Séquence, buffer et réglages globaux : phase 6 (§10).
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
