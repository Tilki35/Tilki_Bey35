#pragma once
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using Json=nlohmann::json;
inline std::string lower(std::string s) { for(char& c:s) c=char(std::tolower(static_cast<unsigned char>(c))); return s; }
inline std::string upper(std::string s) { for(char& c:s) c=char(std::toupper(static_cast<unsigned char>(c))); return s; }
inline std::vector<std::string> split(const std::string& s,char delimiter) {
    std::vector<std::string> result; std::istringstream in(s); std::string item;
    while(std::getline(in,item,delimiter)) result.push_back(item);
    if(!s.empty() && s.back()==delimiter) result.push_back("");
    return result;
}
inline bool letters(const std::string& s) { return !s.empty() && std::all_of(s.begin(),s.end(),[](char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z');}); }
inline bool digits(const std::string& s) { return !s.empty() && std::all_of(s.begin(),s.end(),[](char c){return c>='0'&&c<='9';}); }
inline bool alphanumeric(const std::string& s) { return !s.empty() && std::all_of(s.begin(),s.end(),[](char c){return letters(std::string(1,c))||digits(std::string(1,c));}); }
struct LanguageTag {
    bool valid=false,knownToCLDR=false;
    std::string canonical,language,script,region,direction="ltr";
    std::vector<std::string> extlangs,variants,extensions;
    void assemble() {
        canonical=language;
        for(auto s:extlangs) canonical+="-"+s;
        if(!script.empty()) canonical+="-"+script;
        if(!region.empty()) canonical+="-"+region;
        for(auto s:variants) canonical+="-"+s;
        for(auto s:extensions) canonical+="-"+s;
    }
};
class LanguageStandard {
    Json aliases_,likely_,scripts_;
    static LanguageTag syntax(std::string s) {
        LanguageTag t;
        if(s.empty() || s.size()>255) return t;
        auto parts=split(s,'-');
        if(std::any_of(parts.begin(),parts.end(),[](const auto& p){return p.empty()||p.size()>8||!alphanumeric(p);})) return t;
        size_t i=0;
        if(lower(parts[0])=="x") {
            if(parts.size()<2) return t;
            t.valid=true; t.canonical=lower(s); return t;
        }
        if(parts[0].size()<2 || !letters(parts[0])) return t;
        t.language=lower(parts[i++]);
        if(t.language.size()<=3) while(i<parts.size() && parts[i].size()==3 && letters(parts[i]) && t.extlangs.size()<3) t.extlangs.push_back(lower(parts[i++]));
        if(i<parts.size() && parts[i].size()==4 && letters(parts[i])) {
            t.script=lower(parts[i++]); t.script[0]=char(std::toupper(t.script[0]));
        }
        if(i<parts.size() && ((parts[i].size()==2 && letters(parts[i]))||(parts[i].size()==3 && digits(parts[i])))) t.region=upper(parts[i++]);
        std::set<std::string> variants,singletons;
        while(i<parts.size() && (parts[i].size()>=5 || (parts[i].size()==4 && digits(parts[i].substr(0,1))))) {
            auto v=lower(parts[i++]); if(!variants.insert(v).second) return LanguageTag{}; t.variants.push_back(v);
        }
        while(i<parts.size()) {
            auto singleton=lower(parts[i++]);
            if(singleton.size()!=1 || !singletons.insert(singleton).second) return LanguageTag{};
            t.extensions.push_back(singleton); size_t start=i;
            if(singleton=="x") {
                while(i<parts.size()) t.extensions.push_back(lower(parts[i++]));
            } else while(i<parts.size() && parts[i].size()>=2) t.extensions.push_back(lower(parts[i++]));
            if(i==start) return LanguageTag{};
        }
        t.valid=true; t.assemble(); return t;
    }
    std::string alias(const std::string& kind,const std::string& key) const {
        const auto& group=aliases_.at(kind);
        auto it=group.find(key);
        return it==group.end()?"":it->at("_replacement").get<std::string>();
    }
public:
    LanguageStandard(const Json& aliases,const Json& likely,const Json& scripts)
        :aliases_(aliases.at("supplemental").at("metadata").at("alias")),
         likely_(likely.at("supplemental").at("likelySubtags")),scripts_(scripts.at("scriptMetadata")) {
        Json normalized=Json::object();
        for(auto it=aliases_["languageAlias"].begin();it!=aliases_["languageAlias"].end();++it) normalized[lower(it.key())]=it.value();
        aliases_["languageAlias"]=std::move(normalized);
    }
    size_t likelyEntryCount() const { return likely_.size(); }
    LanguageTag normalize(std::string s,bool posix=false) const {
        if(s.empty() || s.size()>255) return {};
        std::string modifier;
        if(posix) {
            auto at=s.find('@'); if(at!=std::string::npos) {modifier=lower(s.substr(at+1));s.resize(at);}
            auto dot=s.find('.'); if(dot!=std::string::npos) s.resize(dot);
            std::replace(s.begin(),s.end(),'_','-');
            if(lower(s)=="c" || lower(s)=="posix") return {};
        }
        // CLDR includes grandfathered and deprecated tags as well as overlong codes.
        for(int n=0;n<4;n++) {
            auto replacement=alias("languageAlias",lower(s));
            if(replacement.empty() || lower(replacement)==lower(s)) break;
            s=split(replacement,' ')[0];
        }
        auto t=syntax(s); if(!t.valid || t.language.empty()) return t;
        auto replacement=alias("languageAlias",t.language);
        if(!replacement.empty()) {
            auto target=syntax(split(replacement,' ')[0]);
            if(target.valid) {
                t.language=target.language;
                if(t.script.empty()) t.script=target.script;
                if(t.region.empty()) t.region=target.region;
            }
        }
        if(!t.extlangs.empty()) {
            auto r=alias("languageAlias",t.language+"-"+t.extlangs[0]);
            if(!r.empty()) {
                auto target=syntax(split(r,' ')[0]);
                if(target.valid) { t.language=target.language; t.extlangs.erase(t.extlangs.begin()); }
            }
        }
        if(posix && t.script.empty() && (modifier=="latin"||modifier=="cyrillic")) t.script=modifier=="latin"?"Latn":"Cyrl";
        if(!t.script.empty()) { auto r=alias("scriptAlias",t.script); if(!r.empty()) t.script=r; }
        if(!t.region.empty()) {
            auto r=alias("territoryAlias",t.region);
            if(!r.empty()) {
                auto targets=split(r,' '); t.region=targets[0];
                auto likely=likely_.find(t.language);
                if(likely!=likely_.end()) {
                    auto region=syntax(likely->get<std::string>()).region;
                    if(std::find(targets.begin(),targets.end(),region)!=targets.end()) t.region=region;
                }
            }
        }
        for(auto& v:t.variants) {auto r=alias("variantAlias",v);if(!r.empty())v=lower(r);}
        t.assemble(); t.knownToCLDR=likely_.contains(t.language);
        std::string script=t.script;
        if(script.empty()) {
            std::vector<std::string> keys;
            if(!t.region.empty()) keys.push_back(t.language+"-"+t.region);
            keys.push_back(t.language);
            if(!t.region.empty()) keys.push_back("und-"+t.region);
            keys.push_back("und");
            for(const auto& key:keys) {
                auto it=likely_.find(key);
                if(it!=likely_.end()) { script=syntax(it->get<std::string>()).script; break; }
            }
        }
        auto it=scripts_.find(script);
        t.direction=it!=scripts_.end() && it->value("rtl",std::string("NO"))=="YES"?"rtl":"ltr";
        return t;
    }
};
