#include "GenerativeSequencer.h"

#include "../Calibration.h"
#include "Quantizer.h"

#include <algorithm>

namespace dedale
{
namespace cal = calibration;

GenerativeSequencer::GenerativeSequencer(std::uint32_t seedValue) noexcept
    : rng(seedValue), quantMode(cal::kDefaultQuantizerMode)
{
}

// --- Géométrie -------------------------------------------------------------------------------------------

int GenerativeSequencer::span(int lane) const noexcept
{
    return chained ? lanes[0].length + lanes[1].length : lanes[laneIndex(lane)].length;
}

int GenerativeSequencer::writeHead(int lane) const noexcept
{
    const auto& l = lanes[laneIndex(lane)];
    return (l.playHead + l.writeOffset) % span(lane);
}

// En mode chain, les positions 0..15 parcourent les pas de SEQ1 puis ceux de SEQ2.
GenerativeSequencer::StepRef GenerativeSequencer::resolve(int lane, int position) const noexcept
{
    if (!chained)
        return {lane == 0 ? 0 : 1, position};
    return position < lanes[0].length ? StepRef{0, position} : StepRef{1, position - lanes[0].length};
}

void GenerativeSequencer::wrapHeads() noexcept
{
    for (int lane = 0; lane < kNumLanes; ++lane)
        lanes[laneIndex(lane)].playHead %= span(lane);
}

// --- Horloge et transport --------------------------------------------------------------------------------

bool GenerativeSequencer::tick(int lane) noexcept
{
    return running && step(lane, true);
}

// Ordre du §8.2 : avance, (BIT FLIP maintenu), CORRUPT au write head, lecture au play head.
bool GenerativeSequencer::step(int lane, bool applyCorrupt) noexcept
{
    auto& l = lanes[laneIndex(lane)];
    auto& playCurrent = playCurrentStepNext[laneIndex(lane)];
    if (playCurrent)
        playCurrent = false;
    else
        l.playHead = (l.playHead + 1) % span(lane);

    if (flipGate[laneIndex(lane)])
        flipAt(lane, writeHead(lane));

    if (applyCorrupt && cal::kCorruptTiming == cal::CorruptTiming::beforeRead)
        corruptWriteHead(lane);
    const auto trigger = readPlayHead(lane);
    if (applyCorrupt && cal::kCorruptTiming == cal::CorruptTiming::afterRead)
        corruptWriteHead(lane);
    return trigger;
}

void GenerativeSequencer::reset() noexcept
{
    for (auto& l : lanes)
        l.playHead = 0;
    playCurrentStepNext = {true, true};
}

std::array<bool, kNumLanes> GenerativeSequencer::advance() noexcept
{
    if (running)
        return {};
    playCurrentStepNext = {};
    return {step(0, false), step(1, false)};
}

bool GenerativeSequencer::readPlayHead(int lane) noexcept
{
    const auto ref = resolve(lane, lanes[laneIndex(lane)].playHead);
    const auto& source = lanes[laneIndex(ref.lane)];
    const auto index = static_cast<std::size_t>(ref.index);
    if (!source.bit[index])
        return false; // pas de trigger, la CV garde sa dernière valeur
    heldVolts[laneIndex(lane)] = source.voltage[index];
    return true;
}

// --- Mutations -------------------------------------------------------------------------------------------

void GenerativeSequencer::flipAt(int lane, int position) noexcept
{
    const auto ref = resolve(lane, position);
    auto& target = lanes[laneIndex(ref.lane)];
    const auto index = static_cast<std::size_t>(ref.index);
    target.bit[index] = !target.bit[index];
    if (target.bit[index])
        target.voltage[index] =
            rng.bipolarVolts(); // un bit qui s'allume reçoit toujours une nouvelle tension
}

void GenerativeSequencer::corruptWriteHead(int lane) noexcept
{
    const auto amount = lanes[laneIndex(lane)].corrupt;
    if (amount <= 0.0f)
        return;

    const auto upperHalf = std::max(0.0f, (amount - 0.5f) * 2.0f);
    const auto pFlip = upperHalf * cal::kCorruptFlipProbAtMax;
    const auto pVolt = amount <= 0.5f
                           ? amount * 2.0f * cal::kCorruptVoltProbAtNoon
                           : cal::kCorruptVoltProbAtNoon +
                                 upperHalf * (cal::kCorruptVoltProbAtMax - cal::kCorruptVoltProbAtNoon);

    const auto position = writeHead(lane);
    if (pFlip > 0.0f && rng.uniform() < pFlip)
    {
        flipAt(lane, position);
        return;
    }

    const auto ref = resolve(lane, position);
    auto& target = lanes[laneIndex(ref.lane)];
    const auto index = static_cast<std::size_t>(ref.index);
    if ((target.bit[index] || cal::kCorruptChangesOffBitVoltages) && rng.uniform() < pVolt)
        target.voltage[index] = rng.bipolarVolts();
}

void GenerativeSequencer::setBitFlipGate(int lane, bool high) noexcept
{
    auto& gate = flipGate[laneIndex(lane)];
    if (high && !gate)
        flipAt(lane, writeHead(lane));
    gate = high;
}

// --- Longueur et rotations -------------------------------------------------------------------------------

void GenerativeSequencer::pressLength(int lane) noexcept
{
    auto& l = lanes[laneIndex(lane)];
    l.length = l.length > 1 ? l.length - 1 : kMaxSteps;
    wrapHeads();
    forgetRotations();
}

void GenerativeSequencer::resetLength(int lane) noexcept
{
    lanes[laneIndex(lane)].length = kMaxSteps;
    forgetRotations();
}

void GenerativeSequencer::rotate(int lane, bool right) noexcept
{
    const auto n = span(lane);
    const auto at = [&](int position)
    {
        const auto ref = resolve(lane, position);
        auto& l = lanes[laneIndex(ref.lane)];
        const auto index = static_cast<std::size_t>(ref.index);
        return std::pair<bool&, float&>{l.bit[index], l.voltage[index]};
    };

    const auto first = right ? n - 1 : 0;
    const auto direction = right ? -1 : 1;
    const auto savedBit = at(first).first;
    const auto savedVolts = at(first).second;
    for (int i = 0, position = first; i < n - 1; ++i, position += direction)
    {
        at(position).first = at(position + direction).first;
        at(position).second = at(position + direction).second;
    }
    const auto last = right ? 0 : n - 1;
    at(last).first = savedBit;
    at(last).second = savedVolts;
}

void GenerativeSequencer::bitShift(int lane) noexcept
{
    if (chained && lane != 0)
    {
        lanes[1].playHead = (lanes[1].playHead + 1) % span(1);
        return;
    }
    rotate(lane, true);
    auto& count = rotations[laneIndex(lane)];
    count = (count + 1) % span(lane);
}

void GenerativeSequencer::undoBitShift(int lane) noexcept
{
    if (chained && lane != 0)
        return;
    auto& count = rotations[laneIndex(lane)];
    for (; count > 0; --count)
        rotate(lane, false);
}

void GenerativeSequencer::shiftWriteHead(int lane) noexcept
{
    auto& l = lanes[laneIndex(lane)];
    l.writeOffset = (l.writeOffset + 1) % span(lane);
}

void GenerativeSequencer::resetWriteHead(int lane) noexcept
{
    lanes[laneIndex(lane)].writeOffset = 0;
}

// --- Chain -----------------------------------------------------------------------------------------------

void GenerativeSequencer::setChain(bool shouldChain) noexcept
{
    if (chained == shouldChain)
        return;
    chained = shouldChain;
    if (chained)
        syncChainPlayHeads();
    wrapHeads();
    forgetRotations(); // les rotations faites dans l'autre mode ne sont plus annulables
}

void GenerativeSequencer::syncChainPlayHeads() noexcept
{
    if (!chained)
        return;
    lanes[1].playHead = lanes[0].playHead;
    playCurrentStepNext[1] = playCurrentStepNext[0];
}

// --- Quantizer, buffer, réglages -------------------------------------------------------------------------

void GenerativeSequencer::setQuantizerMode(int mode) noexcept
{
    quantMode = std::clamp(mode, 0, kNumQuantizerModes - 1);
}

void GenerativeSequencer::saveBuffer() noexcept
{
    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        buffer.bit[i] = lanes[i].bit;
        buffer.voltage[i] = lanes[i].voltage;
        buffer.length[i] = lanes[i].length;
        buffer.writeOffset[i] = lanes[i].writeOffset;
    }
    buffer.quantMode = quantMode;
    buffer.chained = chained;
    buffer.valid = true;
}

