// DedaleRender : rend les scénarios d'écoute hors temps réel, en WAV 48 kHz / 24 bits (§15.6).
// Usage : DedaleRender [dossier de sortie = renders] [seed = 7]
//         DedaleRender --golden <fichier.json>   réécrit les métriques de référence des golden tests
//         DedaleRender --ui <dossier>            capture le panneau en PNG (vérification visuelle sans DAW)

#include "../Source/PluginEditor.h"
#include "Scenarios.h"

#include <iostream>
#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
bool writeWav(const std::vector<float>& mono, const juce::File& file)
{
    juce::AudioBuffer<float> buffer(2, static_cast<int>(mono.size()));
    for (int channel = 0; channel < 2; ++channel)
        buffer.copyFrom(channel, 0, mono.data(), static_cast<int>(mono.size()));

    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream = std::make_unique<juce::FileOutputStream>(file);
    const auto options = juce::AudioFormatWriterOptions{}
                             .withSampleRate(render::kSampleRate)
                             .withNumChannels(2)
                             .withBitsPerSample(24);
    const auto writer = juce::WavAudioFormat{}.createWriterFor(stream, options);
    return writer != nullptr && writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

int writeGolden(const juce::File& file)
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    for (const auto& scenario : render::scenarios())
    {
        const auto m =
            render::measure(render::renderScenario(scenario, render::kDefaultSeed, render::kGoldenSeconds));
        juce::DynamicObject::Ptr entry = new juce::DynamicObject();
        entry->setProperty("peak_db", m.peakDb);
        entry->setProperty("rms_db", m.rmsDb);
        entry->setProperty("dc", m.dc);
        entry->setProperty("crossings_per_second", m.crossingsPerSecond);
        root->setProperty(scenario.name, entry.get());
        std::cout << scenario.name << " : crête " << m.peakDb << " dB, RMS " << m.rmsDb << " dB\n";
    }
    return file.replaceWithText(juce::JSON::toString(juce::var(root.get())) + "\n") ? 0 : 1;
}
// Capture du panneau : séquenceur en marche, quelques câbles posés, à 200 %.
int writeUi(const juce::File& directory)
{
    DedaleProcessor processor;
    processor.seedSequencer(render::kDefaultSeed);
    render::internalClock(processor);
    render::set(processor, "vcw_fold", 0.55f);
    render::set(processor, "mix_noise", 0.2f);
    processor.sequencer().shiftWriteHead(0);
    processor.connectCable(dedale::Out::mvco, dedale::In::bitFlip1);
    processor.connectCable(dedale::Out::eg1, dedale::In::vcwVcaCv);
    processor.connectCable(dedale::Out::noise, dedale::In::cutoff);
    processor.connectCable(dedale::Out::seq2Cv, dedale::In::blend);

    juce::AudioBuffer<float> block(2, render::kBlockSize);
    juce::MidiBuffer midi;
    processor.setPlayConfigDetails(0, 2, render::kSampleRate, render::kBlockSize);
    processor.prepareToPlay(render::kSampleRate, render::kBlockSize);
    for (int i = 0; i < 70; ++i)
        processor.processBlock(block, midi);

    const std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    editor->setSize(dedale::ui::kDesignWidth * 2, dedale::ui::kDesignHeight * 2);
    // L'oscilloscope et les LEDs suivent le son comme dans une DAW ; on s'arrête sur une note qui sonne.
    for (int i = 0; i < 400; ++i)
    {
        processor.processBlock(block, midi);
        static_cast<DedaleEditor&>(*editor).refreshNow();
        if (i >= 40 && block.getMagnitude(0, block.getNumSamples()) > 0.3f)
            break;
    }
    const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);

    if (!directory.createDirectory())
        return 1;
    const auto file = directory.getChildFile("panneau.png");
    file.deleteFile();
    juce::FileOutputStream stream(file);
    return stream.openedOk() && juce::PNGImageFormat().writeImageToStream(image, stream) ? 0 : 1;
}
} // namespace

int main(int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;
    const auto workingDirectory = juce::File::getCurrentWorkingDirectory();

    if (argc > 2 && juce::String(argv[1]) == "--golden")
        return writeGolden(workingDirectory.getChildFile(argv[2]));
    if (argc > 2 && juce::String(argv[1]) == "--ui")
        return writeUi(workingDirectory.getChildFile(argv[2]));

    const auto directory = workingDirectory.getChildFile(argc > 1 ? argv[1] : "renders");
    const auto seed = argc > 2 ? static_cast<std::uint32_t>(juce::String(argv[2]).getLargeIntValue())
                               : render::kDefaultSeed;
    if (!directory.createDirectory())
    {
        std::cerr << "Impossible de créer " << directory.getFullPathName() << "\n";
        return 1;
    }

    std::cout << "Seed " << seed << ", sortie : " << directory.getFullPathName() << "\n";
    for (const auto& scenario : render::scenarios())
    {
        std::cout << scenario.name << " (" << scenario.seconds << " s)\n";
        if (!writeWav(render::renderScenario(scenario, seed),
                      directory.getChildFile(juce::String(scenario.name) + ".wav")))
        {
            std::cerr << "Échec de l'écriture de " << scenario.name << ".wav\n";
            return 1;
        }
    }
    return 0;
}
