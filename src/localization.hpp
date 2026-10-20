#pragma once
#include "language_tag.hpp"
#include "resources.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <chrono>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <mach-o/dyld.h>
#endif

inline const Json& builtins() { static const Json data=Json::parse(embeddedResources); return data; }
inline std::string environment(const char* name) { const char* value=std::getenv(name); return value?value:""; }
#ifdef _WIN32
inline std::string utf8(const std::wstring& value) {
    if(value.empty()) return {};
    int count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),int(value.size()),nullptr,0,nullptr,nullptr);
    if(!count) throw std::runtime_error("invalid_language");
    std::string result(count,'\0');
    WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),int(value.size()),result.data(),count,nullptr,nullptr);
    return result;
}
inline std::wstring wide(const std::string& value) {
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),nullptr,0);
    if(!count && !value.empty()) throw std::runtime_error("invalid_language");
    std::wstring result(count,L'\0');
    MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),result.data(),count); return result;
}
#endif
inline std::vector<std::string> unicodeArguments(int argc,char** argv) {
    std::vector<std::string> result;
#ifdef _WIN32
    (void)argc; (void)argv;
    SetConsoleOutputCP(CP_UTF8); SetConsoleCP(CP_UTF8);
    int count=0; auto arguments=CommandLineToArgvW(GetCommandLineW(),&count);
    if(!arguments) throw std::runtime_error("invalid_language");
    try { for(int i=0;i<count;i++) result.push_back(utf8(arguments[i])); }
    catch(...) { LocalFree(arguments); throw; }
    LocalFree(arguments);
#else
    for(int i=0;i<argc;i++) result.emplace_back(argv[i]);
#endif
    return result;
}
inline std::filesystem::path executableDirectory(const std::string& argv0) {
#ifdef _WIN32
    std::wstring path(32768,L'\0'); auto count=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
    if(count && count<path.size()) {path.resize(count);return std::filesystem::path(path).parent_path();}
#elif defined(__APPLE__)
    uint32_t count=0; _NSGetExecutablePath(nullptr,&count); std::string path(count,'\0');
    if(_NSGetExecutablePath(path.data(),&count)==0) return std::filesystem::weakly_canonical(path.c_str()).parent_path();
#else
    std::error_code ec; auto path=std::filesystem::read_symlink("/proc/self/exe",ec);
    if(!ec) return path.parent_path();
#endif
    return std::filesystem::absolute(std::filesystem::u8path(argv0)).parent_path();
}
inline std::filesystem::path defaultSettingsPath() {
#ifdef _WIN32
    // Use wide environment APIs for Unicode Windows user profiles.
    for(const wchar_t* name:{L"LOCALAPPDATA",L"APPDATA"}) {
        DWORD size=GetEnvironmentVariableW(name,nullptr,0);
        if(size) {std::wstring value(size,L'\0');auto n=GetEnvironmentVariableW(name,value.data(),size);value.resize(n);return std::filesystem::path(value)/L"UniFrame"/L"settings.json";}
    }
#elif defined(__APPLE__)
    if(!environment("HOME").empty()) return std::filesystem::u8path(environment("HOME"))/"Library/Application Support/UniFrame/settings.json";
#else
    auto xdg=environment("XDG_CONFIG_HOME");
    if(!xdg.empty() && std::filesystem::u8path(xdg).is_absolute()) return std::filesystem::u8path(xdg)/"uniframe/settings.json";
    if(!environment("HOME").empty()) return std::filesystem::u8path(environment("HOME"))/".config/uniframe/settings.json";
#endif
    return {};
}
struct LocaleArguments {
    std::vector<std::string> remaining;
    std::optional<std::string> name,uiLanguage,locale;
    std::filesystem::path settings,resources;
    bool show=false,menu=false,list=false,reset=false;
};
inline LocaleArguments localeArguments(const std::vector<std::string>& args) {
    LocaleArguments result; result.remaining.push_back(args.at(0));
    for(size_t i=1;i<args.size();i++) {
        auto arg=args[i];
        if(arg=="--settings") result.show=true;
        else if(arg=="--settings-menu") result.menu=true;
        else if(arg=="--list-languages") result.list=true;
        else if(arg=="--reset-settings") result.reset=true;
        else if(arg=="--name"||arg=="--ui-language"||arg=="--locale"||arg=="--config"||arg=="--resources") {
            if(i+1==args.size()) throw std::runtime_error("missing_option_value");
            auto value=args[++i];
            if(arg=="--name") result.name=value;
            else if(arg=="--ui-language") result.uiLanguage=value;
            else if(arg=="--locale") result.locale=value;
            else if(arg=="--config") result.settings=std::filesystem::u8path(value);
            else result.resources=std::filesystem::u8path(value);
        } else result.remaining.push_back(arg);
    }
    return result;
}
class Localizer {
    Json catalogs_=builtins().at("catalogs"),names_=builtins().at("names");
    Json preferences_=Json::object();
    std::filesystem::path settings_;
    std::string detectedSource_,effective_="en",programName_,requested_;
    LanguageTag detected_;
    std::vector<std::string> warnings_;
    static Json readJson(const std::filesystem::path& path,size_t limit=2*1024*1024) {
        if(std::filesystem::file_size(path)>limit) throw std::runtime_error("resource_invalid");
        std::ifstream file(path); if(!file) throw std::runtime_error("resource_invalid");
        return Json::parse(file);
    }
    LanguageTag detect(const LocaleArguments& args) {
        if(args.locale) {
            detectedSource_="command_line_override";
            auto tag=standard.normalize(*args.locale,true);
            if(!tag.valid && *args.locale!="C" && *args.locale!="POSIX" && *args.locale!="C.UTF-8") throw std::runtime_error("invalid_language");
            return tag;
        }
#ifdef _WIN32
        ULONG count=0,size=0;
        GetUserPreferredUILanguages(MUI_LANGUAGE_NAME,&count,nullptr,&size);
        if(size) {
            std::vector<wchar_t> buffer(size);
            if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME,&count,buffer.data(),&size)) {
                const wchar_t* item=buffer.data();
                while(*item) {auto t=standard.normalize(utf8(item));if(t.valid){detectedSource_="windows_preferred_ui_languages";return t;} item+=wcslen(item)+1;}
            }
        }
        wchar_t name[LOCALE_NAME_MAX_LENGTH]{};
        if(GetUserDefaultLocaleName(name,LOCALE_NAME_MAX_LENGTH)) {detectedSource_="windows_user_locale";return standard.normalize(utf8(name));}
