// SPDX-License-Identifier: MPL-2.0

#include <windows.h>
#include <ofxImageEffect.h>
#include <ofxMemory.h>
#include <ofxMessage.h>
#include <ofxMultiThread.h>
#include <ofxParam.h>
#include <ofxProperty.h>
#include "ColorSpaceConfig.h"

#include <cstdarg>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace {

// Minimal host for descriptor/instance actions, not a rendering host. Load the
// shipping binary so failures in OFX support wrappers are covered as well.
using Value = std::variant<int, double, std::string, void*>;
using Properties = std::map<std::string, std::vector<Value>>;
struct Parameter { Properties properties; };
struct Effect {
    Properties properties;
    std::map<std::string, Parameter> parameters;
    std::map<std::string, Properties> clips;
};

OfxPropertySetHandle handle(Properties& p) { return reinterpret_cast<OfxPropertySetHandle>(&p); }

template<class T>
OfxStatus set(OfxPropertySetHandle h, const char* name, int index, T value)
{
    if (!h) return kOfxStatErrBadHandle;
    if (index < 0) return kOfxStatErrBadIndex;
    auto& values = (*reinterpret_cast<Properties*>(h))[name];
    if (values.size() <= static_cast<size_t>(index)) values.resize(index + 1, T{});
    values[index] = value;
    return kOfxStatOK;
}

template<class T>
OfxStatus get(OfxPropertySetHandle h, const char* name, int index, T* value)
{
    if (!h) return kOfxStatErrBadHandle;
    auto& properties = *reinterpret_cast<Properties*>(h);
    auto it = properties.find(name);
    if (it == properties.end()) return kOfxStatErrUnknown;
    if (index < 0 || static_cast<size_t>(index) >= it->second.size()) return kOfxStatErrBadIndex;
    auto* result = std::get_if<T>(&it->second[index]);
    if (!result) return kOfxStatErrValue;
    *value = *result;
    return kOfxStatOK;
}

OfxStatus setString(OfxPropertySetHandle h, const char* name, int index, const char* value)
{
    return set(h, name, index, std::string(value));
}

OfxStatus getString(OfxPropertySetHandle h, const char* name, int index, char** value)
{
    if (!h) return kOfxStatErrBadHandle;
    auto& properties = *reinterpret_cast<Properties*>(h);
    auto it = properties.find(name);
    if (it == properties.end()) return kOfxStatErrUnknown;
    if (index < 0 || static_cast<size_t>(index) >= it->second.size()) return kOfxStatErrBadIndex;
    auto* result = std::get_if<std::string>(&it->second[index]);
    if (!result) return kOfxStatErrValue;
    *value = result->data();
    return kOfxStatOK;
}

template<class T, auto Function>
OfxStatus setMany(OfxPropertySetHandle h, const char* name, int count, const T* values)
{
    for (int i = 0; i < count; ++i) {
        const auto status = Function(h, name, i, values[i]);
        if (status != kOfxStatOK) return status;
    }
    return kOfxStatOK;
}

template<class T, auto Function>
OfxStatus getMany(OfxPropertySetHandle h, const char* name, int count, T* values)
{
    for (int i = 0; i < count; ++i) {
        const auto status = Function(h, name, i, &values[i]);
        if (status != kOfxStatOK) return status;
    }
    return kOfxStatOK;
}

OfxPropertySuiteV1 propertySuite = {
    set<void*>, setString, set<double>, set<int>,
    setMany<void*, set<void*>>, setMany<const char*, setString>,
    setMany<double, set<double>>, setMany<int, set<int>>,
    get<void*>, getString, get<double>, get<int>,
    getMany<void*, get<void*>>, getMany<char*, getString>,
    getMany<double, get<double>>, getMany<int, get<int>>,
    [](OfxPropertySetHandle h, const char* name) -> OfxStatus {
        if (!h) return kOfxStatErrBadHandle;
        reinterpret_cast<Properties*>(h)->erase(name);
        return kOfxStatOK;
    },
    [](OfxPropertySetHandle h, const char* name, int* count) -> OfxStatus {
        if (!h) return kOfxStatErrBadHandle;
        auto& properties = *reinterpret_cast<Properties*>(h);
        auto it = properties.find(name);
        *count = it == properties.end() ? 0 : static_cast<int>(it->second.size());
        return kOfxStatOK;
    }
};

