// SPDX-License-Identifier: MPL-2.0

#include <cstdio>
#include <limits>
#include "UserPresetIO.h"

static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

template<class Function> static void rejects(Function operation, const char* message)
{
    bool rejected = false;
    try { operation(); } catch (const std::exception&) { rejected = true; }
    require(rejected,message);
}

template<class T> struct CurrentParam {
    T value {};
    int reads = 0;
    void getValue(T& result) { result = value; ++reads; }
    void getValueAtTime(double, T&) { throw std::runtime_error("Export sampled callback time instead of current UI"); }
};

struct Binding {
    CurrentParam<int>* choice = nullptr;
    CurrentParam<double>* number = nullptr;
    CurrentParam<int>* integer = nullptr;
};

static void testCurrentCapture()
{
    using namespace userpreset;
    std::array<CurrentParam<int>,std::size(look::Controls)> choices {};
    std::array<CurrentParam<double>,std::size(look::Controls)> numbers {};
    std::array<Binding,std::size(look::Controls)> controls {};
    std::array<CurrentParam<bool>,moduleui::Toggles.size()> switches {};
    std::array<CurrentParam<bool>*,moduleui::Toggles.size()> toggles {};
    std::array<CurrentParam<int>,std::size(ContextControls)> integers {};
    std::array<CurrentParam<double>,std::size(ContextControls)> contextNumbers {};
    std::array<Binding,std::size(ContextControls)> context {};
    Snapshot expected;
    for (size_t i = 0; i < controls.size(); ++i) {
        const auto& c = look::Controls[i];
        if (c.kind == look::Choice) {
            choices[i].value = static_cast<int>(c.maximum);
            controls[i].choice = &choices[i];
            expected.controls[i] = c.maximum;
        } else {
            numbers[i].value = c.minimum + (c.maximum-c.minimum)*.237;
            controls[i].number = &numbers[i];
            expected.controls[i] = numbers[i].value;
        }
    }
    for (size_t i = 0; i < toggles.size(); ++i) {
        switches[i].value = expected.modules[i] = i % 2 != 0;
        toggles[i] = &switches[i];
    }
    for (size_t i = 0; i < context.size(); ++i) {
        const auto& c = ContextControls[i];
        expected.context[i] = c.maximum;
        if (c.kind == Number) {
            contextNumbers[i].value = c.maximum;
            context[i].number = &contextNumbers[i];
        } else {
            integers[i].value = static_cast<int>(c.maximum);
            if (c.kind == Choice) context[i].choice = &integers[i];
            else context[i].integer = &integers[i];
        }
    }
    const auto captured = captureCurrent(controls,toggles,context);
    require(toJson(captured) == toJson(expected),"Current host binding capture loses edited settings");
    for (size_t i = 0; i < controls.size(); ++i)
        require(choices[i].reads + numbers[i].reads == 1,"Creative control not captured exactly once");
    for (const auto& toggle : switches) require(toggle.reads == 1,"Module switch not captured exactly once");
    for (size_t i = 0; i < context.size(); ++i)
        require(integers[i].reads + contextNumbers[i].reads == 1,"Context control not captured exactly once");
    // A subsequent modal dialog/host notification cannot mutate the saved snapshot.
    for (auto& number : numbers) number.value = 0;
    for (auto& choice : choices) choice.value = 0;
    for (auto& toggle : switches) toggle.value = true;
    for (auto& number : contextNumbers) number.value = 0;
    for (auto& integer : integers) integer.value = 0;
    require(toJson(parse(serialize(captured))) == toJson(expected),"Snapshot changed after dialog-time host edits");
    Snapshot restored;
    apply(captured,ImportOptions {},[&](size_t i,double v) { restored.controls[i] = v; },
        [&](size_t i,bool v) { restored.modules[i] = v; },
        [&](size_t i,double v) { restored.context[i] = v; });
    require(toJson(restored) == toJson(expected),"Default import does not restore the complete saved look");
}

