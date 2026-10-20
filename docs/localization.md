# Dil ve isimlendirme

EVRKare Türkçe resmî isim, UniFrame uluslararası isim, XFrame geliştirme kod
adıdır. Otomatik ad **sistem diline** göre seçilir; manuel arayüz dili bu adı
değiştirmez. Manuel ad tercihi otomatik kuraldan önce gelir.

## Kurallar ve standartlar

`locales/names.json` yeni ürün isteğiyle güncellendi: tr → EVRKare,
az → KainatKadr, uz → OlamKadr, tk → ÄlemKadr, ky → АаламКадр, kk → ӘмКадр.
Diğer verilen Türk/dünya dilleri önerileri metadata ile saklanır. Örneğin
de → UniversalBild, fr → UniImage. Bunlar kullanıcı tarafından istenen marka
önerileridir; `user_proposed_unverified` dilsel doğrulama değildir.
Diğer iyi biçimlenmiş dil etiketleri → UniFrame. Dil algılanamazsa, C/POSIX,
und/zxx veya yalnız private-use etiketi varsa → XFrame. Önce tam etiket, sonra
temel dil kuralı aranır. Örneğin gelecekte kk-Latn için ayrı doğal ad eklenebilir.

`src/language_tag.hpp`, BCP 47 language/extlang/script/region/variant/extension/
private-use sözdizimini işler. Casing ve CLDR takma adları normalleştirilir;
bozuk etiketler, tekrar eden variant/singleton reddedilir. CLDR 48'in alias,
scriptMetadata ve 7.788 likely-subtag girdisi kullanılır. Bu sayı dil sayısı
değildir; bölge/yazı sistemi bileşimleri dahildir. Tüm girdiler test edildi.

Örnekler: TR-tr → tr-TR, kaz-KZ → kk-KZ, iw-IL → he-IL,
sh-RS → sr-Latn-RS, en-GB-oed → en-GB-oxendict.
OS girişinde tr_TR.UTF-8 ve sr_RS@latin gibi POSIX biçimleri de işlenir.

Bu, IANA Language Subtag Registry'nin eksiksiz semantik doğrulayıcısı değildir;
Unicode extension anahtarlarına özel tam kanonikleştirme yoktur. Sözdizimsel
olarak geçerli, CLDR'de bulunmayan/gelecekteki etiketler reddedilmez.
`language_known_to_cldr` ile `language_well_formed` ayrı raporlanır.
Kaynak commit'leri, SHA-256 ve lisanslar `third_party/sources.json` içindedir.

## İşletim sistemleri

- Windows: GetUserPreferredUILanguages, ardından GetUserDefaultLocaleName.
  CommandLineToArgvW ile Unicode argümanlar; UTF-8 konsol; wide profil yolu API'si.
- Linux: etkin locale için LC_ALL → LC_MESSAGES → LANG. Etkin locale C/POSIX
  değilse LANGUAGE listesindeki ilk geçerli dil GNU mesaj önceliğine uygun seçilir.
- macOS: CoreFoundation CFLocaleCopyPreferredLanguages; gerekirse ortam
  değişkenleri. Adaptör kodda vardır, macOS çalıştırma testi yapılmadı.
- `--locale TAG` geçici algılama override'ıdır; kalıcı arayüz tercihi değildir.

## Ayarlar

`--settings-menu` CLI terminal menüsüdür; ayrıca masaüstü arayüzü vardır. Program adı
seçenekleri: Otomatik, EVRKare, UniFrame, XFrame, ӘмКадр. Arayüz dili seçenekleri
Otomatik ve gerçek çeviri kataloglarıdır. İki tercih bağımsızdır.
`--settings` durum JSON'unu, `--list-languages` mevcut çevirileri verir.

```bash
./uniframe --name EVRKare --ui-language en --settings
./uniframe --name auto --ui-language auto --settings
```

Kalıcı dosya: Windows LOCALAPPDATA/UniFrame/settings.json (yedek: APPDATA),
Linux mutlak XDG_CONFIG_HOME/uniframe/settings.json veya ~/.config/uniframe/
settings.json, macOS ~/Library/Application Support/UniFrame/settings.json.
`--config FILE` yolu değiştirir.

Tercih değişiminde atomik replace yapılır; normal açılış/kare işleme ayarları
yeniden yazmaz. Bilinmeyen ayar anahtarları korunur. Bozuk dosya sessizce
ezilmez; değiştirmek için açık `--reset-settings` gerekir.

## Derlemeden çeviri ekleme

Çalıştırılabilir dosyanın yanındaki `locales/` içine örneğin `de-DE.json` koy:

```json
{
  "_meta":{"language":"de-DE","native_name":"Deutsch","direction":"ltr"},
  "messages":{"settings.program_name":"Programmname"}
}
```

Başka dizin için `--resources DIRECTORY`. En özel etiketten temel dile doğru
lookup yapılır. Her eksik anahtar gömülü İngilizceye döner. Başlangıçta yalnız
**Türkçe ve İngilizce** çeviri vardır. `fallback_message_count` eksik anahtar
sayısını gösterir; tanınan her dil çevrilmiş gibi sunulmaz. Geçersiz kaynak
dosyaları durum çıktısında bildirilir ve kullanılmaz. Doğal yerel adlar aynı
dizindeki names.json ile kod değiştirmeden düzenlenir.

Kullanıcıya gösterilen metin ve hata sözlükleri JSON'dadır. CLI seçenekleri,
JSON/CSV alan adları ve API isimleri sabit makine arayüzleridir. Parametreler
tek geçişte açılır; dosya/ad içindeki süslü parantezler tekrar yorumlanmaz.

## Unicode, RTL ve performans

UTF-8 veri yolu Latin, Kiril, Arap, CJK, Devanagari ve diğer Unicode metinleri
saklar/aktarır. CLDR script metadata yazı yönünü belirler. `os_text_direction`
ile kullanılan çevirinin `ui_text_direction` alanı ayrıdır: Arapça sistemde
çeviri yoksa İngilizce arayüz LTR kalır. RTL katalog eklenince arayüz yön
bilgisi RTL olur.

CLI font/bidi motoru sağlamaz; masaüstü Tk üzerinde ana panel yönünü metadata
ile seçer. Görsel doğruluk terminal/Tk/platforma bağlıdır. Unicode veri yolu ve RTL metadata
test edildi; tüm yazı sistemleri/platformlarda görsel doğrulama yapılmadı.

Dil/isim seçimi, CLDR ve dosya erişimi yalnız başlangıçta/ayar değişimindedir.
Shader ve hareket analizi döngülerine locale sorgusu eklenmez. Tanılama metni
kare süresi ölçümünden sonra biçimlenir. Başlangıç parsing/bellek maliyeti vardır.
