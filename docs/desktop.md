# Masaüstü prototipi

Bu teslimat [ürün isteğinin](product-spec.md) ilk çalışan masaüstü/capture
aşamasıdır; tüm hedeflerin tamamlandığı kararlı sürüm değildir. Referans ekran
görüntüsü yüklenmediğinden tarifteki iki sütunlu yerleşimle özgün kart tasarımı
oluşturuldu. Ticari varlık/kod kopyalanmadı.

## Kullanım ve gerçek işlem yolu

Profil → hedef pencere → mod → Başlat. Ayrı araştırma önizlemesi açılır.
Durdur capture thread'ini, native motoru ve önizlemeyi kapatır. Profiller
oluşturulur/kopyalanır/düzenlenir/silinir; varsayılan silinemez. Son profil,
tema/vurgu, basit/gelişmiş görünüm ve pencere başlığı eşlemesi atomik desktop.json
içinde saklanır. Exe/process taraması veya oyun başlatma henüz yoktur.

| Capture yolu | Durum |
|---|---|
| Linux X11 XGetImage | Xvfb'de gerçek görünür Tk penceresinde doğrulandı |
| XWayland | DISPLAY ve görünür X11 yüzeyi gerekir; ayrıca doğrulanmadı |
| Yerel Wayland / PipeWire portal | Uygulanmadı |
| Windows GDI BitBlt | Kodda, gerçek Windows'ta çalıştırılmadı; WGC değildir |
| WGC / DXGI duplication | Uygulanmadı |
| macOS ScreenCaptureKit / Metal | Uygulanmadı |

X11 yolu 32-bit little-endian BGRX TrueColor ister. Hedef görünür olmalı;
örtülü X11 bölgeleri güvenilir değildir. Kapalı/küçültülmüş/çok büyük pencere
hata verir. GPU oyun yüzeyleri ve exclusive fullscreen doğrulanmadı. Siyah
capture destek kanıtı değildir; korumalı içerik/anti-cheat atlatılmaz.

Görüntü kaynak pencerenin **kendi çözünürlüğünde** işlenir; yakalama ile motor
arasında küçültme/büyütme yoktur. Önizleme sonradan pencereye sığdırılabilir.
Ölçekleme kartı yalnız bu görüntüleme adımının nearest/bilinear yöntemini seçer.
Pencere yeniden boyutlanırsa işçi yeni boyutla yeniden açılır; eski boyutun
kaynak geçmişi atılır. Sınırlar: kenar başına 4096, en fazla 8.294.400 piksel,
çarpan × piksel en fazla 16.777.216. Sınır aşılırsa hata gösterilir; çözünürlük
sessizce düşürülmez. GPU storage buffer sınırı da kontrol edilir.
Oyunun kendi sunumuna kare eklenmez.

## Modlar, kaynaklar ve zamanlama

Native motor `--stream` ile oturum boyunca açık kalır. Yerel JSON+RGB pipe
bir ağ servisi değildir. Cihaz, pipeline, tampon, descriptor/fence tekrar
kullanılır. 3x/4x'te aynı kaynak çiftinin akışı bir kez hesaplanır; diğer i/N
zamanları ayrı warp'lardır. Harmanlama ağırlığı ayarlanır. B'yi bekleyen
interpolasyonun ek lookahead süresi vardır.

Tekrar modunda tek RGB görüntü taşınır; UI nesneyi N slotta yeniden kullanır.
Kaynakla aynı **8-bit RGB**'ye düşen sonuç yeni görüntü sayılmaz. Fallback ve
alt-kod-değeri değişimler tekrar olarak sayılır. Görüntü kopyası yeni hareket
olarak pazarlanmaz.

Capture/işleme ayrı thread'dedir. Kuyruk en fazla bir güncel gruptur. Yeni
grupta eski bekleyen işler ve geç sentetik kareler düşer. Tk callback'leri
hard real-time/VSync/scanout değildir. Boşta capture/motor yoktur; seyrek UI
kontrolü ve 1 Hz uygulama ölçümü vardır.

Eco/Balanced/Performance/Quality önayarları gerçek capture limitlerini ve
kalite/arama maliyetlerini değiştirir. Power Auto şu an Balanced varsayılanıdır.
Arama seçenekleri farklı gerçek yarıçaplardır; AMD algoritmaları değildir.
Watt tasarrufu, GPU load tabanlı güç kontrolü veya özel APU termal yolu yoktur.

## Ölçüm anlamları

| Alan | Ölçülen |
|---|---|
| capture_fps | Başarılı pencere örnekleri/s; oyun render FPS'si değil |
| preview_submission_fps | Tk görüntü güncelleme başvuruları/s; scanout değil |
| generated_completed | Kaynaklardan farklı tamamlanan RGB görüntüler |
| repeated_submitted | Önizlemede tekrar başvuruları |
| processing_ms | Native işleme; capture/sunum/giriş gecikmesi değil |
| cpu_core_percent | GUI + motor; bir çekirdek %100 |
| process_tree_rss_sum_bytes | Süreç RSS toplamı; benzersiz fiziksel RAM değil |
| Oyun FPS / fiziksel FPS / input latency | null, ölçülmedi |
| GPU utilization / VRAM / watt | null, ölçülmedi |

