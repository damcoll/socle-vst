#include "Presets.h"

#include "PresetData.h"

namespace
{
    const juce::Identifier presetTag { "SOCLE_PRESET" };
    const juce::Identifier presetProperty { "preset" };

    // every parameter of the plugin, in a stable order
    std::vector<juce::RangedAudioParameter*> parametersOf (juce::AudioProcessorValueTreeState& state)
    {
        std::vector<juce::RangedAudioParameter*> result;
        for (auto* p : state.processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
                result.push_back (ranged);
        return result;
    }
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s) : state (s)
{
    // the factory presets are binary resources named "NN_NAME.socle", in that order
    std::vector<std::pair<juce::String, int>> factory;
    for (int i = 0; i < PresetData::namedResourceListSize; ++i)
    {
        const juce::String original = PresetData::originalFilenames[i];
        if (original.endsWith (fileExtension()))
            factory.emplace_back (original, i);
    }
    std::sort (factory.begin(), factory.end());

    for (const auto& [filename, i] : factory)
    {
        int size = 0;
        const auto* data = PresetData::getNamedResource (PresetData::namedResourceList[i], size);
        const auto xml = juce::String::fromUTF8 (data, size);
        if (const auto parsed = juce::parseXML (xml); parsed != nullptr && parsed->hasTagName (presetTag))
            presets.push_back ({ parsed->getStringAttribute ("name"), true, xml, {} });
    }
    factoryCount = (int) presets.size();
    rescanUserFolder();
}

juce::File PresetManager::userFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("SOCLE").getChildFile ("Presets");
}

void PresetManager::rescanUserFolder()
{
    const auto currentPreset = currentIndex >= 0 ? presets[(size_t) currentIndex] : Preset {};
    presets.resize ((size_t) factoryCount);

    auto files = userFolder().findChildFiles (juce::File::findFiles, false, "*" + fileExtension());
    files.sort();
    for (const auto& file : files)
        presets.push_back ({ file.getFileNameWithoutExtension(), false, {}, file });

    // keep pointing at the same preset
    currentIndex = -1;
    for (size_t i = 0; i < presets.size(); ++i)
        if (presets[i].name == currentPreset.name && presets[i].factory == currentPreset.factory)
            currentIndex = (int) i;
}

void PresetManager::load (int index)
{
    if (index < 0 || index >= (int) presets.size())
        return;

    const auto& preset = presets[(size_t) index];
    const auto xml = preset.factory ? juce::parseXML (preset.xml) : juce::parseXML (preset.file);
    if (xml == nullptr || ! xml->hasTagName (presetTag))
        return;

    apply (*xml);
    currentIndex = index;
    state.state.setProperty (presetProperty, (preset.factory ? "factory:" : "user:") + preset.name, nullptr);
    takeSnapshot();
}

std::vector<float> PresetManager::valuesFrom (const juce::XmlElement& xml) const
{
    std::vector<float> values;
    for (auto* param : parametersOf (state))
    {
        auto value = param->getDefaultValue();
        for (const auto* p : xml.getChildWithTagNameIterator ("PARAM"))
            if (p->getStringAttribute ("id") == param->getParameterID())
                value = param->convertTo0to1 ((float) p->getDoubleAttribute ("value"));
        values.push_back (value);
    }
    return values;
}

void PresetManager::apply (const juce::XmlElement& xml)
{
    const auto values = valuesFrom (xml);
    const auto params = parametersOf (state);
    for (size_t i = 0; i < params.size(); ++i)
    {
        params[i]->beginChangeGesture();
        params[i]->setValueNotifyingHost (values[i]);
        params[i]->endChangeGesture();
    }
}

bool PresetManager::save (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty())
        return false;

    juce::XmlElement xml (presetTag);
    xml.setAttribute ("name", name);
    xml.setAttribute ("version", 1);
    for (auto* param : parametersOf (state))
    {
        auto* p = xml.createNewChildElement ("PARAM");
        p->setAttribute ("id", param->getParameterID());
        p->setAttribute ("value", param->convertFrom0to1 (param->getValue()));
    }

    const auto file = userFolder().getChildFile (name + fileExtension());
    if (! userFolder().createDirectory() || ! xml.writeTo (file))
        return false;

    rescanUserFolder();
    for (size_t i = (size_t) factoryCount; i < presets.size(); ++i)
        if (presets[i].file == file)
            currentIndex = (int) i;
    state.state.setProperty (presetProperty, "user:" + name, nullptr);
    takeSnapshot();
    return true;
}

juce::String PresetManager::currentName() const
{
    return currentIndex >= 0 ? presets[(size_t) currentIndex].name : juce::String ("INIT");
}

void PresetManager::takeSnapshot()
{
    snapshot.clear();
    for (auto* param : parametersOf (state))
        snapshot.push_back (param->getValue());
}

bool PresetManager::isModified() const
{
    const auto params = parametersOf (state);
    if (snapshot.size() != params.size())
        return false;
    for (size_t i = 0; i < params.size(); ++i)
        if (std::abs (params[i]->getValue() - snapshot[i]) > 1.0e-4f)
            return true;
    return false;
}

void PresetManager::restoreFromState()
{
    const auto stored = state.state.getProperty (presetProperty).toString();
    const auto factory = stored.startsWith ("factory:");
    const auto name = stored.fromFirstOccurrenceOf (":", false, false);

    currentIndex = -1;
    for (size_t i = 0; i < presets.size(); ++i)
        if (presets[i].factory == factory && presets[i].name == name)
            currentIndex = (int) i;

    // the set holds the sound as it was saved, maybe edited: compare against the preset itself
    if (currentIndex >= 0)
    {
        const auto& preset = presets[(size_t) currentIndex];
        const auto xml = preset.factory ? juce::parseXML (preset.xml) : juce::parseXML (preset.file);
        snapshot = xml != nullptr ? valuesFrom (*xml) : std::vector<float> {};
    }
    else
    {
        takeSnapshot();
    }
}
