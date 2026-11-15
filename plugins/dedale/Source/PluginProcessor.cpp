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
    char unit = '%'; // '%' : pourcentage, 'h' : hertz, 's' : secondes
};

juce::String describe(float value, char unit)
{
    if (unit == 'h')
        return juce::String(value, value < 100.0f ? 1 : 0) + " Hz";
    if (unit == 's')
        return value < 1.0f ? juce::String(juce::roundToInt(value * 1000.0f)) + " ms"
                            : juce::String(value, 2) + " s";
    return juce::String(juce::roundToInt(value * 100.0f)) + " %";
}

// Paramètres continus du §10, dans l'ordre de DedaleProcessor::Continuous.
constexpr std::array<ContinuousSpec, DedaleProcessor::numContinuous> kContinuous{{
    {"vco_freq", "VCO FREQUENCY", 20.0f, 5000.0f, 110.0f, true, 'h'},
    {"vco_eg1_amt", "VCO EG1 AMT", -1.0f, 1.0f, 0.0f},
    {"vco_seq1_amt", "VCO SEQ1 AMT", 0.0f, 1.0f, 1.0f},
    {"mvco_freq", "MOD VCO FREQUENCY", 0.1f, 1300.0f, 55.0f, true, 'h'},
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
    {"vcf_cutoff", "VCF CUTOFF", 20.0f, 20000.0f, 800.0f, true, 'h'},
    {"vcf_eg1_amt", "VCF EG1/CV AMT", -1.0f, 1.0f, 0.3f},
    {"vcf_seq2_amt", "VCF SEQ2 AMT", 0.0f, 1.0f, 0.0f},
    {"vcf_res", "RESONANCE", 0.0f, 1.0f, 0.2f},
    {"vcf_mode", "FILTER MODE", 0.0f, 1.0f, 0.0f},
    {"blend", "BLEND", 0.0f, 1.0f, 0.5f},
    {"eg1_decay", "EG1 DECAY", 0.01f, 10.0f, 0.3f, true, 's'},
    {"eg2_decay", "EG2 DECAY", 0.01f, 10.0f, 0.4f, true, 's'},
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
    const auto percent = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
        [](float value, int) { return describe(value, '%'); });
    for (const auto& spec : kContinuous)
    {
        const auto range = spec.exponential ? exponentialRange(spec.minimum, spec.maximum)
                                            : juce::NormalisableRange<float>{spec.minimum, spec.maximum};
        const auto attributes = juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [unit = spec.unit](float value, int) { return describe(value, unit); });
        layout.add(std::make_unique<Float>(id(spec.id), spec.name, range, spec.defaultValue, attributes));
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        id("order"), "ORDER", juce::StringArray{"PARALLEL", "VCW>VCF", "VCF>VCW"}, 0));

    // TEMPO : en pas par minute, avec l'équivalent en hertz (§8.6).
    layout.add(std::make_unique<Float>(
        id("tempo"), "TEMPO", exponentialRange(cal::kTempoMinStepsPerMinute, cal::kTempoMaxStepsPerMinute),
        cal::kTempoDefaultStepsPerMinute,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(
            [](float value, int)
            {
                return juce::String(juce::roundToInt(value)) + " pas/min (" + juce::String(value / 60.0f, 1) +
                       " Hz)";
            })));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        id("sync_mode"), "SYNC", juce::StringArray{"INTERNAL", "HOST"}, kSyncHost));
    layout.add(std::make_unique<Int>(id("clock_div"), "DIVISION", 1, 16, 1));
    layout.add(std::make_unique<Float>(id("seq1_corrupt"), "SEQ1 CORRUPT", unit, 0.0f, percent));
    layout.add(std::make_unique<Float>(id("seq2_corrupt"), "SEQ2 CORRUPT", unit, 0.0f, percent));
    layout.add(std::make_unique<Float>(id("seq1_range"), "SEQ1 CV RANGE", unit, 0.3f, percent));
    layout.add(std::make_unique<Float>(id("seq2_range"), "SEQ2 CV RANGE", unit, 0.3f, percent));
    layout.add(std::make_unique<Bool>(id("run"), "RUN/STOP", false));
    layout.add(std::make_unique<Bool>(id("chain"), "CHAIN SEQ", false));
    layout.add(std::make_unique<Int>(id("quant_mode"), "QUANTIZER", 1, 16, cal::kDefaultQuantizerMode + 1));
    return layout;
}

DedaleProcessor::DedaleProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "DEDALE", createParameterLayout())
{
    for (auto& cable : cables)
        cable.store(-1);
    appliedCables.fill(-1);
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
    publishDisplay();
    startTimerHz(15);
}

