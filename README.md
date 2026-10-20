# EVRKare / UniFrame / XFrame

Aynı yazılımın Türkçe resmî adı **EVRKare**, uluslararası adı **UniFrame**,
geliştirme kod adı **XFrame**'dir. Kazakça başlangıç adı **ӘмКадр**'dır.

Bu sürüm, Linux X11 üzerinde gerçek pencere yakalama ve ayrı işleme önizlemesi bulunan **araştırma prototipidir**.
Windows x64 dosyası çapraz derlenir; Windows ve fiziksel GPU üzerinde çalışma
henüz doğrulanmadı. macOS için dil algılama adaptörü ve kaynak derleme yolu
vardır; macOS ikilisi üretilmedi ve macOS çalıştırma testi yapılmadı.
X11 pencere pikselleri yakalanır; Windows GDI adaptörü kodda vardır ancak Windows üzerinde test edilmedi. Windows Graphics Capture, oyunlara bağlanan Vulkan layer, DirectX/OpenGL/Metal entegrasyonu ve oyunun kendi sunumuna ek kare yerleştirme henüz uygulanmadı. AFMF 2.1 performansına veya
100 gerçek → 200 görüntülenen FPS hedefine ulaşıldığı iddia edilmez.

## Hazır paketleri indir

- [Linux masaüstü paketi](downloads/UniFrame-Desktop-linux-x64.tar.gz): arşivi açıp `UniFrame-Desktop/UniFrame-Desktop` dosyasını çalıştır.
- [Windows x64 paketi](downloads/UniFrame-prototype-windows-x64.zip): `EVRKare.exe` arayüz başlatıcısı Python/Tk gerektirir; `Install-desktop.cmd` bağımlılıkları kurar. `UniFrame.exe` komut satırı motorudur. Windows üzerinde henüz test edilmedi.
- [Paket SHA-256 değerleri](downloads/SHA256SUMS.txt).

GitHub dosya sayfasında **Download raw** düğmesiyle arşivi indirebilirsin.

## İndirilebilir yerel çıktılar

- Windows: `dist/UniFrame-prototype-windows-x64.zip` içindeki `UniFrame.exe`.
- Linux: `dist/UniFrame-prototype-linux-x64.tar.gz` içindeki `uniframe`.
- `dist/SHA256SUMS.txt` arşiv ve ikili dosya hash'lerini içerir.
- Eski `tilki-fg` / `tilki-fg.exe` dosya adları uyumluluk için korunur.

Linux paketi bu Debian 13 x86-64 ortamında test edildi. glibc 2.35+,
GLIBCXX_3.4.32 sağlayan libstdc++, libgcc ve libm gerekir. Diğer dağıtımlar
ayrıca sınanmalıdır; eski sistemlerde kaynak derlemesi gerekebilir.
Windows geliştirme hedefi Windows 10/11 x64'tür. Vulkan yolu loader ve uygun
sürücü gerektirir; CPU yolu Vulkan gerektirmez. SPIR-V ve varsayılan dil
kaynakları ikiliye gömülüdür; son kullanıcı shader derleyicisi kurmaz.

## Modlar

| Teknoloji | Seçenek | Bugünkü davranış |
|---|---|---|
| Frame Repetition | `--mode repeat` | A karesini aynen tekrarlar; yeni görüntü üretmez |
| Frame Blending | `--mode blend` | A ve B'yi %50 karıştırır; hareket telafisi yoktur, gölgelenme yapabilir |
| Frame Interpolation | `--mode interpolate` | Hareketle hizalanmış t=0.5 ara kare |
| Motion Estimation | `--mode motion` | İleri/geri blok vektörlerini CSV'ye yazar; görüntü çıktısı B'dir |
| Optical Flow | `--mode optical-flow` | Yoğun Lucas–Kanade akışını CSV'ye yazar; görüntü çıktısı B'dir |
| Optical Flow + Interpolation | `--mode interpolate --estimator optical-flow` | Blok tohumu + yoğun alt piksel iyileştirme + warp |
| Adaptive Generation | `--mode adaptive` | Sahne kesimi, refresh sınırı, önceki maliyet ve eşleşme güvenine göre üretim/tekrar/geçiş |
| Artifact Reduction | `--artifact-reduction` (`on`, `off`) | Ara kare üretimine eklenebilen koruma; varsayılan açık |
| Frame Pacing | `--pacing` (`on`, `off`) + `--pacing-demo` | Bağımsız son tarih zamanlayıcısının simülasyon kaydı; ekrana sunum değildir |

