#include "language_tag.hpp"
#include "resources.hpp"
#include <iostream>
#include <stdexcept>
void require(bool value,const std::string& message) {if(!value)throw std::runtime_error(message);}
int main() {
    try {
        auto data=Json::parse(embeddedResources);
        LanguageStandard tags(data["aliases"],data["likely"],data["scripts"]);
        for(auto [input,expected]:std::vector<std::pair<std::string,std::string>>{
            {"TR-tr","tr-TR"},{"kaz-KZ","kk-KZ"},{"iw-IL","he-IL"},{"in-ID","id-ID"},
            {"sh-RS","sr-Latn-RS"},{"en-GB-oed","en-GB-oxendict"},{"i-klingon","tlh"},
            {"zh-hant-tw","zh-Hant-TW"},{"de-CH-1901","de-CH-1901"},
            {"en-us-u-ca-gregory-x-local","en-US-u-ca-gregory-x-local"}}) {
            auto tag=tags.normalize(input);require(tag.valid && tag.canonical==expected,input+" -> "+tag.canonical);
            require(tags.normalize(tag.canonical).canonical==tag.canonical,"Normalization must be idempotent");
        }
        require(tags.normalize("sr_RS.UTF-8@latin",true).canonical=="sr-Latn-RS","POSIX script modifier");
        require(tags.normalize("tr_TR.UTF-8",true).canonical=="tr-TR","POSIX encoding");
        for(auto bad:{"","en--US","en-","a","en-u","en-x","en-u-ca-gregory-u-nu-latn","sl-rozaj-rozaj","../../tr","not a language"})
            require(!tags.normalize(bad).valid,std::string("Reject malformed tag: ")+bad);
        require(!tags.normalize("C.UTF-8",true).valid,"C is not a detected human language");
        for(auto rtl:{"ar-SA","he-IL","fa-IR","ur-PK","ug-CN","ff-Adlm-GN","az-Arab-IR"}) require(tags.normalize(rtl).direction=="rtl",rtl);
        for(auto ltr:{"en-US","kk-KZ","zh-TW","ja-JP","ko-KR","hi-IN","az-Latn-AZ"}) require(tags.normalize(ltr).direction=="ltr",ltr);
        size_t count=0;
        auto likely=data["likely"]["supplemental"]["likelySubtags"];
        for(auto it=likely.begin();it!=likely.end();++it) {
            auto key=tags.normalize(it.key()),value=tags.normalize(it->get<std::string>());
            require(key.valid,"CLDR key invalid: "+it.key());require(value.valid,"CLDR value invalid: "+it.key());
            require(tags.normalize(key.canonical).canonical==key.canonical,"CLDR key idempotence: "+it.key());
            count++;
        }
        std::cout<<"PASS: "<<count<<" CLDR entries, BCP 47 syntax/casing, aliases, POSIX forms, scripts and RTL.\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
