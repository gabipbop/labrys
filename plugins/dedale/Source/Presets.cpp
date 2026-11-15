#include "Presets.h"

#include "PluginProcessor.h"

namespace
{
using dedale::In;
using dedale::Out;

struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values; // écarts par rapport aux valeurs par défaut
    std::vector<std::pair<Out, In>> cables;
};

// Presets d'usine : des patchs, pas des séquences. Les valeurs sont dans l'unité du paramètre (Hz, s, 0..1).
const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets{
        {"Init", {}, {}},
        {"Sub dub",
         {{"vco_freq", 55.0f},
          {"mix_vco", 0.6f},
          {"vcw_fold", 0.1f},
          {"vcf_cutoff", 300.0f},
          {"vcf_res", 0.35f},
          {"blend", 0.8f},
          {"eg1_decay", 0.25f},
          {"eg2_decay", 0.9f},
          {"vcf_eg1_amt", 0.35f},
          {"seq1_range", 0.15f}},
         {}},
        {"Rimshot indus",
         {{"vco_freq", 320.0f},
          {"mix_noise", 0.6f},
          {"noise_tone", 0.7f},
          {"vco_eg1_amt", 0.5f},
          {"vcw_fold", 0.55f},
          {"blend", 0.3f},
          {"vcf_cutoff", 2500.0f},
          {"vcf_res", 0.5f},
          {"vcf_mode", 0.6f},
          {"eg1_decay", 0.05f},
          {"eg2_decay", 0.08f},
          {"seq1_range", 0.2f}},
         {}},
        {"Cloche TZFM",
         {{"vco_freq", 440.0f},
          {"mvco_freq", 617.0f},
          {"mvco_fm_amt", 0.55f},
          {"vcw_fold", 0.0f},
          {"vcw_eg1_amt", 0.0f},
          {"blend", 0.15f},
          {"vcf_cutoff", 4000.0f},
          {"eg1_decay", 0.6f},
          {"eg2_decay", 1.8f},
          {"quant_mode", 4.0f}},
         {}},
        {"Bruit de forge",
         {{"vco_freq", 110.0f},
          {"mvco_freq", 73.0f},
          {"mix_ring", 0.7f},
          {"mix_noise", 0.35f},
          {"vcw_fold", 0.8f},
          {"vcw_bias", 0.4f},
          {"order", 1.0f},
          {"vcf_cutoff", 900.0f},
          {"vcf_res", 0.7f},
          {"vcf_eg1_amt", 0.6f},
          {"eg1_decay", 0.12f},
          {"eg2_decay", 0.3f}},
         {}},
        {"Arpège qui pourrit",
         {{"vco_freq", 220.0f},
          {"seq1_corrupt", 0.35f},
          {"seq1_range", 0.45f},
          {"quant_mode", 4.0f},
          {"vcw_fold", 0.35f},
          {"vcf_cutoff", 1500.0f},
          {"eg2_decay", 0.25f}},
         {}},
        {"Polymétrie CLOCK 2",
         {{"mvco_freq", 3.0f}, {"eg_trig_mix", 0.5f}, {"vcf_seq2_amt", 0.5f}, {"seq2_range", 0.4f}},
         {{Out::mvco, In::clock2}}},
        {"Dédale mouvant",
         {{"mvco_freq", 0.7f}, {"vcw_fold", 0.45f}, {"vcw_seq1_amt", 0.3f}, {"eg2_decay", 0.35f}},
         {{Out::mvco, In::bitFlip1}}},
        {"Acide sec",
         {{"vco_freq", 82.0f},
          {"mix_vco", 0.8f},
          {"blend", 1.0f},
          {"vcf_cutoff", 350.0f},
          {"vcf_res", 0.85f},
          {"vcf_eg1_amt", 0.55f},
          {"vcf_seq2_amt", 0.3f},
          {"eg1_decay", 0.18f},
          {"eg2_decay", 0.3f}},
         {}},
        {"Fauve en cage",
         {{"vcf_cutoff", 1200.0f}, {"vcf_res", 0.7f}, {"vcw_fold", 0.5f}, {"mix_vco", 0.7f}},
         {{Out::vca, In::vcfIn}, {Out::noise, In::cutoff}}},
        {"Cathédrale de tôle",
         {{"vco_freq", 147.0f},
          {"mvco_freq", 220.0f},
          {"mvco_fm_amt", 0.3f},
          {"mix_ring", 0.5f},
          {"vcw_fold", 0.6f},
          {"vcf_cutoff", 2200.0f},
          {"vcf_res", 0.45f},
          {"eg1_decay", 1.2f},
          {"eg2_decay", 4.0f},
          {"seq1_corrupt", 0.1f}},
         {{Out::eg1, In::vcwVcaCv}}},
    };
    return presets;
}