OfxStatus parameterValue(OfxParamHandle h, va_list args, bool write)
{
    auto& p = reinterpret_cast<Parameter*>(h)->properties;
    const auto& type = std::get<std::string>(p.at(kOfxParamPropType)[0]);
    if (type == kOfxParamTypeDouble) {
        if (write) return set(handle(p), kOfxParamPropDefault, 0, va_arg(args, double));
        return get(handle(p), kOfxParamPropDefault, 0, va_arg(args, double*));
    }
    if (type == kOfxParamTypeBoolean || type == kOfxParamTypeChoice || type == kOfxParamTypeInteger) {
        if (write) return set(handle(p), kOfxParamPropDefault, 0, va_arg(args, int));
        return get(handle(p), kOfxParamPropDefault, 0, va_arg(args, int*));
    }
    return kOfxStatErrUnsupported;
}

OfxStatus getParameterValue(OfxParamHandle h, ...)
{
    va_list args;
    va_start(args, h);
    const auto status = parameterValue(h, args, false);
    va_end(args);
    return status;
}

OfxStatus setParameterValue(OfxParamHandle h, ...)
{
    va_list args;
    va_start(args, h);
    const auto status = parameterValue(h, args, true);
    va_end(args);
    return status;
}

OfxImageEffectSuiteV1 effectSuite{};
OfxParameterSuiteV1 parameterSuite{};
OfxMemorySuiteV1 memorySuite{};
OfxMultiThreadSuiteV1 threadSuite{};
OfxMessageSuiteV1 messageSuite{};

void initializeSuites()
{
    effectSuite.getPropertySet = [](OfxImageEffectHandle h, OfxPropertySetHandle* p) -> OfxStatus {
        *p = handle(reinterpret_cast<Effect*>(h)->properties);
        return kOfxStatOK;
    };
    effectSuite.getParamSet = [](OfxImageEffectHandle h, OfxParamSetHandle* p) -> OfxStatus {
        *p = reinterpret_cast<OfxParamSetHandle>(h);
        return kOfxStatOK;
    };
    effectSuite.clipDefine = [](OfxImageEffectHandle h, const char* name, OfxPropertySetHandle* p) -> OfxStatus {
        *p = handle(reinterpret_cast<Effect*>(h)->clips[name]);
        return kOfxStatOK;
    };
    effectSuite.clipGetHandle = [](OfxImageEffectHandle h, const char* name, OfxImageClipHandle* clip,
                                   OfxPropertySetHandle* p) -> OfxStatus {
        auto& clips = reinterpret_cast<Effect*>(h)->clips;
        auto it = clips.find(name);
        if (it == clips.end()) return kOfxStatErrUnknown;
        *clip = reinterpret_cast<OfxImageClipHandle>(&it->second);
        if (p) *p = handle(it->second);
        return kOfxStatOK;
    };
    parameterSuite.paramDefine = [](OfxParamSetHandle h, const char* type, const char* name,
                                     OfxPropertySetHandle* p) -> OfxStatus {
        auto& params = reinterpret_cast<Effect*>(h)->parameters;
        auto inserted = params.emplace(name, Parameter{});
        if (!inserted.second) return kOfxStatErrExists;
        *p = handle(inserted.first->second.properties);
        setString(*p, kOfxPropName, 0, name);
        setString(*p, kOfxParamPropType, 0, type);
        setString(*p, kOfxParamPropParent, 0, "");
        set(*p, kOfxParamPropEnabled, 0, 1);
        return kOfxStatOK;
    };
    parameterSuite.paramGetHandle = [](OfxParamSetHandle h, const char* name, OfxParamHandle* param,
                                        OfxPropertySetHandle* p) -> OfxStatus {
        auto& params = reinterpret_cast<Effect*>(h)->parameters;
        auto it = params.find(name);
        if (it == params.end()) return kOfxStatErrUnknown;
        *param = reinterpret_cast<OfxParamHandle>(&it->second);
        if (p) *p = handle(it->second.properties);
        return kOfxStatOK;
    };
    parameterSuite.paramSetGetPropertySet = [](OfxParamSetHandle h, OfxPropertySetHandle* p) -> OfxStatus {
        *p = handle(reinterpret_cast<Effect*>(h)->properties);
        return kOfxStatOK;
    };
    parameterSuite.paramGetPropertySet = [](OfxParamHandle h, OfxPropertySetHandle* p) -> OfxStatus {
        *p = handle(reinterpret_cast<Parameter*>(h)->properties);
        return kOfxStatOK;
    };
    parameterSuite.paramGetValue = getParameterValue;
    parameterSuite.paramSetValue = setParameterValue;
    parameterSuite.paramEditBegin = [](OfxParamSetHandle, const char*) -> OfxStatus { return kOfxStatOK; };
    parameterSuite.paramEditEnd = [](OfxParamSetHandle) -> OfxStatus { return kOfxStatOK; };
}