Tekrar/karıştırma modları optik akış hesaplamaz. `auto` backend bu hafif
modlarda CPU'yu seçer; `--backend vulkan` açıkça seçilirse Vulkan kullanılır.
Diğer modlarda `auto`, Vulkan başlatılamazsa CPU'ya döner. `vulkan` seçeneği
Vulkan yokken hata verir, testleri gizlice CPU'ya geçirmez.

```bash
./uniframe --mode adaptive --backend auto --output results
./uniframe --mode interpolate --estimator optical-flow --output results-flow
./uniframe --mode optical-flow --scene fractional --output flow-analysis
./uniframe --mode interpolate --pacing-demo --source-fps 100 --display-hz 200
```

Windows terminalinde aynı seçeneklerle `UniFrame.exe` kullanılır.
`--help` mevcut seçenekleri gösterir. `--source-fps` ve `--display-hz` **ayar**
değerleridir, ölçüm değildir. `--simulated-processing-ms` yalnız zamanlama
simülasyonunun maliyet varsayımıdır. Gerçek zamanlı FPS veya input-to-photon
ölçümü olarak kullanılamaz.

## Dil, isim ve ayarlar

```bash
./uniframe --settings
./uniframe --settings-menu
./uniframe --name EVRKare --ui-language en --settings
./uniframe --name auto --ui-language auto --settings
./uniframe --locale kk-KZ --settings
```

Program adı ve arayüz dili bağımsız olarak kaydedilir. `--locale` yalnız o
çalıştırmada sistem dili algılamasını geçersiz kılar, kalıcı arayüz tercihi değildir.
Bu seçenekler CLI terminal menüsüdür. Ayrıca profil, tema ve bağımsız ad/dil ayarları içeren masaüstü arayüzü vardır; aşağıdaki Masaüstü bölümüne bak.

Unicode CLDR 48 verileriyle dil/bölge etiketleri, takma adlar ve yazı yönü
çözümlenir. Varsayılan çeviriler yalnız **Türkçe ve İngilizce**'dir. Diğer
sistem dillerinin tanınması tam arayüz çevirisi olduğu anlamına gelmez.
Eksik anahtarlar gömülü İngilizceye döner. UTF-8 metin ve RTL yön bilgisi vardır;
terminalin font kapsamı, şekillendirme ve bidi yerleşimi uygulama tarafından
sağlanmaz. [Yerelleştirme ayrıntıları](docs/localization.md).

Yeni bir çeviri eklemek için paketteki `locales/` içine BCP 47 koduyla adlandırılan
bir JSON koymak yeterlidir; kaynak kodu değiştirmek veya derlemek gerekmez.
Dil başına doğal program adları `locales/names.json` üzerinden değiştirilebilir.

## Girdi ve çıktı

Varsayılan çalıştırma yedi sentetik sahneyi test eder. `--scene fractional`,
alt piksel hareketini ayrıca sınar. Kendi kare çiftin için:

```bash
./uniframe --input-a A.ppm --input-b B.ppm --mode interpolate --output results
```

Girişler şimdilik **256×144 P6 PPM, maxval 255** olmalıdır. Bu, yüksek
çözünürlüklü canlı oyun işleme değildir. Çıktılar PPM, analiz modlarında akış
CSV'si ve `metrics.json`'dır. Harici girdide gerçek ara görüntü bilinmediğinden
MAE alanları `null` kalır; yalnız sonlu piksel ve seçilen modun işlemi doğrulanır.

`metrics.json` duvar süresi ve destekleniyorsa Vulkan timestamp süresini ayrı
raporlar. Başlatma/JIT maliyeti, cihaz tipi, seçilen mod ve karar nedeni görünür.
Ölçülmeyen gerçek/üretilen/görüntülenen FPS, sunum aralığı ve giriş gecikmesi
`null` kalır. `timeline.json` içindeki hızlar ve aralıklar açıkça **simulated**
olarak işaretlidir. Kare tekrarı üretilmiş yeni görüntü sayısına eklenmez.

## Netlik ve performans sınırları

Değişmeden kalan pikseller, sabit HUD kenarları dahil, aynen korunur.
Kaynaklar hareketle hizalandıktan sonra kanal başına 1/255'ten fazla ayrışıyorsa,
akış tutarsızsa veya örnek görüntü dışına taşıyorsa yeni gerçek kareye dönülür.
Bu politika akıcılıktan vazgeçebilir; sıfır bozulma garantisi değildir.
Saydamlık, örtülme, tekrar eden desenler ve hızlı hareket için daha geniş
veri seti gerekir. HDR/renk yönetimi ve çok ölçekli akış henüz yoktur.