#elif defined(__APPLE__)
        auto languages=CFLocaleCopyPreferredLanguages();
        if(languages) {
            LanguageTag found;
            for(CFIndex i=0;i<CFArrayGetCount(languages);i++) {
                auto value=static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages,i));
                std::vector<char> buffer(size_t(CFStringGetMaximumSizeForEncoding(CFStringGetLength(value),kCFStringEncodingUTF8)+1));
                if(CFStringGetCString(value,buffer.data(),buffer.size(),kCFStringEncodingUTF8)) {found=standard.normalize(buffer.data());if(found.valid)break;}
            }
            CFRelease(languages); if(found.valid){detectedSource_="macos_preferred_languages";return found;}
        }
#endif
        // GNU message-language precedence. C/POSIX disables LANGUAGE overrides.
        std::string effective,source;
        for(const char* key:{"LC_ALL","LC_MESSAGES","LANG"}) if(!environment(key).empty()) {effective=environment(key);source=key;break;}
        if(!effective.empty() && !standard.normalize(effective,true).valid &&
           (lower(effective)=="c"||lower(effective)=="posix"||lower(effective).rfind("c.",0)==0)) {detectedSource_="environment:"+source;return {};}
        for(auto candidate:split(environment("LANGUAGE"),':')) {
            auto tag=standard.normalize(candidate,true);
            if(tag.valid) {detectedSource_="environment:LANGUAGE";return tag;}
        }
        auto tag=standard.normalize(effective,true);detectedSource_=tag.valid?"environment:"+source:"unavailable";return tag;
    }
    void validatePreferences() {
        auto name=preferences_.value("name",std::string("auto"));
        const std::vector<std::string> allowed{"auto","EVRKare","UniFrame","XFrame","ӘмКадр"};
        if(std::find(allowed.begin(),allowed.end(),name)==allowed.end()) throw std::runtime_error("invalid_name");
        auto language=preferences_.value("ui_language",std::string("auto"));
        if(language!="auto") {
            auto tag=standard.normalize(language);
            if(!tag.valid) throw std::runtime_error("invalid_language");
            preferences_["ui_language"]=tag.canonical;
        }
        preferences_["name"]=name;
    }
    void resolve() {
        requested_=preferences_.value("ui_language",std::string("auto"));
        auto tag=requested_=="auto"?detected_:standard.normalize(requested_);
        effective_="en";
        // BCP 47 lookup: strip extensions/variants/region/script progressively.
        auto candidate=tag.canonical;
        while(!candidate.empty()) {
            if(catalogs_.contains(candidate)) {effective_=candidate;break;}
            auto at=candidate.rfind('-');if(at==std::string::npos)break;candidate.resize(at);
        }
        auto name=preferences_.value("name",std::string("auto"));
        if(name!="auto") programName_=name;
        else if(!detected_.valid || detected_.language.empty() || detected_.language=="und" || detected_.language=="zxx") programName_=names_.at("undetected").get<std::string>();
        else {
            const auto& rules=names_.at("languages");
            programName_=rules.value(detected_.canonical,rules.value(detected_.language,names_.at("default").get<std::string>()));
        }
    }