int main()
{
    try {
        using namespace userpreset;
        testCurrentCapture();
        Snapshot original;
        for (size_t i = 0; i < original.modules.size(); ++i)
            require(original.modules[i] == (moduleui::Toggles[i].module != film::SelectiveColor),
                    "Default snapshot enables Selective Color or disables an ordinary module");
        original.name = "Night look \"A\" \n \u00e9";
        for (size_t i = 0; i < original.controls.size(); ++i) {
            const auto& c = look::Controls[i];
            original.controls[i] = c.kind == look::Choice ? c.maximum : c.minimum + (c.maximum-c.minimum)*.371;
        }
        for (size_t i = 0; i < original.modules.size(); ++i) original.modules[i] = i % 2 == 0;
        for (size_t i = 0; i < original.context.size(); ++i) original.context[i] = ContextControls[i].maximum;
        const auto encoded = serialize(original);
        const auto decoded = parse(encoded);
        require(toJson(decoded) == toJson(original),"Preset serialization loses settings or metadata");
        for (int mask = 0; mask < 8; ++mask) {
            const ImportOptions options {bool(mask&1),bool(mask&2),bool(mask&4)};
            Snapshot applied;
            const auto before = applied;
            int controls = 0, toggles = 0;
            apply(decoded,options,[&](size_t i, double v) { applied.controls[i] = v; ++controls; },
                [&](size_t i, bool v) { applied.modules[i] = v; ++toggles; },
                [&](size_t i, double v) { applied.context[i] = v; });
            require(controls == std::size(look::Controls) && toggles == moduleui::Toggles.size(),"Incomplete preset import");
            require(applied.controls == original.controls && applied.modules == original.modules,"Creative tuning not restored");
            for (size_t i = 0; i < applied.context.size(); ++i)
                require(applied.context[i] == (options.preserves(ContextControls[i].group) ? before.context[i] : original.context[i]),
                    "Import preservation policy failed");
        }
        for (int preset = 0; preset < look::Count; ++preset) {
            Snapshot stock;
            const auto recipe = look::recipe(preset);
            for (size_t i = 0; i < stock.controls.size(); ++i) stock.controls[i] = recipe[look::Controls[i].setting];
            for (size_t i = 0; i < stock.modules.size(); ++i)
                stock.modules[i] = (static_cast<int>(recipe[film::ModuleIndex]) & moduleui::Toggles[i].module) != 0;
            require(toJson(parse(serialize(stock))) == toJson(stock),"Built-in recipe does not round-trip");
        }
        const auto valid = toJson(original);
        for (const char* version : {"0.27","0.28","0.29"}) {
            auto older = valid;
            older["createdWith"] = version;
            require(toJson(parse(older.dump())) == valid,"Older preset settings no longer load");
        }
        auto invalid = valid;
        auto v5 = valid; v5["formatVersion"] = 5;
        for (const char* name : {"sdrContrast","sdrRolloff","sdrGamut"}) v5["context"].erase(name);
        auto v5Context = original.context;
        for (size_t i=0; i<v5Context.size(); ++i)
            if (std::string(ContextControls[i].name).rfind("sdr",0)==0) v5Context[i]=0;
        const auto migratedV5=parse(v5.dump());
        require(migratedV5.context==v5Context && migratedV5.controls==original.controls && migratedV5.modules==original.modules,
                "v5 migration changes existing look or SDR defaults");
        auto mixedV5=v5; mixedV5["context"]["sdrContrast"]=0;
        rejects([&] { parse(mixedV5.dump()); },"Mixed SDR schema accepted");
        auto oldValid = valid;
        for (const char* name : {"sdrContrast","sdrRolloff","sdrGamut"}) oldValid["context"].erase(name);
        oldValid["context"].erase("hdrPeak"); oldValid["context"].erase("hdrWhite");
        oldValid["context"]["outputSpace"] = color::HDRPQOutput-1;
        oldValid["context"]["outputRendering"] = color::StandardSDR;
        auto oldContext = original.context;
        for (size_t i=0; i<oldContext.size(); ++i) {
            const std::string name = ContextControls[i].name;
            if (name.rfind("sdr",0)==0) oldContext[i]=0;
            if (name == "hdrPeak" || name == "hdrWhite") oldContext[i] = ContextControls[i].initial;
            if (name == "outputSpace") oldContext[i] = color::HDRPQOutput-1;
            if (name == "outputRendering") oldContext[i] = color::StandardSDR;
        }
        auto v4=oldValid; v4["formatVersion"]=4;
        const auto migratedV4=parse(v4.dump());
        require(migratedV4.context==oldContext && migratedV4.controls==original.controls && migratedV4.modules==original.modules,
                "v4 migration changes existing settings or HDR defaults");
        auto mixedV4=v4; mixedV4["context"]["hdrPeak"]=1000;
        rejects([&] { parse(mixedV4.dump()); },"Mixed HDR schema accepted");
        auto legacy = oldValid;
        legacy["formatVersion"] = 1;
        legacy["context"].erase("outputRendering");
        for (const auto& c : look::Controls) if (c.setting >= film::SelectiveAmount) legacy["controls"].erase(c.name);
        legacy["modules"].erase("enableSelectiveColor");
        const auto migrated = parse(legacy.dump());
        for (size_t i=0; i<migrated.controls.size(); ++i)
            require(migrated.controls[i] == (look::Controls[i].setting >= film::SelectiveAmount ?
                look::Controls[i].initial : original.controls[i]),"Legacy migration changes existing settings");
        auto legacyContext=oldContext;
        for (size_t i=0; i<legacyContext.size(); ++i)
            if (std::string(ContextControls[i].name)=="outputRendering") legacyContext[i]=color::ConversionOnly;
        require(!migrated.modules.back() && migrated.context == legacyContext,"Legacy import enables selective color or changes context");
        auto oldControls=original.controls;
        for (size_t i=0; i<oldControls.size(); ++i) if (look::Controls[i].setting == film::HighlightRetention) oldControls[i]=0;
        auto v2=oldValid; v2["formatVersion"]=2; v2["context"].erase("outputRendering");
        v2["controls"].erase("highlightRetention");
        const auto migratedV2=parse(v2.dump());
        require(migratedV2.controls == oldControls && migratedV2.modules == original.modules &&
            migratedV2.context == legacyContext,"v2 preset gains an unrequested display rendering");
        auto v3=oldValid; v3["formatVersion"]=3; v3["controls"].erase("highlightRetention");
        const auto migratedV3=parse(v3.dump());
        require(migratedV3.controls==oldControls && migratedV3.context==oldContext && migratedV3.modules==original.modules,
            "v3 migration changes rendering or enables highlight retention");
        auto mixedV3=v3; mixedV3["controls"]["highlightRetention"]=.5;
        rejects([&] { parse(mixedV3.dump()); },"Mixed highlight schema accepted");
        auto brokenV2=v2; brokenV2["context"]["outputRendering"]=0;
        rejects([&] { parse(brokenV2.dump()); },"Mixed rendering schema accepted");
        auto brokenLegacy=legacy; brokenLegacy["controls"].erase("grain");
        rejects([&] { parse(brokenLegacy.dump()); },"Incomplete legacy preset accepted");
        brokenLegacy=legacy; brokenLegacy["controls"]["selectiveAmount"]=1;
        rejects([&] { parse(brokenLegacy.dump()); },"Mixed-schema preset accepted");
        invalid["formatVersion"] = FormatVersion+1;
        rejects([&] { parse(invalid.dump()); },"Future schema silently accepted");
        invalid = valid; invalid["formatVersion"] = 1.0;
        rejects([&] { parse(invalid.dump()); },"Non-integer schema accepted");
        invalid = valid; invalid["plugin"] = "other.plugin";
        rejects([&] { parse(invalid.dump()); },"Foreign plugin accepted");
        invalid = valid; invalid["controls"].erase("grainResponse");
        rejects([&] { parse(invalid.dump()); },"Incomplete settings accepted");
        invalid["controls"]["unsupported"] = 0;
        rejects([&] { parse(invalid.dump()); },"Unknown control accepted");
        invalid = valid; invalid["modules"]["enableGrain"] = 1;
        rejects([&] { parse(invalid.dump()); },"Numeric module switch accepted");
        invalid = valid; invalid["controls"]["grainResponse"] = .5;
        rejects([&] { parse(invalid.dump()); },"Fractional choice accepted");
        invalid = valid; invalid["context"]["grainSeed"] = 1000001;
        rejects([&] { parse(invalid.dump()); },"Invalid context accepted even when preserved");
        for (const auto& c : look::Controls) {
            for (auto value : {Json(nullptr),Json("1"),Json(true),Json(c.minimum-1),Json(c.maximum+1)}) {
                invalid = valid; invalid["controls"][c.name] = value;
                rejects([&] { parse(invalid.dump()); },"Invalid control value accepted");
            }
        }
        for (const auto& c : ContextControls) {
            for (auto value : {Json(nullptr),Json("1"),Json(true),Json(c.minimum-1),Json(c.maximum+1)}) {
                invalid=valid; invalid["context"][c.name]=value;
                rejects([&] { parse(invalid.dump()); },"Invalid context value accepted");
            }
        }
        rejects([&] { parse("{\"name\":\"duplicate\"," + valid.dump().substr(1)); },"Duplicate root key accepted");
        auto duplicated = encoded;
        const auto field = duplicated.find("\"grainResponse\"");
        duplicated.insert(field,"\"grainResponse\": 0, ");
        rejects([&] { parse(duplicated); },"Duplicate control accepted");
        rejects([&] { parse(encoded + "garbage"); },"Trailing content accepted");
        rejects([&] { parse(std::string(MaximumBytes+1,' ')); },"Oversize file accepted");
        rejects([&] { parse("{\"a\":" + std::string(12,'[') + "0" + std::string(12,']') + "}"); },"Deep nesting accepted");
        auto nonfinite = original; nonfinite.controls[0] = std::numeric_limits<double>::quiet_NaN();
        rejects([&] { serialize(nonfinite); },"Nonfinite export accepted");

        const auto folder = std::filesystem::temp_directory_path() /
            ("OpenEmulsionPresetTests-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
        require(std::filesystem::create_directory(folder),"Could not create unique test folder");
        const auto path = folder / L"look-\u00e9-\u7535\u5f71.oepreset";
        struct Cleanup {
            std::filesystem::path file, folder;
            ~Cleanup() { std::error_code error; std::filesystem::remove(file,error); std::filesystem::remove(folder,error); }
        } cleanup {path,folder};
        saveFile(path,original);
        require(toJson(loadFile(path)) == valid,"Unicode file path did not round-trip");
        auto replacement = original; replacement.name = "Replacement";
        saveFile(path,replacement);
        require(loadFile(path).name == replacement.name,"Replacing preset failed");
        rejects([&] { saveFile(path,nonfinite); },"Invalid preset was saved");
        require(loadFile(path).name == replacement.name,"Rejected export damaged existing file");
        HANDLE lock = CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        require(lock != INVALID_HANDLE_VALUE,"Could not lock test destination");
        bool refused = false;
        try { saveFile(path,original); } catch (const std::exception&) { refused = true; }
        CloseHandle(lock);
        require(refused && loadFile(path).name == replacement.name,"Failed replacement damaged existing file");
        for (const auto& entry : std::filesystem::directory_iterator(folder))
            require(entry.path() == path,"Temporary preset file leaked");
        rejects([&] { saveFile(folder/"missing"/"look.oepreset",original); },"Missing folder silently accepted");
        std::puts("User presets: current host binding capture, immutable pre-dialog snapshot, complete default restore, all recipes, exact JSON round-trip, every preservation policy, strict validation, Unicode paths, atomic replacement, and failed-save preservation pass.");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr,"FAILED: %s\n",error.what());
        return 1;
    }
}