Ham kısa test `results/desktop/measured-session.json` içindedir. Hedef pencere
GUI ile aynı test Python sürecindedir; motor ayrı süreçtir. Xvfb sunucusu
toplama dahil değildir. Bu, oyun benchmark'ı veya kararlı güç ölçümü değildir.

## İsimler ve kimlik

names.json yeni ürün tablosunu içerir. Verilen öneriler otomatik kurallara
eklendi; dilsel doğrulanmamış adlar `user_proposed_unverified` olarak saklanır.
Tam çeviri olarak sunulmaz. Bilinmeyen native ad/tam açılım alanları null'dır.
Türk Dünyası sayfası dil/ad/script/çeviri/öneri durumlarını gösterir; konumdan
etnik kimlik çıkarımı veya kapsamlı siyasi/coğrafi envanter değildir.
Hakkında bölümünde istenen Türk geliştirici proje açıklaması TR/EN vardır.

Turan görseli verilmediği için uydurulmadı. PNG/JPEG dosya seçicisi ve
Otomatik/Açık/Kapalı vardır. Otomatik, arayüz/sistem dil grubuna bağlıdır;
resmî ülke bayrakları yerine kullanılmaz.

## Dağıtım ve kalan işler

Linux frozen paket Python/Tk/Pillow/psutil ve motor içerir. Derleme ortamının
Python/Tcl değişkenleri kaldırılarak gerçek X11 penceresi açıldı ve WM protocol
ile normal kapandı. Diğer dağıtımlar/fiziksel oyunlar doğrulanmadı.

Windows EVRKare.exe çapraz derlenmiş **başlatıcıdır**, Python içermez.
Python 3.10+/Tk ve Install-desktop.cmd gerekir. Native UniFrame.exe ayrıca
dahildir. Windows GUI/capture çalıştırılmadı. Bağımsız Windows GUI paketi
Windows üzerinde tools/freeze_desktop.py ile yapılmalıdır. macOS capture/paket yoktur.

Kalanlar: WGC/Wayland portal/macOS capture, yüksek çözünürlüklü device-local
GPU yolu, oyun/swapchain entegrasyonu, gerçek HDR/VRR, temporal tracking,
gelişmiş occlusion/HUD ayrımı, GPU/VRAM/güç sensörleri, özel APU/multi-GPU ve
fiziksel AMD/NVIDIA/Intel/Apple Silicon testleri. AFMF 2.1 eşdeğerliği veya
100→200 fiziksel görüntüleme doğrulanmadı. Tamamlanmamış seçenekler etkin
bir özellik gibi sunulmaz.

## v0.1.1 kullanım akışı

Pencereyi açın, Ana Sayfa > Yakalama kaynağı listesinden seçin; listede yoksa
Pencereleri yenile düğmesine basın. Kare oluşturma modunu ve çarpanı seçip
Önizlemeyi başlat düğmesine basın. Uygulama odaktayken Ctrl+Alt+S aynı işlemi
yapar. Ayrı önizlemeyi büyütebilir veya F11 ile tam ekrana geçebilirsiniz;
Esc tam ekrandan çıkar. Önizlemeyi kapatmak yakalamayı durdurur. Kaynak pencere
pikselleri görünür olmalıdır; bu sürüm oyun motoruna bağlanmaz.

## v0.2 modları ve süreler

Basit menüde Tekrar Kare, Karışık Kare (Harmanlama), Hareket Telafili Ara Kare
ve Otomatik Ara Kare + Tekrar ayrı seçimlerdir. Karışık Kare seçildiğinde ağırlık
ayarı basit görünümde de açılır. Tekrar kare yeni hareket üretmez; harmanlama
optik akış değildir ve gölgelenme yapabilir. AMD'nin özel algoritmalarının
birebir kopyası olduğu iddia edilmez.

Tekrar yolu doğrudan RGB baytlarını geçirir; CPU harmanlama yolu float RGBA
kareleri veya GPU aktarımı oluşturmadan çalışır. Bu modlarda işlem yükü azalır,
fakat yakalama, pipe aktarımı ve Tk sunumu maliyetleri sürer. Tekrar modunda
Vulkan seçilse bile kopyalama CPU'da yapılır ve backend CPU olarak raporlanır.

Performans sayfası kaynak boyutunu, yakalama+işlem+aktarımı ve iki kaynak kare
arasındaki lookahead süresini ayrı gösterir. Bunlar input-to-photon ölçümü değildir.
İnterpolasyon sonraki kaynak kareyi bekler; üretilen kareler veya tekrarlar tek
başına giriş gecikmesini azaltmaz. Fiziksel GPU'da düşük milisaniye hedefi
henüz doğrulanmadı; yazılım Vulkan/CPU Full HD interpolasyonu gerçek zamanlı
oyun performansı sağlamaz.

Harmanlanmış görüntüler `blended_count` / `blended_completed` olarak ayrı
sayılır; `generated_count` hareket telafili yeni ara karelere ayrılmıştır.