DedaleProcessor::~DedaleProcessor()
{
    stopTimer();
}

void DedaleProcessor::seedSequencer(std::uint32_t seed) noexcept
{
    engine.sequencer().seed(seed);
    dedale::Rng pattern(seed ^ 0xa5a5a5a5u);
    for (int lane = 0; lane < dedale::kNumLanes; ++lane)
        for (int step = 0; step < dedale::kMaxSteps; ++step)
            engine.sequencer().setStep(lane, step, pattern.uniform() < 0.5f, pattern.bipolarVolts());
    engine.sequencer().reset();
    publishedState = engine.sequencer().state();
}

void DedaleProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    maxChunk = juce::jmax(1, samplesPerBlock);
    currentSampleRate = sampleRate;
    scratch.setSize(1, maxChunk);
    internalClock.prepare(sampleRate);
    hostClock.reset();
    rebuildOversampling();
    wasRunning = false;

    smoothingCoefficient =
        1.0f - std::exp(-1.0f / (cal::kParamSmoothingSeconds * static_cast<float>(sampleRate)));
    for (std::size_t i = 0; i < smoothed.size(); ++i)
        smoothed[i] = continuousParams[i]->load(); // pas de glissement au démarrage
}

// Suréchantillonneur et moteur pour le facteur choisi dans les réglages (×2, ×4 ou ×8).
void DedaleProcessor::rebuildOversampling()
{
    const auto stages = juce::roundToInt(std::log2(static_cast<float>(settings().oversampling)));
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        1, static_cast<size_t>(stages), juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    oversampling->initProcessing(static_cast<size_t>(maxChunk));
    engine.prepare(currentSampleRate, static_cast<int>(oversampling->getOversamplingFactor()), maxChunk);
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

    if (const auto mode = juce::roundToInt(quantModeParam->load()) - 1; mode != lastQuantParam)
        engine.sequencer().setQuantizerMode(lastQuantParam = mode);
    if (const auto chain = chainParam->load() > 0.5f ? 1 : 0; chain != lastChainParam)
        engine.sequencer().setChain((lastChainParam = chain) == 1);

    blockSettings = dedale::unpack(settingsBits.load(std::memory_order_relaxed));
    engine.sequencer().setUnipolar(blockSettings.unipolarCv);
    engine.setResetRecallsBuffer(blockSettings.resetRecallsBuffer);

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
    applyCables();
    exchangeSequencerState();
    applyUiActions();
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

    // Oscilloscope : si l'interface ne lit pas, on arrête d'écrire (elle rattrapera au prochain bloc).
    const auto scope = scopeFifo.write(juce::jmin(buffer.getNumSamples(), scopeFifo.getFreeSpace()));
    std::copy_n(output, scope.blockSize1, scopeData.begin() + scope.startIndex1);
    std::copy_n(output + scope.blockSize1, scope.blockSize2, scopeData.begin() + scope.startIndex2);
    publishDisplay();
}

// --- Pont interface ↔ audio ------------------------------------------------------------------------------

void DedaleProcessor::perform(const dedale::Action& action)
{
    using Type = dedale::ActionType;
    const auto stepParameter = [this](const char* id, std::atomic<float>* raw, int delta, bool wrap)
    {
        auto* parameter = apvts.getParameter(id);
        auto value = juce::roundToInt(raw->load()) + delta;
        value = wrap ? (value + 15) % 16 + 1 : juce::jlimit(1, 16, value);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(value)));
    };

    switch (action.type)
    {
    case Type::quantizerNext:
        return stepParameter("quant_mode", quantModeParam, 1, true);
    case Type::quantizerPrevious:
        return stepParameter("quant_mode", quantModeParam, -1, true);
    case Type::clockDivisionUp:
        return stepParameter("clock_div", clockDivParam, 1, false);
    case Type::clockDivisionDown:
        return stepParameter("clock_div", clockDivParam, -1, false);
    default:
        break;
    }

    const auto slot = actionFifo.write(1);
    if (slot.blockSize1 > 0)
        actionQueue[static_cast<std::size_t>(slot.startIndex1)] = action;
}

