// DedaleRender : rend les scénarios d'écoute hors temps réel, en WAV 48 kHz / 24 bits (§15.6).
// Usage : DedaleRender [dossier de sortie = renders] [seed = 7]

#include "../Source/PluginProcessor.h"

#include <functional>
#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

// Transport hôte simulé : lecture continue à tempo fixe depuis la position 0.
class FixedTempoPlayHead final : public juce::AudioPlayHead
{
public:
    explicit FixedTempoPlayHead(double bpmToUse) : bpm(bpmToUse) {}

    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setIsPlaying(true);
        info.setBpm(bpm);
        info.setPpqPosition(static_cast<double>(samples) / kSampleRate * bpm / 60.0);
        info.setTimeInSamples(samples);
        return info;
    }

    void advance(int numSamples) { samples += numSamples; }

private:
    double bpm;
    juce::int64 samples = 0;
};

struct Scenario
{
    const char* name;
    double seconds;
    double hostBpm; // 0 = horloge interne
    std::function<void(DedaleProcessor&)> setup;
    std::function<void(DedaleProcessor&, double)> atBlockStart = {}; // reçoit le temps en secondes
};

void set(DedaleProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.apvts.getParameter(id);
    jassert(parameter != nullptr);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void muteLane(DedaleProcessor& processor, int lane)
{
    for (int step = 0; step < dedale::kMaxSteps; ++step)
        processor.sequencer().setStep(lane, step, false, 0.0f);
}

// 4 pas par seconde partout : croches à 120 BPM (hôte, division 2) ou 240 pas/min (interne).
void hostEighths(DedaleProcessor& processor)
{
    set(processor, "sync_mode", 1.0f);
    set(processor, "clock_div", 2.0f);
}

void internalClock(DedaleProcessor& processor)
{
    set(processor, "sync_mode", 0.0f);
    set(processor, "tempo", 240.0f);
}

std::vector<Scenario> scenarios()
{
    const auto corrupt = [](float amount)
    {
        return [amount](DedaleProcessor& p)
        {
            hostEighths(p);
            muteLane(p, 1);
            set(p, "seq1_corrupt", amount);
        };
    };

    return {
        {"seq_corrupt_0", 16.0, 120.0, corrupt(0.0f)},
        {"seq_corrupt_40", 24.0, 120.0, corrupt(0.4f)},
        {"seq_corrupt_75", 24.0, 120.0, corrupt(0.75f)},
        {"seq_polymeter_8_5", 20.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             for (int i = 0; i < 3; ++i)
                 p.sequencer().pressLength(1); // SEQ2 sur 5 pas
         }},
        {"seq_chain_16", 16.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             set(p, "chain", 1.0f);
         }},
        {"seq_quant_modes", 30.0, 0.0,
         [](DedaleProcessor& p)
         {
             internalClock(p);
             muteLane(p, 1);
             set(p, "seq1_range", 0.5f);
         },
         [](DedaleProcessor& p, double time)
         {
             // Toutes les 6 s (3 boucles de 8 pas) : non quantifié, chromatique, majeur, pentatonique, quads.
             constexpr std::array<float, 5> modes{1.0f, 2.0f, 3.0f, 4.0f, 16.0f};
             set(p, "quant_mode", modes[std::min<std::size_t>(static_cast<std::size_t>(time / 6.0), 4)]);
         }},
    };
}

void printPattern(DedaleProcessor& processor)
{
    for (int lane = 0; lane < dedale::kNumLanes; ++lane)
    {
        const auto& l = processor.sequencer().lane(lane);
        std::cout << "    SEQ" << lane + 1 << " (" << l.length << " pas) : ";
        for (std::size_t step = 0; step < l.bit.size(); ++step)
            std::cout << (l.bit[step] ? "x" : ".");
        std::cout << "\n";
    }
}

bool render(const Scenario& scenario, const juce::File& directory, std::uint32_t seed)
{
    DedaleProcessor processor;
    processor.seedSequencer(seed);
    set(processor, "run", 1.0f);
    scenario.setup(processor);

    FixedTempoPlayHead playHead(scenario.hostBpm);
    if (scenario.hostBpm > 0.0)
        processor.setPlayHead(&playHead);

    std::cout << scenario.name << " (" << scenario.seconds << " s)\n";
    printPattern(processor);

    const auto totalSamples = static_cast<int>(scenario.seconds * kSampleRate);
    juce::AudioBuffer<float> output(2, totalSamples);
    juce::AudioBuffer<float> block(2, kBlockSize);
    juce::MidiBuffer midi;

    processor.setPlayConfigDetails(0, 2, kSampleRate, kBlockSize);
    processor.prepareToPlay(kSampleRate, kBlockSize);
    for (int start = 0; start < totalSamples; start += kBlockSize)
    {
        if (scenario.atBlockStart)
            scenario.atBlockStart(processor, start / kSampleRate);
        const auto length = std::min(kBlockSize, totalSamples - start);
        block.setSize(2, length, false, false, true);
        processor.processBlock(block, midi);
        playHead.advance(length);
        for (int channel = 0; channel < 2; ++channel)
            output.copyFrom(channel, start, block, channel, 0, length);
    }
    processor.releaseResources();

    const auto file = directory.getChildFile(juce::String(scenario.name) + ".wav");
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream>(file);
    const auto options =
        juce::AudioFormatWriterOptions{}.withSampleRate(kSampleRate).withNumChannels(2).withBitsPerSample(24);
    const auto writer = juce::WavAudioFormat{}.createWriterFor(stream, options);
    return writer != nullptr && writer->writeFromAudioSampleBuffer(output, 0, totalSamples);
}
} // namespace

int main(int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const auto directory =
        juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "renders");
    const auto seed = static_cast<std::uint32_t>(argc > 2 ? juce::String(argv[2]).getLargeIntValue() : 7);
    if (!directory.createDirectory())
    {
        std::cerr << "Impossible de créer " << directory.getFullPathName() << "\n";
        return 1;
    }

    std::cout << "Seed " << seed << ", sortie : " << directory.getFullPathName() << "\n";
    for (const auto& scenario : scenarios())
    {
        if (!render(scenario, directory, seed))
        {
            std::cerr << "Échec de l'écriture de " << scenario.name << ".wav\n";
            return 1;
        }
    }
    return 0;
}