const void* fetchSuite(OfxPropertySetHandle, const char* name, int version)
{
    if (version != 1) return nullptr;
    if (std::string(name) == kOfxPropertySuite) return &propertySuite;
    if (std::string(name) == kOfxImageEffectSuite) return &effectSuite;
    if (std::string(name) == kOfxParameterSuite) return &parameterSuite;
    if (std::string(name) == kOfxMemorySuite) return &memorySuite;
    if (std::string(name) == kOfxMultiThreadSuite) return &threadSuite;
    if (std::string(name) == kOfxMessageSuite) return &messageSuite;
    return nullptr;
}

void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

template<class T>
T value(Properties& p, const char* name)
{
    return std::get<T>(p.at(name).at(0));
}

void testContext(OfxPlugin& plugin, const char* context)
{
    Effect descriptor;
    Properties args;
    setString(handle(args), kOfxImageEffectPropContext, 0, context);
    auto action = [&](const char* name, Effect& effect, OfxPropertySetHandle in = nullptr) {
        std::cout << context << ": " << name << std::endl;
        require(plugin.mainEntry(name, &effect, in, nullptr) == kOfxStatOK, std::string(name) + " failed");
    };
    action(kOfxActionDescribe, descriptor);
    action(kOfxImageEffectActionDescribeInContext, descriptor, handle(args));

    for (const char* name : {"hdrPeak", "hdrWhite"}) {
        auto& p = descriptor.parameters.at(name).properties;
        require(value<std::string>(p, kOfxParamPropParent).empty(), "HDR slider must be ungrouped");
        require(value<std::string>(p, kOfxParamPropType) == kOfxParamTypeDouble, "HDR slider type");
    }
    require(value<double>(descriptor.parameters.at("hdrPeak").properties, kOfxParamPropDefault) == 1000, "Peak default");
    require(value<double>(descriptor.parameters.at("hdrWhite").properties, kOfxParamPropDefault) == 203, "White default");
    for (auto& entry : descriptor.parameters) {
        const auto parent = value<std::string>(entry.second.properties, kOfxParamPropParent);
        if (parent.empty()) continue;
        auto it = descriptor.parameters.find(parent);
        require(it != descriptor.parameters.end(), "Missing parent for " + entry.first);
        require(value<std::string>(it->second.properties, kOfxParamPropType) == kOfxParamTypeGroup, "Non-group parent");
    }
    auto& children = descriptor.parameters.at("Controls").properties.at(kOfxParamPropPageChild);
    size_t peak = children.size(), white = children.size(), gauge = children.size();
    for (size_t i = 0; i < children.size(); ++i) {
        auto name = std::get<std::string>(children[i]);
        if (name == "hdrPeak") peak = i;
        if (name == "hdrWhite") white = i;
        if (name == "filmGauge") gauge = i;
    }
    require(peak < white && white < gauge, "HDR sliders must precede Film Gauge");

    for (bool hdr : {false, true}) {
        Effect instance = descriptor;
        setString(handle(instance.properties), kOfxImageEffectPropContext, 0, context);
        if (hdr) {
            // Scene LogC3 + Auto activates both HDR sliders on the first UI refresh.
            set(handle(instance.parameters.at("sourceSpace").properties), kOfxParamPropDefault, 0, int(color::AlexaLogC3));
            set(handle(instance.parameters.at("outputSpace").properties), kOfxParamPropDefault, 0, color::HDRPQOutput);
        }
        action(kOfxActionCreateInstance, instance);
        require(value<void*>(instance.properties, kOfxPropInstanceData) != nullptr, "Missing instance data");
        for (const char* name : {"hdrPeak", "hdrWhite"})
            require(value<int>(instance.parameters.at(name).properties, kOfxParamPropEnabled) == int(hdr), "HDR enable state");
        action(kOfxActionDestroyInstance, instance);
        require(value<void*>(instance.properties, kOfxPropInstanceData) == nullptr, "Instance not destroyed");
    }
}

} // namespace