void DedaleProcessor::applyUiActions() noexcept
{
    using Type = dedale::ActionType;
    auto& seq = engine.sequencer();
    const auto pending = actionFifo.read(actionFifo.getNumReady());
    const auto apply = [&](const dedale::Action& action)
    {
        const auto chainedSeq2 = seq.isChained() && action.lane == 1;
        switch (action.type)
        {
        case Type::bitFlipGate:
            engine.setBitFlipButton(action.lane, action.on);
            break;
        case Type::bitShift:
            seq.bitShift(action.lane);
            break;
        case Type::bitShiftReset:
            chainedSeq2 ? seq.syncChainPlayHeads() : seq.undoBitShift(action.lane);
            break;
        case Type::bitShiftAdvance:
            chainedSeq2 ? seq.bitShift(1) : seq.shiftWriteHead(action.lane);
            break;
        case Type::writeHeadReset:
            seq.resetWriteHead(action.lane);
            break;
        case Type::lengthPress:
            seq.pressLength(action.lane);
            break;
        case Type::lengthReset:
            seq.resetLength(action.lane);
            break;
        case Type::reset:
            seq.reset();
            break;
        case Type::advance:
        {
            const auto triggers = seq.advance();
            pendingTrigger = pendingTrigger || triggers[0] || triggers[1];
            break;
        }
        case Type::bufferSave:
            seq.saveBuffer();
            break;
        case Type::bufferRecall:
        case Type::clear:
            action.type == Type::clear ? seq.clear() : seq.recallBuffer();
            modesChanged = true; // chain et quantizer ont pu changer : les paramètres suivront
            break;
        case Type::trigger:
            pendingTrigger = true;
            break;
        case Type::quantizerNext:
        case Type::quantizerPrevious:
        case Type::clockDivisionUp:
        case Type::clockDivisionDown:
            break; // traités par les paramètres
        }
    };
    for (int i = 0; i < pending.blockSize1; ++i)
        apply(actionQueue[static_cast<std::size_t>(pending.startIndex1 + i)]);
    for (int i = 0; i < pending.blockSize2; ++i)
        apply(actionQueue[static_cast<std::size_t>(pending.startIndex2 + i)]);
}

void DedaleProcessor::connectCable(dedale::Out output, dedale::In input) noexcept
{
    if (input != dedale::In::midi)
        cables[static_cast<std::size_t>(input)].store(static_cast<std::int8_t>(output));
}

void DedaleProcessor::disconnectCable(dedale::In input) noexcept
{
    cables[static_cast<std::size_t>(input)].store(-1);
}

int DedaleProcessor::cableSource(dedale::In input) const noexcept
{
    return cables[static_cast<std::size_t>(input)].load();
}

void DedaleProcessor::applyCables() noexcept
{
    for (std::size_t i = 0; i < cables.size(); ++i)
    {
        const auto source = cables[i].load();
        if (source == appliedCables[i])
            continue;
        appliedCables[i] = source;
        if (source < 0)
            engine.patchBay().disconnect(static_cast<dedale::In>(i));
        else
            engine.patchBay().connect(static_cast<dedale::Out>(source), static_cast<dedale::In>(i));
    }
}

void DedaleProcessor::publishDisplay() noexcept
{
    auto& seq = engine.sequencer();
    for (std::size_t lane = 0; lane < display.bits.size(); ++lane)
    {
        const auto index = static_cast<int>(lane);
        const auto& l = seq.lane(index);
        std::uint8_t mask = 0;
        for (std::size_t step = 0; step < l.bit.size(); ++step)
            mask = static_cast<std::uint8_t>(mask | (l.bit[step] ? 1u << step : 0u));
        display.bits[lane].store(mask, std::memory_order_relaxed);
        display.playHead[lane].store(l.playHead, std::memory_order_relaxed);
        display.writeHead[lane].store(seq.writeHead(index), std::memory_order_relaxed);
        display.length[lane].store(l.length, std::memory_order_relaxed);
    }
    display.quantMode.store(seq.quantizerMode(), std::memory_order_relaxed);
    display.chained.store(seq.isChained(), std::memory_order_relaxed);
    display.running.store(seq.isRunning(), std::memory_order_relaxed);
    display.hasBuffer.store(seq.hasBuffer(), std::memory_order_relaxed);

    if (const juce::SpinLock::ScopedTryLockType lock(stateLock); lock.isLocked())
        publishedState = seq.state();
    if (modesChanged)
    {
        modesChanged = false;
        modesDirty.store(true);
    }
}

// Restauration demandée par l'interface (chargement d'un projet ou d'un preset) : appliquée ici, sans
// attendre.
void DedaleProcessor::exchangeSequencerState() noexcept
{
    const juce::SpinLock::ScopedTryLockType lock(stateLock);
    if (lock.isLocked() && hasPendingState)
    {
        engine.sequencer().setState(pendingState);
        hasPendingState = false;
        modesChanged = true;
    }
}

