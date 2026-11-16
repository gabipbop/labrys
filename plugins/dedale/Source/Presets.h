#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class DedaleProcessor;

// Presets : une dizaine de presets d'usine (dans le code) et les presets de l'utilisateur (fichiers XML).
// Un preset est un patch : réglages et câbles. Il ne touche ni à la séquence en cours, ni au transport
// (RUN, TEMPO, synchro, division), ni aux réglages globaux. À utiliser depuis le thread de l'interface.
class PresetManager
{
public:
    explicit PresetManager(DedaleProcessor& processorToControl,
                           juce::File userDirectory = defaultDirectory());

    // ~/Library/Application Support/Raw Powa Studio/Dedale/Presets (ou l'équivalent du système).
    static juce::File defaultDirectory();

    int numFactoryPresets() const noexcept;
    juce::String factoryName(int index) const;
    juce::StringArray userNames() const; // triés, sans l'extension

    void loadFactory(int index);
    bool loadUser(const juce::String& name);
    bool saveUser(const juce::String& name); // écrase un preset du même nom
    bool deleteUser(const juce::String& name);
    void step(int delta); // preset précédent ou suivant, d'usine puis utilisateur

    juce::String currentName() const; // dernier preset chargé ou enregistré

private:
    juce::File fileFor(const juce::String& name) const;
    void setCurrentName(const juce::String& name);

    DedaleProcessor& processor;
    juce::File directory;
};
