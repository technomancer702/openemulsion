// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cmath>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "../third_party/nlohmann/json.hpp"
#include "ColorSpaceConfig.h"
#include "LookPresetConfig.h"

namespace userpreset {

using Json = nlohmann::json;
constexpr size_t MaximumBytes = 65536;
constexpr int FormatVersion = 7;
constexpr const char* Plugin = "org.openemulsion.film";

enum ContextKind { Choice, Number, Integer };
enum ContextGroup { Spaces, Camera, Seed };
struct ContextControl {
    const char* name;
    ContextKind kind;
    ContextGroup group;
    double initial, minimum, maximum;
};
inline constexpr ContextControl ContextControls[] {
    {"sourceSpace",Choice,Spaces,color::Rec709Gamma24,0,color::SpaceCount-1},
    {"outputSpace",Choice,Spaces,0,0,std::size(color::OutputSpaces)-1},
    {"exposure",Number,Camera,0,-4,4}, {"temperature",Number,Camera,0,-3,3},
    {"tint",Number,Camera,0,-3,3}, {"grainSeed",Integer,Seed,0,0,1000000},
    {"outputRendering",Choice,Spaces,color::Automatic,0,color::StandardHDR},
    {"hdrPeak",Number,Spaces,1000,400,10000}, {"hdrWhite",Number,Spaces,203,80,300},
    {"sdrContrast",Number,Spaces,0,-1,1}, {"sdrRolloff",Number,Spaces,0,-1,1},
    {"sdrGamut",Number,Spaces,0,-1,1},
    {"hdrExposure",Number,Spaces,0,-4,4}, {"hdrRolloff",Number,Spaces,0,-1,1}
};

struct Snapshot {
    std::string name;
    std::array<double, std::size(look::Controls)> controls {};
    std::array<bool, moduleui::Toggles.size()> modules {};
    std::array<double, std::size(ContextControls)> context {};
    Snapshot() {
        for (size_t i = 0; i < controls.size(); ++i) controls[i] = look::Controls[i].initial;
        for (size_t i = 0; i < modules.size(); ++i)
            modules[i] = (film::DefaultModules & moduleui::Toggles[i].module) != 0;
        for (size_t i = 0; i < context.size(); ++i) context[i] = ContextControls[i].initial;
    }
};

struct ImportOptions {
    bool preserveSpaces = false, preserveCamera = false, preserveSeed = false;
    bool preserves(ContextGroup group) const {
        return group == Spaces ? preserveSpaces : group == Camera ? preserveCamera : preserveSeed;
    }
};

template<class Binding> inline double readCurrentControl(const Binding& control)
{
    if (control.choice) { int value = 0; control.choice->getValue(value); return value; }
    if (control.integer) { int value = 0; control.integer->getValue(value); return value; }
    double value = 0;
    control.number->getValue(value);
    return value;
}

// UI export uses current host values, not a render-time sample or a recipe label.
template<class Controls, class Toggles, class Context>
inline Snapshot captureCurrent(const Controls& controls, const Toggles& toggles, const Context& context)
{
    Snapshot preset;
    for (size_t i = 0; i < preset.controls.size(); ++i) preset.controls[i] = readCurrentControl(controls[i]);
    for (size_t i = 0; i < preset.modules.size(); ++i) toggles[i]->getValue(preset.modules[i]);
    for (size_t i = 0; i < preset.context.size(); ++i) preset.context[i] = readCurrentControl(context[i]);
    return preset;
}

inline double validateNumber(double value, double minimum, double maximum, bool integer, const char* name)
{
    if (!std::isfinite(value) || value < minimum || value > maximum || (integer && value != std::floor(value)))
        throw std::runtime_error(std::string("Invalid preset value: ") + name);
    return value;
}

inline double readNumber(const Json& object, const char* name, double minimum, double maximum, bool integer)
{
    const auto& value = object.at(name);
    if (!value.is_number()) throw std::runtime_error(std::string("Expected a numeric preset value: ") + name);
    return validateNumber(value.get<double>(), minimum, maximum, integer, name);
}

inline Json toJson(const Snapshot& preset)
{
    if (preset.name.size() > 256) throw std::runtime_error("Preset name is too long.");
    Json controls = Json::object(), modules = Json::object(), context = Json::object();
    for (size_t i = 0; i < preset.controls.size(); ++i) {
        const auto& c = look::Controls[i];
        controls[c.name] = validateNumber(preset.controls[i], c.minimum, c.maximum, c.kind == look::Choice, c.name);
    }
    for (size_t i = 0; i < preset.modules.size(); ++i) modules[moduleui::Toggles[i].name] = preset.modules[i];
    for (size_t i = 0; i < preset.context.size(); ++i) {
        const auto& c = ContextControls[i];
        context[c.name] = validateNumber(preset.context[i], c.minimum, c.maximum, c.kind != Number, c.name);
    }
    return {{"formatVersion",FormatVersion}, {"plugin",Plugin}, {"createdWith","0.43"},
            {"name",preset.name}, {"controls",controls}, {"modules",modules}, {"context",context}};
}

inline std::string serialize(const Snapshot& preset)
{
    auto output = toJson(preset).dump(2) + "\n";
    if (output.size() > MaximumBytes) throw std::runtime_error("Preset exceeds the file size limit.");
    return output;
}

inline Snapshot parse(const std::string& input)
{
    if (input.empty() || input.size() > MaximumBytes) throw std::runtime_error("Invalid preset file size (maximum 64 KiB).");
    // Reject duplicate keys rather than silently accepting a later conflicting value.
    std::vector<std::set<std::string>> keys;
    const auto document = Json::parse(input, [&](int depth, Json::parse_event_t event, Json& value) {
        if (depth > 8) throw std::runtime_error("Preset JSON nesting is too deep.");
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        if (event == Json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)
            throw std::runtime_error("Preset contains a duplicate JSON field.");
        if (event == Json::parse_event_t::object_end) keys.pop_back();
        return true;
    });
    if (!document.is_object() || document.size() != 7 ||
        !document.contains("formatVersion") || !document["formatVersion"].is_number_integer() ||
        (document["formatVersion"] < 1 || document["formatVersion"] > FormatVersion) || document.value("plugin",std::string()) != Plugin)
        throw std::runtime_error("Not a supported OpenEmulsion preset (format version 1-7 required).");
    if (!document.at("createdWith").is_string() || document.at("createdWith").get<std::string>().size() > 64 ||
        !document.at("name").is_string() || document.at("name").get<std::string>().size() > 256)
        throw std::runtime_error("Invalid preset metadata.");
    Snapshot preset;
    preset.name = document.at("name").get<std::string>();
    const auto& controls = document.at("controls");
    const auto& modules = document.at("modules");
    const auto& context = document.at("context");
    const bool legacy = document["formatVersion"] == 1;
    const bool legacyRendering = document["formatVersion"] <= 2;
    const bool legacyRetention = document["formatVersion"] < 4;
    const bool legacyHDR = document["formatVersion"] < 5;
    const bool legacySDR = document["formatVersion"] < 6;
    const bool legacyHDRViewing = document["formatVersion"] < 7;
    if (!controls.is_object() || controls.size() != preset.controls.size() - (legacy ? 6 : 0) - (legacyRetention ? 1 : 0) ||
        !modules.is_object() || modules.size() != preset.modules.size() - (legacy ? 1 : 0) ||
        !context.is_object() || context.size() != preset.context.size() - (legacyRendering ? 1 : 0) - (legacyHDR ? 2 : 0) - (legacySDR ? 3 : 0) - (legacyHDRViewing ? 2 : 0))
        throw std::runtime_error("Incomplete or unsupported preset controls.");
    for (size_t i = 0; i < preset.controls.size(); ++i) {
        const auto& c = look::Controls[i];
        if ((legacy && c.setting >= film::SelectiveAmount) || (legacyRetention && c.setting == film::HighlightRetention)) continue;
        preset.controls[i] = readNumber(controls,c.name,c.minimum,c.maximum,c.kind == look::Choice);
    }
    for (size_t i = 0; i < preset.modules.size(); ++i) {
        if (legacy && moduleui::Toggles[i].module == film::SelectiveColor) {
            preset.modules[i] = false;
            continue;
        }
        const auto& value = modules.at(moduleui::Toggles[i].name);
        if (!value.is_boolean()) throw std::runtime_error("Preset module switches must be booleans.");
        preset.modules[i] = value.get<bool>();
    }
    for (size_t i = 0; i < preset.context.size(); ++i) {
        const auto& c = ContextControls[i];
        if (legacyHDRViewing && (std::string(c.name) == "hdrExposure" || std::string(c.name) == "hdrRolloff")) continue;
        if (legacySDR && (std::string(c.name) == "sdrContrast" || std::string(c.name) == "sdrRolloff" ||
                          std::string(c.name) == "sdrGamut")) continue;
        if (legacyHDR && (std::string(c.name) == "hdrPeak" || std::string(c.name) == "hdrWhite")) continue;
        if (legacyRendering && std::string(c.name) == "outputRendering") {
            preset.context[i] = color::ConversionOnly;
            continue;
        }
        preset.context[i] = readNumber(context,c.name,c.minimum,c.maximum,c.kind != Number);
        if (legacyHDR && ((std::string(c.name) == "outputRendering" && preset.context[i] > color::StandardSDR) ||
                          (std::string(c.name) == "outputSpace" && preset.context[i] >= color::HDRPQOutput)))
            throw std::runtime_error("HDR choices require preset format 5.");
    }
    return preset;
}

template<class Writer, class ToggleWriter, class ContextWriter>
inline void apply(const Snapshot& preset, ImportOptions options, Writer write, ToggleWriter toggle, ContextWriter context)
{
    for (size_t i = 0; i < preset.controls.size(); ++i) write(i,preset.controls[i]);
    for (size_t i = 0; i < preset.modules.size(); ++i) toggle(i,preset.modules[i]);
    for (size_t i = 0; i < preset.context.size(); ++i)
        if (!options.preserves(ContextControls[i].group)) context(i,preset.context[i]);
}

} // namespace userpreset
