// Tests du processor complet (avec JUCE) : sauvegarde et restauration du projet, réglages globaux.

#include "../Tools/Scenarios.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
// Fait tourner le processor hors temps réel et renvoie le canal gauche.
std::vector<float> process(DedaleProcessor& processor, double seconds)
{
    const auto total = static_cast<int>(seconds * render::kSampleRate);
    std::vector<float> output(static_cast<std::size_t>(total));
    juce::AudioBuffer<float> block(2, render::kBlockSize);
    juce::MidiBuffer midi;
    processor.setPlayConfigDetails(0, 2, render::kSampleRate, render::kBlockSize);
    processor.prepareToPlay(render::kSampleRate, render::kBlockSize);
    for (int start = 0; start < total; start += render::kBlockSize)
    {
        const auto length = std::min(render::kBlockSize, total - start);
        block.setSize(2, length, false, false, true);
        processor.processBlock(block, midi);
        std::copy_n(block.getReadPointer(0), length, output.begin() + start);
    }
    return output;
}

// Un patch qui touche à tout : paramètres, séquence, buffer, câbles, réglages globaux.
void configure(DedaleProcessor& p)
{
    p.seedSequencer(11);
    render::internalClock(p);
    render::set(p, "vcw_fold", 0.62f);
    render::set(p, "vcf_cutoff", 1234.0f);
    render::set(p, "order", 2.0f);
    render::set(p, "quant_mode", 5.0f);
    p.sequencer().pressLength(1);
    p.sequencer().shiftWriteHead(0);
    p.sequencer().setQuantizerMode(4);
    p.sequencer().saveBuffer();
    p.sequencer().bitShift(0);
    p.seedSequencer(11); // remet le motif, garde le buffer : il diffère maintenant de la séquence
    p.sequencer().pressLength(0);
    p.connectCable(dedale::Out::mvco, dedale::In::bitFlip2);
    p.connectCable(dedale::Out::eg1, dedale::In::vcwVcaCv);

    auto settings = p.settings();
    settings.unipolarCv = true;
    settings.analogDrift = false;
    settings.resetRecallsBuffer = true;
    settings.oversampling = 2;
    settings.midiChannel = 7;
    p.setSettings(settings);
}

juce::MemoryBlock save(DedaleProcessor& p)
{
    juce::MemoryBlock data;
    p.getStateInformation(data);
    return data;
}
} // namespace

TEST_CASE("Projet : la sauvegarde retrouve séquence, buffer, câbles, réglages et paramètres", "[state]")
{
    DedaleProcessor original;
    configure(original);
    // seedSequencer et sequencer() agissent hors temps réel : on publie l'état comme le ferait un bloc audio.
    original.setSequencerState(original.sequencer().state());
    const auto saved = save(original);

    DedaleProcessor restored;
    restored.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));

    SECTION("même état, relu tout de suite ou après avoir joué")
    {
        REQUIRE(restored.sequencerState() == original.sequencerState());
        REQUIRE(restored.settings() == original.settings());
        REQUIRE(restored.cableSource(dedale::In::bitFlip2) == static_cast<int>(dedale::Out::mvco));
        REQUIRE(restored.cableSource(dedale::In::vcwVcaCv) == static_cast<int>(dedale::Out::eg1));
        REQUIRE(restored.cableSource(dedale::In::cutoff) == -1);
        for (const auto* id : {"vcw_fold", "vcf_cutoff", "order", "tempo", "run"})
            REQUIRE(restored.apvts.getRawParameterValue(id)->load() ==
                    original.apvts.getRawParameterValue(id)->load());

        // Sauver ce qu'on vient de charger redonne les mêmes données.
        REQUIRE(save(restored) == saved);
    }

    SECTION("deux instances restaurées jouent exactement la même chose")
    {
        DedaleProcessor twin;
        twin.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
        const auto a = process(restored, 3.0);
        const auto b = process(twin, 3.0);
        REQUIRE(render::measure(a).rmsDb > -50.0); // ça joue vraiment
        REQUIRE(a == b);
        // Les deux séquences ont vécu la même chose (M VCO → BIT FLIP 2) ; seules les tensions tirées au
        // hasard diffèrent, la seed du générateur ne faisant pas partie du projet.
        REQUIRE(restored.sequencerState().lanes[1].bit == twin.sequencerState().lanes[1].bit);
        REQUIRE(restored.sequencerState().buffer.valid);
        REQUIRE(restored.sequencerState().lanes[0].length == 7);
    }
}

