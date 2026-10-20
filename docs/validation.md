# Doğrulama — 20 Ekim 2026

Linux x86-64 / Debian 13 üzerinde `tools/test_all.py` tamamlandı. Fiziksel GPU
yok; Vulkan dispatch'leri Mesa Lavapipe/llvmpipe üzerinde çalıştı. Khronos ve
synchronization validation etkinleştirildi; başarılı testlerde hata raporlanmadı.

## Geçen gruplar

- Core: adaptif seçim, maliyet cooldown/recovery, kesim/refresh sınırları,
  mutlak deadline, geç ara işi düşürme, ayrı sayaçlar, eşleşmiş input timestamp
  ile input-to-submit hesabı, steady-clock callback ve geçersiz girdiler.
- Standartlar: 7.788 CLDR girdisinin tamamı; BCP 47 sözdizimi/casing,
  yaygın alias'lar, POSIX biçimleri, script ve RTL kontrolleri.
- Regresyon: yedi sahne × CPU/Vulkan/eksik sürücüde CPU fallback = 21 sahne
  kontrolü. Yatay/dikey/çapraz/sabit/düz/HUD/kesim; tüm sahnelerde PPM eşitliği.
- Modlar: altı mod CPU/Vulkan; hafif modlarda akışın atlanması; blok/yoğun
  analiz CSV boyutları; adaptif kesim/display sınırı; harici PPM ve hata yolları.
- Alt piksel (5.5, 2.5) analitik sahne: koruma kapalı iç RGB MAE blokta
  yaklaşık **0.006216**, yoğun akışta **0.002692**. CPU ve Vulkan iyileşmesi
  doğrulandı; gerçek oyun kalitesine genellenemez.
- Zamanlama modeli: zamanında tamamlanan işler varsayımıyla 100 kaynak +
  100 yeni görüntü olayı/s → 200 planlı başvuru/s, 5 ms p99. 9 ms işlem
  varsayımında geç 20 ara iş düşer; gerçek karelerin deadline'ları değişmez.
  Tekrar modunda yeni görüntü sayısı sıfır. **Bunlar simülasyondur.**
- Yerelleştirme: dil/isim kuralları, Kazakça Unicode, bağımsız kalıcılık,
  Linux öncelikleri/C/POSIX, derlemeden katalog/ad kuralı ekleme, İngilizce
  fallback, RTL metadata, menü, bozuk dosya ve bilinmeyen ayar anahtarı koruması.

`passed`, `validation_kind` ile okunmalıdır: analitik flow/görüntü, mod işlemi
veya yalnız sonlu piksel gibi farklı kontroller olabilir. Harici görüntünün
gerçek ara karesi bilinmez; kalite MAE alanları `null` kalır. Korumasız deneysel
interpolasyon testi tek başına keyfi görüntü kalitesini doğrulamaz.

## Sınırlar

Linux ikilisi çalıştırıldı. Windows ikilisi ve test programları çapraz
derlendi; Windows çalıştırması yapılmadı. macOS adaptörü kodda; macOS derleme
ve çalıştırması yapılmadı. Unicode veri yolu/RTL metadata test edildi;
tüm terminal/font/yazı sistemlerinin görsel görünümü doğrulanmadı.

RX 7800 XT ve diğer fiziksel GPU'lar, 1440p/4K çalıştırma, gerçek oyun yakalama,
DX/OpenGL entegrasyonu, ekrana sunum ve fiziksel giriş gecikmesi doğrulanmadı.
Gerçek/üretilen/görüntülenen FPS ve giriş gecikmesinin `null` kalması kasıtlıdır.
Sıradaki donanım doğrulaması canlı sahipli Vulkan test swapchain'i, gerçek
timestamp/presentation ölçümü ve RX 7800 XT kalite/gecikme matrisidir.


## Masaüstü ve sürekli akış ek doğrulaması

Yeni `tools/test_stream.py`, CPU ve Vulkan üzerinde kalıcı RGB pipe, 2x/3x/4x,
ayarlı harmanlama, gerçekten farklı ara görüntüler, aynı kaynak çiftinde yoğun
akışı tekrar kullanma, artifact fallback'i yeni kare saymama ve 8-bit kod
değerinde kaynakla aynı sonucu tekrar sayma kontrollerini içerir.

`tools/test_desktop.py` Xvfb'de gerçek X11 pencere pikselini okuyup rengini
kontrol etti; profil/tema/tüm sayfalar, motor bağlantısı, önizlemede tekrar,
Durdur ve kaynak temizliği geçti. CPU ve RSS gerçekten süreçlerden okundu.
Ham kısa örnek `results/desktop/measured-session.json` içindedir. Bu test
hedef pencereyi GUI ile aynı Python sürecinde çalıştırdığı için CPU/RSS oyun
benchmark'ı değildir. GPU utilization/VRAM/watt ve fiziksel FPS null kaldı.

`tools/test_frozen_desktop.py` derleme Python/Tcl ortam değişkenlerini kaldırıp
bağımsız Linux paketini açtı, gerçek pencereyi doğruladı ve WM_DELETE_WINDOW
ile normal kapattı. Windows motoru/başlatıcısı çapraz derlendi; Windows capture
ve GUI çalıştırılmadı. macOS capture hâlâ uygulanmadı.

## v0.2 kaynak çözünürlüğü doğrulaması

`tools/test_native_resolution.py`, 1920×1080'de piksel piksel aynı kaynak
geçişini, tekrarın yeni kare olarak sayılmamasını, harmanlama piksel değerlerini
ve hareket telafili gerçek ara kare çıktısını CPU ve yazılım Vulkan'da sınar.
321×181 tek boyutlu kenarlar dahil CPU/Vulkan ara kare eşleşmesi kontrol edilir.
Geçersiz boyutlar ve toplam payload sınırı reddedilir. Masaüstü testi gerçek
641×359 X11 yakalamanın küçülmediğini, pencere 800×450'ye değiştiğinde motorun
aynı boyuta geçtiğini ve normal duruşta iş parçacığının kapandığını sınar.

`results/native-resolution/measurements.json` tek kontrollü çalıştırmanın
native işlem ve pipe gidiş-dönüş sürelerini ayrı kaydeder. İşlem milisaniyesi
küçük olsa bile transfer/sunum ve lookahead maliyetleri sıfır değildir. Ölçüm
100→200 fiziksel FPS, AFMF eşdeğerliği veya düşük giriş gecikmesi kanıtı değildir.

Harmanlanmış görüntüler `blended_count` / `blended_completed` olarak ayrı
sayılır; `generated_count` hareket telafili yeni ara karelere ayrılmıştır.