// Le transport n'appartient pas au patch : un preset ne doit ni arrêter la lecture ni changer le tempo.
const juce::StringArray kTransport{"run", "tempo", "sync_mode", "clock_div"};
const juce::Identifier kPresetName{"preset_name"};

void setParameter(DedaleProcessor& processor, const juce::String& id, float value)
{
    if (auto* parameter = processor.apvts.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
} // namespace

PresetManager::PresetManager(DedaleProcessor& processorToControl, juce::File userDirectory)
    : processor(processorToControl), directory(std::move(userDirectory))
{
}

juce::File PresetManager::defaultDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
#if JUCE_MAC
        .getChildFile("Application Support")
#endif
        .getChildFile("Raw Powa Studio")
        .getChildFile("Dedale")
        .getChildFile("Presets");
}

int PresetManager::numFactoryPresets() const noexcept
{
    return static_cast<int>(factoryPresets().size());
}

juce::String PresetManager::factoryName(int index) const
{
    return juce::String::fromUTF8(
        factoryPresets()[static_cast<std::size_t>(juce::jlimit(0, numFactoryPresets() - 1, index))].name);
}

juce::StringArray PresetManager::userNames() const
{
    juce::StringArray names;
    for (const auto& file : directory.findChildFiles(juce::File::findFiles, false, "*.xml"))
        names.add(file.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

juce::File PresetManager::fileFor(const juce::String& name) const
{
    return directory.getChildFile(juce::File::createLegalFileName(name.trim()) + ".xml");
}

juce::String PresetManager::currentName() const
{
    return processor.apvts.state.getProperty(kPresetName, factoryName(0)).toString();
}

void PresetManager::setCurrentName(const juce::String& name)
{
    processor.apvts.state.setProperty(kPresetName, name, nullptr);
}

void PresetManager::loadFactory(int index)
{
    const auto& preset =
        factoryPresets()[static_cast<std::size_t>(juce::jlimit(0, numFactoryPresets() - 1, index))];

    for (auto* parameter : processor.getParameters())
        if (auto* withId = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter);
            withId != nullptr && !kTransport.contains(withId->paramID))
            parameter->setValueNotifyingHost(parameter->getDefaultValue());
    for (const auto& [id, value] : preset.values)
        setParameter(processor, id, value);

    for (int input = 0; input < dedale::kNumInputs; ++input)
        processor.disconnectCable(static_cast<In>(input));
    for (const auto& [output, input] : preset.cables)
        processor.connectCable(output, input);
    setCurrentName(juce::String::fromUTF8(preset.name));
}

bool PresetManager::saveUser(const juce::String& name)
{
    const auto file = fileFor(name);
    if (name.trim().isEmpty() || !directory.createDirectory())
        return false;
    auto tree = processor.captureState(false);
    tree.setProperty(kPresetName, file.getFileNameWithoutExtension(), nullptr);
    tree.removeProperty("ui_width", nullptr);
    const auto xml = tree.createXml();
    if (xml == nullptr || !xml->writeTo(file))
        return false;
    setCurrentName(file.getFileNameWithoutExtension());
    return true;
}

bool PresetManager::loadUser(const juce::String& name)
{
    const auto xml = juce::parseXML(fileFor(name));
    if (xml == nullptr)
        return false;
    const auto tree = juce::ValueTree::fromXml(*xml);
    if (!tree.hasType(processor.apvts.state.getType()))
        return false;

    std::vector<std::pair<juce::String, float>> transport;
    for (const auto& id : kTransport)
        transport.emplace_back(id, processor.apvts.getRawParameterValue(id)->load());
    processor.applyState(tree, false);
    for (const auto& [id, value] : transport)
        setParameter(processor, id, value);
    setCurrentName(fileFor(name).getFileNameWithoutExtension());
    return true;
}

bool PresetManager::deleteUser(const juce::String& name)
{
    const auto file = fileFor(name);
    return file.existsAsFile() && file.deleteFile();
}

void PresetManager::step(int delta)
{
    const auto users = userNames();
    const auto total = numFactoryPresets() + users.size();
    auto current = 0;
    for (int i = 0; i < total; ++i)
        if ((i < numFactoryPresets() ? factoryName(i) : users[i - numFactoryPresets()]) == currentName())
            current = i;
    const auto next = ((current + delta) % total + total) % total;
    if (next < numFactoryPresets())
        loadFactory(next);
    else
        loadUser(users[next - numFactoryPresets()]);
}