int DedaleProcessor::pullScope(float* destination, int maxSamples) noexcept
{
    const auto block = scopeFifo.read(juce::jmin(maxSamples, scopeFifo.getNumReady()));
    std::copy_n(scopeData.begin() + block.startIndex1, block.blockSize1, destination);
    std::copy_n(scopeData.begin() + block.startIndex2, block.blockSize2, destination + block.blockSize1);
    return block.blockSize1 + block.blockSize2;
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
        auto manualTrigger = pendingTrigger; // bouton TRIGGER, ou ADVANCE tombé sur un pas actif
        pendingTrigger = false;
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
        v.analogDrift = blockSettings.analogDrift;
        v.vcwZeroColour = blockSettings.vcwZeroColour;

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

// --- État du plugin --------------------------------------------------------------------------------------

dedale::SequencerState DedaleProcessor::sequencerState() const
{
    const juce::SpinLock::ScopedLockType lock(stateLock);
    return hasPendingState ? pendingState : publishedState;
}

void DedaleProcessor::setSequencerState(const dedale::SequencerState& state)
{
    const juce::SpinLock::ScopedLockType lock(stateLock);
    pendingState = state;
    hasPendingState = true;
}

dedale::GlobalSettings DedaleProcessor::settings() const noexcept
{
    return dedale::unpack(settingsBits.load());
}

void DedaleProcessor::setSettings(const dedale::GlobalSettings& newSettings)
{
    const auto oversamplingChanged = newSettings.oversampling != settings().oversampling;
    settingsBits.store(dedale::pack(newSettings));
    if (oversamplingChanged && oversampling != nullptr)
    {
        // Changement de facteur : le traitement est suspendu le temps de reconstruire le suréchantillonneur.
        suspendProcessing(true);
        {
            const juce::ScopedLock lock(getCallbackLock());
            rebuildOversampling();
        }
        suspendProcessing(false);
    }
}

juce::ValueTree DedaleProcessor::captureState(bool withSequence)
{
    namespace io = dedale::stateio;
    auto tree = apvts.copyState(); // recopie d'abord les valeurs courantes des paramètres dans l'arbre
    for (const auto& name : {io::kSequencer, io::kCables, io::kSettings})
        tree.removeChild(tree.getChildWithName(name), nullptr);
    tree.setProperty(io::kVersion, io::kStateVersion, nullptr);

    io::Cables cableTable;
    for (std::size_t i = 0; i < cableTable.size(); ++i)
        cableTable[i] = cables[i].load();
    tree.appendChild(io::toTree(cableTable), nullptr);
    tree.appendChild(io::toTree(settings()), nullptr);
    if (withSequence)
        tree.appendChild(io::toTree(sequencerState()), nullptr);
    return tree;
}

void DedaleProcessor::applyState(const juce::ValueTree& state, bool withSequence)
{
    namespace io = dedale::stateio;
    if (!state.hasType(apvts.state.getType()))
        return;

    const auto width =
        apvts.state.getProperty("ui_width"); // la taille de la fenêtre n'appartient pas au preset
    auto parameters = state.createCopy();
    for (const auto& name : {io::kSequencer, io::kCables, io::kSettings})
        parameters.removeChild(parameters.getChildWithName(name), nullptr);
    if (!withSequence && !width.isVoid())
        parameters.setProperty("ui_width", width, nullptr);
    apvts.replaceState(parameters);

    // Câbles et réglages : absents d'un ancien projet, ils gardent alors leur valeur courante.
    if (const auto cableTree = state.getChildWithName(io::kCables); cableTree.isValid())
    {
        const auto table = io::cablesFromTree(cableTree);
        for (std::size_t i = 0; i < table.size(); ++i)
            cables[i].store(static_cast<std::int8_t>(table[i]));
    }
    if (const auto settingsTree = state.getChildWithName(io::kSettings);
        settingsTree.isValid() && withSequence)
        setSettings(io::settingsFromTree(settingsTree, settings()));
    if (const auto sequencerTree = state.getChildWithName(io::kSequencer);
        sequencerTree.isValid() && withSequence)
        setSequencerState(io::sequencerFromTree(sequencerTree, sequencerState()));
}

void DedaleProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (const auto xml = captureState(true).createXml())
        copyXmlToBinary(*xml, destData);
}

void DedaleProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary(data, sizeInBytes))
        applyState(juce::ValueTree::fromXml(*xml), true);
}

void DedaleProcessor::syncParametersFromSequencer()
{
    if (!modesDirty.exchange(false))
        return;
    const auto set = [this](const char* id, float value)
    {
        auto* parameter = apvts.getParameter(id);
        const auto normalised = parameter->convertTo0to1(value);
        if (std::abs(parameter->getValue() - normalised) > 1.0e-6f)
            parameter->setValueNotifyingHost(normalised);
    };
    set("quant_mode", static_cast<float>(display.quantMode.load() + 1));
    set("chain", display.chained.load() ? 1.0f : 0.0f);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DedaleProcessor();
}