public:
    LanguageStandard standard{builtins().at("aliases"),builtins().at("likely"),builtins().at("scripts")};
    Localizer(const LocaleArguments& args,const std::filesystem::path& executable) {
        auto dir=args.resources.empty()?executable/"locales":args.resources;
        if(std::filesystem::exists(dir)) {
            if(!std::filesystem::is_directory(dir)) throw std::runtime_error("resource_invalid");
            for(const auto& entry:std::filesystem::directory_iterator(dir)) {
                if(!entry.is_regular_file() || entry.path().extension()!=".json") continue;
                try {
                    auto data=readJson(entry.path());
                    if(entry.path().filename()=="names.json") {
                        if(!data.at("default").is_string()||!data.at("undetected").is_string()||!data.at("languages").is_object()) throw std::runtime_error("resource_invalid");
                        Json rules=Json::object();
                        for(auto it=data["languages"].begin();it!=data["languages"].end();++it) {
                            auto tag=standard.normalize(it.key());
                            if(!tag.valid||!it.value().is_string())throw std::runtime_error("resource_invalid");
                            rules[tag.canonical]=it.value();
                        }
                        data["languages"]=rules; names_=std::move(data);
                    } else {
                        auto tag=standard.normalize(entry.path().stem().u8string());
                        if(!tag.valid || !data.at("messages").is_object()) throw std::runtime_error("resource_invalid");
                        for(const auto& value:data["messages"]) if(!value.is_string()) throw std::runtime_error("resource_invalid");
                        if(data.contains("_meta")) {
                            if(!data["_meta"].is_object() || !data["_meta"].value("native_name",tag.canonical).size()) throw std::runtime_error("resource_invalid");
                            auto direction=data["_meta"].value("direction",tag.direction);
                            if(direction!="rtl" && direction!="ltr") throw std::runtime_error("resource_invalid");
                        }
                        catalogs_[tag.canonical]=std::move(data);
                    }
                } catch(const std::exception&) {warnings_.push_back(entry.path().filename().u8string());}
            }
        } else if(!args.resources.empty()) throw std::runtime_error("resource_invalid");
        // The embedded English dictionary is always the final per-key fallback.
        settings_=args.settings.empty()?defaultSettingsPath():args.settings;
        if(!args.reset && !settings_.empty() && std::filesystem::exists(settings_)) {
            try {preferences_=readJson(settings_,65536);if(!preferences_.is_object())throw std::runtime_error("settings_invalid");}
            catch(const std::exception&) {throw std::runtime_error("settings_invalid");}
        }
        if(args.name) preferences_["name"]=*args.name;
        if(args.uiLanguage) preferences_["ui_language"]=*args.uiLanguage;
        validatePreferences();detected_=detect(args);resolve();
        if(args.name || args.uiLanguage || args.reset) save();
    }
    const std::string& name() const {return programName_;}
    std::string text(const std::string& key,const std::vector<std::pair<std::string,std::string>>& values={}) const {
        std::string value;
        const auto& english=builtins().at("catalogs").at("en").at("messages");
        const auto& messages=catalogs_.at(effective_).at("messages");
        if(messages.contains(key)) value=messages.at(key).get<std::string>();
        else if(english.contains(key)) value=english.at(key).get<std::string>();
        else value=key;
        // Single-pass substitution: a path/name containing {tokens} stays literal.
        std::string result;
        for(size_t i=0;i<value.size();) {
            auto end=value[i]=='{'?value.find('}',i):std::string::npos; bool replaced=false;
            if(end!=std::string::npos) {
                auto token=value.substr(i+1,end-i-1);
                if(token=="name") {result+=programName_;replaced=true;}
                for(auto p:values) if(token==p.first) {result+=p.second;replaced=true;break;}
                if(replaced) {i=end+1;continue;}
            }
            result+=value[i++];
        }
        return result;
    }
    Json available() const {
        Json result=Json::array(); const auto& english=builtins().at("catalogs").at("en").at("messages");
        for(auto it=catalogs_.begin();it!=catalogs_.end();++it) {
            auto meta=it->value("_meta",Json::object()); size_t missing=0;
            for(auto key=english.begin();key!=english.end();++key) missing+=!it->at("messages").contains(key.key());
            result.push_back({{"tag",it.key()},{"native_name",meta.value("native_name",it.key())},{"fallback_message_count",missing}});
        }
        return result;
    }
    Json summary() const {
        auto meta=catalogs_.at(effective_).value("_meta",Json::object());
        Json nameMetadata=names_.value("metadata",Json::object()).value(detected_.language,Json::object());
        return {{"program_name",programName_},{"name_preference",preferences_.value("name",std::string("auto"))},
            {"local_name_metadata",nameMetadata},
            {"detected_language",detected_.valid?Json(detected_.canonical):Json(nullptr)},{"locale_source",detectedSource_},
            {"language_well_formed",detected_.valid},{"language_known_to_cldr",detected_.knownToCLDR},
            {"os_text_direction",detected_.direction},{"ui_language_requested",requested_},{"ui_language_effective",effective_},
            {"ui_text_direction",meta.value("direction",standard.normalize(effective_).direction)},
            {"available_translations",available()},{"cldr_likely_subtag_entries",standard.likelyEntryCount()},
            {"settings_file",settings_.u8string()},{"invalid_resource_files",warnings_},{"rendering_backend","terminal"}};
    }
    void save() const {
        if(settings_.empty()) throw std::runtime_error("settings_write_failed");
        auto temp=settings_;
        temp+=".tmp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        try {
            if(!settings_.parent_path().empty()) std::filesystem::create_directories(settings_.parent_path());
            std::ofstream file(temp,std::ios::binary); file<<preferences_.dump(2)<<'\n';file.close();
            if(!file) throw std::runtime_error("settings_write_failed");
#ifdef _WIN32
            if(!MoveFileExW(temp.c_str(),settings_.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("settings_write_failed");
#else
            std::filesystem::permissions(temp,std::filesystem::perms::owner_read|std::filesystem::perms::owner_write);
            std::filesystem::rename(temp,settings_);
#endif
        } catch(const std::exception&) {std::error_code ec;std::filesystem::remove(temp,ec);throw std::runtime_error("settings_write_failed");}
    }
    void menu() {
        std::cout<<text("settings.title")<<'\n'<<text("settings.program_name")<<'\n';
        std::vector<std::string> names{"auto","EVRKare","UniFrame","XFrame","ӘмКадр"};
        auto choose=[&](const std::vector<std::string>& labels)->std::optional<size_t> {
            for(size_t i=0;i<labels.size();i++)std::cout<<i+1<<". "<<labels[i]<<'\n';
            std::cout<<text("settings.prompt");std::string input;std::getline(std::cin,input);
            if(input.empty())return std::nullopt;
            if(!digits(input)||input.size()>3)throw std::runtime_error("settings.invalid");
            auto n=std::stoul(input);if(n<1||n>labels.size())throw std::runtime_error("settings.invalid");return n-1;
        };
        auto nameLabels=names;nameLabels[0]=text("settings.auto");
        auto name=choose(nameLabels);
        std::cout<<text("settings.ui_language")<<'\n';
        std::vector<std::string> tags{"auto"},labels{text("settings.auto")};
        for(auto language:available()) {tags.push_back(language["tag"]);labels.push_back(language["native_name"]);}
        auto language=choose(labels);
        if(name)preferences_["name"]=names[*name];
        if(language)preferences_["ui_language"]=tags[*language];
        validatePreferences();resolve();save();std::cout<<text("settings.saved")<<'\n';
    }
};
inline Localizer* activeLocalizer=nullptr;
inline std::string ui(const std::string& key,const std::vector<std::pair<std::string,std::string>>& values={}) {
    if(activeLocalizer)return activeLocalizer->text(key,values);
    auto value=builtins().at("catalogs").at("en").at("messages").value(key,key);
    for(auto pair:values) {
        auto at=value.find("{"+pair.first+"}");
        if(at!=std::string::npos)value.replace(at,pair.first.size()+2,pair.second);
    }
    return value;
}
inline std::string uiError(const std::string& diagnostic) {
    auto separator=diagnostic.find('|');auto key=diagnostic.substr(0,separator);
    auto value=ui(key,{{"detail",separator==std::string::npos?"":diagnostic.substr(separator+1)}});
    return value==key?ui("error.unclassified",{{"detail",diagnostic}}):value;
}