TEST_CASE("Projet : données invalides ou anciennes", "[state]")
{
    DedaleProcessor processor;
    configure(processor);
    processor.setSequencerState(processor.sequencer().state());
    const auto before = processor.sequencerState();
    const auto settingsBefore = processor.settings();

    SECTION("octets quelconques : rien ne change, pas de crash")
    {
        const std::array<char, 64> garbage{"ceci n'est pas un etat de plugin \x01\x02\x03\xff"};
        processor.setStateInformation(garbage.data(), static_cast<int>(garbage.size()));
        processor.setStateInformation(nullptr, 0);
        REQUIRE(processor.sequencerState() == before);
        REQUIRE(processor.settings() == settingsBefore);
        REQUIRE(processor.cableSource(dedale::In::bitFlip2) == static_cast<int>(dedale::Out::mvco));
    }

    SECTION("projet d'avant la phase 6 (paramètres seuls) : séquence, câbles et réglages gardés")
    {
        auto old = processor.captureState(true);
        for (const auto& name :
             {dedale::stateio::kSequencer, dedale::stateio::kCables, dedale::stateio::kSettings})
            old.removeChild(old.getChildWithName(name), nullptr);
        old.removeProperty(dedale::stateio::kVersion, nullptr);
        render::set(processor, "vcw_fold", 0.1f);
        processor.applyState(old, true);
        REQUIRE(processor.apvts.getRawParameterValue("vcw_fold")->load() == 0.62f);
        REQUIRE(processor.sequencerState() == before);
        REQUIRE(processor.settings() == settingsBefore);
        REQUIRE(processor.cableSource(dedale::In::bitFlip2) == static_cast<int>(dedale::Out::mvco));
    }

    SECTION("valeurs hors bornes dans le fichier : ramenées dans les bornes, et ça joue")
    {
        auto broken = processor.captureState(true);
        broken.getChildWithName(dedale::stateio::kCables)
            .setProperty("sources", "99 -5 3 abc -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 4 -1 -1 7 7 7",
                         nullptr);
        auto sequencer = broken.getChildWithName(dedale::stateio::kSequencer);
        sequencer.setProperty("quant", 4000, nullptr);
        sequencer.getChild(0).setProperty("length", -12, nullptr);
        sequencer.getChild(0).setProperty("volts", "1e30 nan -1e30", nullptr);
        broken.getChildWithName(dedale::stateio::kSettings).setProperty("oversampling", 4096, nullptr);
        processor.applyState(broken, true);
        REQUIRE(processor.cableSource(dedale::In::vcoPitch) == -1); // 99 : pas une sortie
        REQUIRE(processor.cableSource(dedale::In::mvcoPitch) == 3);
        REQUIRE(processor.cableSource(dedale::In::midi) == -1); // le jack MIDI n'est pas câblable
        REQUIRE(processor.settings().oversampling == 8);
        for (const auto v : process(processor, 1.0))
            REQUIRE(std::isfinite(v));
        const auto after = processor.sequencerState();
        REQUIRE(after.quantMode == 15);
        REQUIRE(after.lanes[0].length == 1);
        REQUIRE(after.lanes[0].voltage[0] == 5.0f);
    }
}

TEST_CASE("Réglages globaux : suréchantillonnage ×2, ×4, ×8", "[settings]")
{
    for (const auto factor : {2, 4, 8})
    {
        DedaleProcessor processor;
        processor.seedSequencer(render::kDefaultSeed);
        render::internalClock(processor);
        auto settings = processor.settings();
        settings.oversampling = factor;
        processor.setSettings(settings);

        const auto first = render::measure(process(processor, 1.5));
        REQUIRE(first.finite);
        REQUIRE(first.rmsDb > -40.0);
        REQUIRE(first.peakDb <= 0.0);

        // Changement en cours de route : le plugin continue de jouer.
        settings.oversampling = factor == 8 ? 2 : factor * 2;
        processor.setSettings(settings);
        juce::AudioBuffer<float> block(2, render::kBlockSize);
        juce::MidiBuffer midi;
        float peak = 0.0f;
        for (int i = 0; i < 150; ++i)
        {
            processor.processBlock(block, midi);
            peak = std::max(peak, block.getMagnitude(0, block.getNumSamples()));
        }
        REQUIRE(std::isfinite(peak));
        REQUIRE(peak > 0.01f);
        REQUIRE(peak <= 1.0f);
    }
}

TEST_CASE("Rappel du buffer : les paramètres chain et quantizer suivent", "[state]")
{
    DedaleProcessor processor;
    processor.seedSequencer(3);
    render::set(processor, "quant_mode", 4.0f);
    render::set(processor, "chain", 1.0f);
    process(processor, 0.1);
    processor.perform({dedale::ActionType::bufferSave});
    process(processor, 0.1);

    render::set(processor, "quant_mode", 9.0f);
    render::set(processor, "chain", 0.0f);
    process(processor, 0.1);
    REQUIRE(processor.sequencerState().quantMode == 8);
    REQUIRE_FALSE(processor.sequencerState().chained);

    processor.perform({dedale::ActionType::bufferRecall});
    process(processor, 0.1);
    process(processor, 0.1); // le rappel tient : les paramètres, inchangés, ne l'écrasent pas
    REQUIRE(processor.sequencerState().quantMode == 3);
    REQUIRE(processor.sequencerState().chained);

    processor.syncParametersFromSequencer(); // ce que fait la minuterie du plugin
    REQUIRE(processor.apvts.getRawParameterValue("quant_mode")->load() == 4.0f);
    REQUIRE(processor.apvts.getRawParameterValue("chain")->load() == 1.0f);
    process(processor, 0.1);
    REQUIRE(processor.sequencerState().quantMode == 3);
}

