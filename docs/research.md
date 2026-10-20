# Kamuya açık kaynak araştırması — 20 Ekim 2026

## AFMF 2.1 erişim sınırı

[AMD AFMF ürün sayfası](https://www.amd.com/en/products/software/adrenalin/afmf.html)
ve [25.3.1 sürüm notları](https://www.amd.com/en/resources/support-articles/release-notes/RN-RAD-WIN-25-3-1.html)
bu ortamın egress proxy'sinde 403 ile engellendi. İçerik okunamadığından AFMF
2.1'e özel algoritma, hız veya uyumluluk iddiaları bu oturumda birincil
kaynaktan doğrulanmış sayılmaz. www.amd.com ve amd.com ağ eklemeleri ortam
taslağına kaydedildi; bu, erişimin çalışan makineye uygulandığı anlamına gelmez.
Ortam ayarları kaydedilip yayımlandıktan sonra erişim yeniden denenmelidir.

## Okunabilen resmî AMD kaynakları

GPUOpen FidelityFX/FSR SDK commit'i:
`60f4ea81909200d8542eca14dccb2628b763a9a3`.

- [Frame Interpolation](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/FidelityFX/docs/techniques/frame-interpolation.md):
  iki ardışık görüntüden ara görüntü hesaplanması ile bunun swapchain'e
  sunulmasını ayrı sorunlar olarak ele alır.
- [Frame Generation Swapchain](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/blob/60f4ea81909200d8542eca14dccb2628b763a9a3/Kits/FidelityFX/docs/techniques/frame-interpolation-swap-chain.md):
  UI/HUD kenarları, GPU bitişini bekleme, hareketli kare zamanı ortalaması,
  sunum zamanlaması ve fazla kuyruklanmanın gecikmeye etkisini açıklar.
  UI Composition, Frame pacing and presentation, FAQ bölümleri incelendi.

**Bunlar FSR belgeleridir; AFMF 2.1'in uygulama ayrıntıları değildir.** Motor
verisiyle çalışan bir entegrasyonla motorsuz yakalama eşdeğer sayılamaz.
SDK kodu kopyalanmadı; belgeler genel mimari kararları değerlendirmek için okundu.

## Bağımsız uygulama kararlarımız

Kare tekrarı yeni ara görüntü sayılmaz. Yoğun Lucas–Kanade, bizim açık ve basit
prototip seçimimizdir; AMD'nin bu algoritmayı kullandığı iddia edilmez.
Adaptif eşikler de AMD'den alınmış değildir. Maliyet/refresh/kesim ve warp
reddi vekil oranına göre üretim/tekrar/geçiş seçilir. Geç sentetik işler düşer;
sonraki gerçek karelerin planlanan zamanları değiştirilmez. Canlı swapchain
bağlantısı henüz yoktur. Sunum başvurusu, scanout ve input→photon ayrı ölçümlerdir.

## Yerelleştirme kaynakları

[Unicode CLDR JSON](https://github.com/unicode-org/cldr-json/tree/91c267402229a59e3ef2774544f001bf959e8809)
içinden CLDR 48 alias/likelySubtags/scriptMetadata dosyaları kullanıldı.
[BCP 47 / RFC 5646](https://www.rfc-editor.org/rfc/rfc5646) standart referansıdır;
bu RFC sayfasının bu oturumda çevrimiçi okunduğu iddia edilmez.
[nlohmann/json 3.12.0](https://github.com/nlohmann/json/tree/55f93686c01528224f448c19128836e7df245f72)
JSON ayrıştırıcısıdır. Commit URL'leri, SHA-256 ve lisanslar depodadır.