int main(int argc, char** argv)
{
    try {
        require(argc == 2, "Pass the built plugin path");
        initializeSuites();
        Properties hostProperties;
        auto p = handle(hostProperties);
        setString(p, kOfxPropName, 0, "OpenEmulsion.StartupTestHost");
        setString(p, kOfxPropLabel, 0, "OpenEmulsion startup test");
        for (const char* name : {
            kOfxImageEffectHostPropIsBackground, kOfxImageEffectPropSupportsOverlays,
            kOfxImageEffectPropSupportsMultiResolution, kOfxImageEffectPropSupportsTiles,
            kOfxImageEffectPropTemporalClipAccess, kOfxImageEffectPropSupportsMultipleClipDepths,
            kOfxImageEffectPropSupportsMultipleClipPARs, kOfxImageEffectPropSetableFrameRate,
            kOfxImageEffectPropSetableFielding, kOfxParamHostPropSupportsStringAnimation,
            kOfxParamHostPropSupportsCustomInteract, kOfxParamHostPropSupportsChoiceAnimation,
            kOfxParamHostPropSupportsBooleanAnimation, kOfxParamHostPropSupportsCustomAnimation,
            kOfxParamHostPropMaxParameters, kOfxParamHostPropMaxPages}) set(p, name, 0, 0);
        set(p, kOfxParamHostPropPageRowColumnCount, 0, 0);
        set(p, kOfxParamHostPropPageRowColumnCount, 1, 0);
        setString(p, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
        setString(p, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
        setString(p, kOfxImageEffectPropSupportedContexts, 1, kOfxImageEffectContextGeneral);
        setString(p, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
        OfxHost host{p, fetchSuite};
        HMODULE module = LoadLibraryExA(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        require(module != nullptr, "Cannot load plugin: " + std::to_string(GetLastError()));
        auto count = reinterpret_cast<int(*)()>(GetProcAddress(module, "OfxGetNumberOfPlugins"));
        auto getPlugin = reinterpret_cast<OfxPlugin*(*)(int)>(GetProcAddress(module, "OfxGetPlugin"));
        require(count && getPlugin && count() == 1, "OFX exports");
        auto* plugin = getPlugin(0);
        plugin->setHost(&host);
        require(plugin->mainEntry(kOfxActionLoad, nullptr, nullptr, nullptr) == kOfxStatOK, "Load action");
        testContext(*plugin, kOfxImageEffectContextFilter);
        testContext(*plugin, kOfxImageEffectContextGeneral);
        require(plugin->mainEntry(kOfxActionUnload, nullptr, nullptr, nullptr) == kOfxStatOK, "Unload action");
        FreeLibrary(module);
        std::cout << "Plugin startup regression passed" << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