TEST_CASE("Presets : usine et utilisateur", "[presets]")
{
    const auto directory =
        juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("dedale_presets_" + juce::String(juce::Random::getSystemRandom().nextInt(1000000)));
    DedaleProcessor processor;
    PresetManager presets(processor, directory);
    processor.seedSequencer(5);
    const auto sequence = processor.sequencerState();
    const auto value = [&](const char* id) { return processor.apvts.getRawParameterValue(id)->load(); };

    REQUIRE(presets.numFactoryPresets() >= 10);
    REQUIRE(presets.currentName() == "Init");

    SECTION("un preset d'usine pose ses réglages et ses câbles, sans toucher à la séquence ni au transport")
    {
        render::internalClock(processor); // run, tempo 240, synchro interne
        render::set(processor, "vcw_bias", -0.8f);
        processor.connectCable(dedale::Out::eg2, dedale::In::blend);

        for (int i = 0; i < presets.numFactoryPresets(); ++i)
            if (presets.factoryName(i) == juce::String::fromUTF8("Polymétrie CLOCK 2"))
                presets.loadFactory(i);
        REQUIRE(presets.currentName() == juce::String::fromUTF8("Polymétrie CLOCK 2"));
        REQUIRE(value("mvco_freq") == Catch::Approx(3.0f));
        REQUIRE(value("eg_trig_mix") == Catch::Approx(0.5f));
        REQUIRE(value("vcw_bias") == Catch::Approx(0.0f)); // revenu à sa valeur par défaut
        REQUIRE(processor.cableSource(dedale::In::clock2) == static_cast<int>(dedale::Out::mvco));
        REQUIRE(processor.cableSource(dedale::In::blend) == -1); // l'ancien câble est retiré
        REQUIRE(value("run") == 1.0f);
        REQUIRE(value("tempo") == Catch::Approx(240.0f));
        REQUIRE(value("sync_mode") == 0.0f);
        REQUIRE(processor.sequencerState() == sequence);
    }

    SECTION("chaque preset d'usine se charge et joue sans valeur aberrante")
    {
        render::internalClock(processor);
        for (int i = 0; i < presets.numFactoryPresets(); ++i)
        {
            presets.loadFactory(i);
            const auto m = render::measure(process(processor, 2.0));
            INFO(presets.factoryName(i));
            REQUIRE(m.finite);
            REQUIRE(m.peakDb <= 0.0);
            REQUIRE(m.rmsDb > -55.0); // aucun preset muet
        }
    }

    SECTION("enregistrer, relire, parcourir, supprimer un preset utilisateur")
    {
        REQUIRE(presets.userNames().isEmpty());
        render::set(processor, "vcf_cutoff", 777.0f);
        render::set(processor, "tempo", 300.0f);
        processor.connectCable(dedale::Out::noise, dedale::In::vcwFold);
        REQUIRE(presets.saveUser("Mon patch / test"));
        REQUIRE(presets.userNames().size() == 1);
        const auto name = presets.userNames()[0]; // nom rendu légal pour un fichier
        REQUIRE(presets.currentName() == name);

        presets.loadFactory(0);
        render::set(processor, "tempo", 90.0f);
        REQUIRE(value("vcf_cutoff") == Catch::Approx(800.0f));
        REQUIRE(processor.cableSource(dedale::In::vcwFold) == -1);

        REQUIRE(presets.loadUser(name));
        REQUIRE(value("vcf_cutoff") == Catch::Approx(777.0f));
        REQUIRE(processor.cableSource(dedale::In::vcwFold) == static_cast<int>(dedale::Out::noise));
        REQUIRE(value("tempo") == Catch::Approx(90.0f)); // le tempo du moment est gardé
        REQUIRE(processor.sequencerState() == sequence);

        // Le parcours passe par les presets d'usine puis par ceux de l'utilisateur, et boucle.
        presets.step(1);
        REQUIRE(presets.currentName() == "Init");
        presets.step(-1);
        REQUIRE(presets.currentName() == name);

        REQUIRE(presets.deleteUser(name));
        REQUIRE(presets.userNames().isEmpty());
        REQUIRE_FALSE(presets.loadUser(name));
        REQUIRE_FALSE(presets.saveUser("   "));
    }

    SECTION("un fichier illisible est refusé sans rien changer")
    {
        directory.createDirectory();
        directory.getChildFile("casse.xml").replaceWithText("<pas un preset");
        directory.getChildFile("autre.xml").replaceWithText("<AUTRE_PLUGIN/>");
        render::set(processor, "vcf_cutoff", 555.0f);
        REQUIRE_FALSE(presets.loadUser("casse"));
        REQUIRE_FALSE(presets.loadUser("autre"));
        REQUIRE(value("vcf_cutoff") == Catch::Approx(555.0f));
    }

    directory.deleteRecursively();
}