Marka kilidi yoktur: uygun Vulkan 1.1 compute, bellek türü ve sürücüye sahip
AMD/NVIDIA/Intel cihazları adaydır; ailelerin tamamı test edilmemiştir.
Eski donanım için CPU tekrar/karıştırma yolu hafiftir. CPU optik akış gerçek
zamanlı hız garantisi vermez; 1980'ler donanımı veya eski işletim sistemlerinde
çalışma sözü yoktur. RX 7800 XT optimizasyonu fiziksel kart ölçümü bekler.

## Derleme ve doğrulama

C++17, Python 3.8+, Vulkan başlıkları, glslangValidator ve spirv-val gerekir.
Depoya sabitlenmiş nlohmann/json ve CLDR için build sırasında SHA-256 doğrulanır;
native motor çalıştırmada İnternet veya paket kurulumu gerektirmez. Kaynaktan masaüstü arayüzü ayrıca Tk/Pillow/psutil ister; Linux frozen paketi bunları içerir. Üçüncü taraf lisansları
`third_party/` ve `locales/cldr/LICENSE` içindedir.

Bu Debian 13 bulut makinesinde:

```bash
bash tools/cloud-bootstrap.sh
source /workspace/toolchain/env.sh
python3 tools/build.py
python3 tools/build.py --windows
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation \
VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT \
python3 tools/test_all.py
python3 tools/package.py
```

Windows kaynak derlemesi için MinGW-w64 ile `python tools/build.py --windows`
kullanılır. macOS kaynak yolu CoreFoundation ile linklenir; Vulkan loader yoksa
CPU yolu seçilebilir. macOS yolu bu makinede derlenip çalıştırılmadı.

[Mimari](docs/architecture.md) · [Kaynak araştırması](docs/research.md) ·
[Test sonuçları ve sınırlamalar](docs/validation.md)

## Masaüstü uygulaması

Linux için `dist/UniFrame-Desktop-linux-x64.tar.gz` paketini açıp içindeki
`UniFrame-Desktop` dosyasını çalıştır. Paket Python/Tk ve native motoru içerir;
bu makinede Xvfb/X11 üzerinde gerçek pencere açılışı ve normal kapanış doğrulandı.
Diğer Linux dağıtımları ve fiziksel oyunlar henüz sınanmadı.

Windows prototip paketindeki `EVRKare.exe`, **Python içermeyen GUI başlatıcısıdır**.
Önce Python 3'ü Tk ile kur, sonra `Install-desktop.cmd` çalıştır. `UniFrame.exe`
native motordur. Tam bağımsız Windows GUI paketi Windows üzerinde
`python tools/freeze_desktop.py` ile oluşturulmalıdır; bu Linux ortamında
Windows PyInstaller paketi üretildiği iddia edilmez.

Kaynaktan başlatma:

```bash
python3 -m pip install -r requirements-desktop.txt
python3 -m desktop.app
```

Tk Python kurulumunda bulunmalıdır. Masaüstü paketini hedef işletim sisteminde
oluşturmak için `requirements-packaging.txt` içindeki PyInstaller kullanılır.
`tools/freeze_desktop.py` desteklenen Linux Tcl/Tk kitaplıklarını açıkça paketler;
NumPy gerektirmediği için dağıtıma eklemez.

Arayüz: profil seçimi, basit/gelişmiş görünüm, Başlat/Durdur, koyu/açık tema,
vurgu rengi, 2x/3x/4x, kalite/güç önayarları ve ölçüm dışa aktarma. İşlenen
**256×144** görüntüler ayrı önizlemededir; oyunun çözünürlüğü/FPS'si değiştirilmez.
Gerçek oyun FPS'si yerine yakalama hızı gösterilir; fiziksel scanout ve giriş
gecikmesi bilinmediğinde boş bırakılır. GPU kullanım/VRAM/watt ölçümü yoktur.

Yerel ad tablosu yeni ürün metnindeki önerilerle güncellendi: örneğin az-AZ
KainatKadr, uz-UZ OlamKadr, de-DE UniversalBild, fr-FR UniImage. İsimler
`user_proposed_unverified` olarak işaretlidir; arayüz çevirisi sayılmaz.
Türk Dünyası sayfası bunları ve dil/script/çeviri durumlarını gösterir.
Turan görseli sağlanmadığından dosya seçimi vardır; görsel uydurulmadı.

Masaüstü doğrulaması:

```bash
python3 tools/test_all.py
xvfb-run -a python3 tools/test_desktop.py
python3 tools/freeze_desktop.py
xvfb-run -a python3 tools/test_frozen_desktop.py
```

[X11/Windows capture sınırları, ölçümler ve kalan işler](docs/desktop.md).