void GenerativeSequencer::recallBuffer() noexcept
{
    if (!buffer.valid)
        return;
    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        lanes[i].bit = buffer.bit[i];
        lanes[i].voltage = buffer.voltage[i];
        lanes[i].length = buffer.length[i];
        lanes[i].writeOffset = buffer.writeOffset[i];
    }
    quantMode = buffer.quantMode;
    chained = buffer.chained;
    wrapHeads();
    forgetRotations();
}

void GenerativeSequencer::clear() noexcept
{
    for (auto& l : lanes)
    {
        l.bit = {};
        l.writeOffset = 0;
    }
    quantMode = cal::kQuantizerModeAfterClear;
    forgetRotations();
}

void GenerativeSequencer::setCorrupt(int lane, float amount) noexcept
{
    lanes[laneIndex(lane)].corrupt = std::clamp(amount, 0.0f, 1.0f);
}

void GenerativeSequencer::setCvRange(int lane, float amount) noexcept
{
    lanes[laneIndex(lane)].cvRange = std::clamp(amount, 0.0f, 1.0f);
}

void GenerativeSequencer::setStep(int lane, int index, bool on, float volts) noexcept
{
    auto& l = lanes[laneIndex(lane)];
    const auto i = static_cast<std::size_t>(std::clamp(index, 0, kMaxSteps - 1));
    l.bit[i] = on;
    l.voltage[i] = std::clamp(volts, -5.0f, 5.0f);
}

SequencerState GenerativeSequencer::state() const noexcept
{
    return {lanes,   heldVolts, playCurrentStepNext, rotations, buffer, quantMode, rootSemitones,
            chained, unipolar};
}

void GenerativeSequencer::setState(const SequencerState& s) noexcept
{
    const auto volts = [](float v) { return v == v ? std::clamp(v, -5.0f, 5.0f) : 0.0f; }; // NaN → 0 V
    const auto unit = [](float v) { return v == v ? std::clamp(v, 0.0f, 1.0f) : 0.0f; };

    chained = s.chained;
    unipolar = s.unipolar;
    quantMode = std::clamp(s.quantMode, 0, kNumQuantizerModes - 1);
    rootSemitones = std::clamp(s.rootSemitones, -127, 127);
    lanes = s.lanes;
    buffer = s.buffer;
    playCurrentStepNext = s.playCurrentStepNext;

    for (std::size_t i = 0; i < lanes.size(); ++i)
    {
        auto& l = lanes[i];
        l.length = std::clamp(l.length, 1, kMaxSteps);
        l.corrupt = unit(l.corrupt);
        l.cvRange = unit(l.cvRange);
        for (auto& v : l.voltage)
            v = volts(v);
        heldVolts[i] = volts(s.heldVolts[i]);

        buffer.length[i] = std::clamp(buffer.length[i], 1, kMaxSteps);
        buffer.writeOffset[i] = std::clamp(buffer.writeOffset[i], 0, 2 * kMaxSteps - 1);
        for (auto& v : buffer.voltage[i])
            v = volts(v);
    }
    buffer.quantMode = std::clamp(buffer.quantMode, 0, kNumQuantizerModes - 1);

    // Têtes et compteurs : dans la longueur parcourue, qui dépend des longueurs et du mode chain.
    for (int lane = 0; lane < kNumLanes; ++lane)
    {
        auto& l = lanes[laneIndex(lane)];
        const auto n = span(lane);
        l.playHead = std::clamp(l.playHead, 0, n - 1);
        l.writeOffset = std::clamp(l.writeOffset, 0, n - 1);
        rotations[laneIndex(lane)] = std::clamp(s.rotations[laneIndex(lane)], 0, n - 1);
    }
}

float GenerativeSequencer::cv(int lane) const noexcept
{
    const auto range = lanes[laneIndex(lane)].cvRange;
    auto volts =
        heldVolts[laneIndex(lane)] * (cal::kCvRangeMinScale + range * (1.0f - cal::kCvRangeMinScale));
    if (unipolar)
        volts = volts * 0.5f + 2.5f; // avant le quantizer, pour rester dans la gamme (ADR-012)
    return quantize(volts, quantMode, rootSemitones);
}
} // namespace dedale
