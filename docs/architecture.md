# Teknik mimari ve aşamalar

## Hedef ve gecikme bütçesi

Hedef: uygulamanın ürettiği A ve B gerçek kareleri arasına bir G ara karesi
eklemek. 100 Hz gerçek üretim sürdürülebilir ve ekran/sunum 200 Hz destekliyorsa,
uygun zamanlamada 200 kare/s görüntüleme araştırılabilir. Oyun simülasyonu ve
girdi örnekleme hızı 100 Hz kalır. Hareket kestirimi gerçek girdiyi hızlandırmaz.

İki kareli interpolasyon B'yi bekler: 100 FPS'te bir kare yaklaşık 10 ms'dir.
Bu beklemeyi, GPU işlemini ve sunum kuyruğunu ayrı hesaplamak gerekir.
Üretilen kareyi ekrana daha erken vermek için geçmişten extrapolasyon başka bir
algoritmadır ve hata riski daha yüksektir. Düşük gecikme ve iki kareli kalite
arasında açık bir tercih vardır; sıfır ek gecikme sözü verilmez.

## Veri akışı

```text
Vulkan uygulaması [gelecek]
  -> opt-in Vulkan layer: swapchain + present takibi [gelecek]
  -> izinli görüntü kopyası + renk uzayı/format dönüşümü [gelecek]
  -> A/B geçmişi
  -> hareket analizi (ileri + geri) [prototip]
  -> t=0.5 inverse warp + tutarlılık kontrolü [prototip]
  -> zamanlayıcı + sunum [gelecek]
  -> gerçek/üretilmiş/sunulan/düşen kare telemetrisi [gelecek]
```

Bugünkü dosya sentetik A/B üretir veya 256×144 PPM çifti okur. Tekrar,
karıştırma, analiz, interpolasyon ve adaptif modlar ayrı seçilir. Gerektiğinde
blok akışı, yoğun refinement ve warp dispatch'leri çalışır. Vulkan oyun katmanı
ve ekrana sunum yoktur. Merkezi dil/isim çözümü başlangıçtadır, shader'da değildir.

## 1. Vulkan katmanı

Gelecekte uygulama bazında açıkça etkinleştirilen bir layer manifest kullanılacak.
Instance/device dispatch zinciri Vulkan loader sözleşmesine uygun kurulacak;
instance/device oluşturma, swapchain oluşturma/yok etme, image acquisition ve
`vkQueuePresentKHR` ele alınacak. Süreç genelinde gizli enjeksiyon yapılmayacak.
Anti-cheat/protected content engelleri aşılmayacak; izin vermeyen oyunlarda
katman devre dışı kalacak. Katman uyumluluğu oyun motorundan bağımsızlık hedefidir,
her oyunda çalışacağının kanıtı değildir.

Swapchain görüntüsünü okumak için uygun usage flag ve format desteği
sorgulanmalı. `TRANSFER_SRC` körlemesine eklenemez; surface/device yetenekleri
uyuşmazsa kare üretimi kapatılmalı. Resize, minimize, HDR, out-of-date,
suboptimal, çoklu swapchain ve farklı queue family'ler ayrı ele alınmalı.

## 2. Kare yakalama ve senkronizasyon

Present öncesinde, uygulamanın render-complete semaphore'ları yalnız bir kez
tüketilecek. Layer kopya/compute işini bu semaphore'ları bekleyerek gönderecek;
orijinal present, layer'ın iş tamamlandı semaphore'unu bekleyecek. Image layout,
queue ownership ve erişim bariyerleri açıkça takip edilecek. Semaphore'lar ve
history slot'ları işi bitmeden yeniden kullanılmayacak.

Her karede `vkQueueWaitIdle`/`vkDeviceWaitIdle` kullanılmayacak. Bounded ring
buffer, fence/timeline takibi, generation ID ve en fazla bir bekleyen üretilmiş
kare politikası gecikmenin sınırsız büyümesini önleyecek. Bugünkü offline
prototip basit fence beklemesi kullanır; canlı sunum için tasarlanmamıştır.
Cihaz/pipeline/tampon/descriptor/command buffer/fence çiftler arasında yeniden
kullanılır. Normal kare yolunda device-idle yoktur; kapanış/hata temizliğinde vardır.

## 3. Hareket analizi

Bugün: shader üzerinde 8×8 blok, ±12 piksel tamsayı arama, blok başına
seyrek 16 parlaklık örneğiyle ileri/geri block matching. Bu temel görüntü tabanlı
hareket tahminidir. İsteğe bağlı yoğun Lucas–Kanade, bu vektörleri başlangıç
alıp piksel başına 5×5 pencerede üç iterasyonla ileri/geri alt piksel hareketi
iyileştirir. Tek ölçekli araştırma prototipidir; üretim kalitesinde akış değildir.
256×144 ve küçük hareket içindir. Büyük hareketlerde, tekrar eden desenlerde ve
obje sınırlarında güvenilmezdir. Arama maliyeti yarıçapın karesiyle artar.

Sonraki adım: çok ölçekli luminance piramidi, coarse-to-fine arama, daha güçlü
texture confidence, komşulukla düzenleme ve ileri/geri eşleşmenin
karşılık gelen koordinatta kontrolü. Üretici optical-flow donanımı zorunlu
olmayacak; isteğe bağlı hızlandırıcılar genel compute yolundan ayrılacak.

## 4. Ara kare üretimi

Bugün: iki iterasyonlu yaklaşık inverse mapping, bilinear örnekleme ve iki
kaynağın ortalaması. Tamamen değişmeden kalan pikseller doğrudan korunur.
Akış tutarsızlığı, kanal başına 1/255'ten fazla hizalanmış kaynak farkı veya
görüntü dışı erişimde B karesine geri dönülür. Böylece belirsiz kenarlarda
karıştırma yerine akıcılıktan vazgeçilir. Bu koruyucu eşikler tam bir occlusion/confidence modeli
değildir; semantik HUD ayrımı ve disocclusion doldurma yoktur. Adaptif modda
görüntünün çoğunda büyük luminance sıçraması için basit kesim sezgiseli vardır;
genel sahne kesimi algısı veya hareketli UI ayrıştırıcısı değildir.
Mevcut RGB alanında işlem yapılır; HDR ve renk doğruluğu iddiası yoktur.

Sonraki adım: occlusion maskesi ve tek kaynak seçimi, güven ağırlıklı birleştirme,
kenar koruma, sahne kesiminde history reset, doğrusal renk alanı/HDR dönüşümleri.
HUD için güvenilir motor metadatası yoksa ayrı test/koruma stratejisi gerekir.

## 5. Sunum ve frame pacing

Bugün `src/pacing.hpp` mutlak deadline planı, refresh/ready-time kontrolü,
geç sentetik işi düşürme ve steady-clock callback adaptörü içerir.
`--pacing-demo` bunun etiketlenmiş simülasyon kaydını verir; callback kendisi
ekrana sunum yapmaz. `Telemetry` kaynak/üretim/tekrar/sunum başvurusu olaylarını
ayırır; girdi zaman damgası yokken gecikme uydurmaz. Gerçek sunum bağlantısı eksiktir.

İlk gerçek zamanlı aşama oyun layer'ı değil, sahip olduğumuz bir Vulkan test
uygulamasının swapchain'i olacak. Sabit zaman çizelgesinde A/G/B sunumu,
refresh sınırları, VSync/VRR, missed deadline ve input kuyruğu ölçülecek.
İş geç kalıyorsa G düşürülmeli; sonraki gerçek kare geciktirilmemeli.

Layer içinde ikinci kez present çağırmak tek başına güvenli bir çözüm değildir.
Uygulamaya ait acquisition/ownership ve image sayısı korunmalıdır. Sunum
görüntülerinin kim tarafından edinilip geri bırakılacağı tasarlanmadan oyunlara
frame doubling eklendiği iddia edilmeyecek. Layer PoC önce yalnız gözlem/kopya
yapacak; ekstra sunum ancak kontrollü örnekte doğrulandıktan sonra eklenecek.

## 6. Ölçümler

| Ölçüm | Kaynak ve sınırlama |
|---|---|
| Gerçek FPS | Layer'a giren benzersiz uygulama kareleri / monoton zaman |
| Üretilen FPS | Gerçekten tamamlanmış G kareleri / zaman |
| Sunulan FPS | Başarılı present olayları; gerçek scanout için ayrıca display timing desteği gerekir |
| GPU maliyeti | Desteklenen queue timestampValidBits + timestampPeriod; flow/warp/transfer ayrı |
| Frame pacing | Present/display zamanları p50/p95/p99; yalnız ortalama FPS yeterli değil |
| Giriş gecikmesi | Kontrollü uygulamada input event→render/present yazılım ölçümü; input→photon için dış donanım |

Present sayısı fiziksel ekranda gösterilen kare sayısı değildir. GPU süresi de
uçtan uca giriş gecikmesi değildir. Bugünkü JSON bu ölçülmemiş alanları boş tutar.
Vulkan compute timestamp query'si desteklendiğinde işlem süresi ölçülür;
CPU backend veya timestamp desteği olmayan kuyrukta bu alan null kalır.
Yazılımsal Vulkan timestamp'i fiziksel GPU performans ölçümü değildir.

## 7. Uyumluluk ve optimizasyon

GPU yolu Vulkan 1.1 compute kullanır; üretici adı kontrol edilmez. Gerekli
özellikler ve bellek türü uygun değilse otomatik CPU referans yolu vardır.
CPU performansı yetersizse kullanıcıya bu açıkça bildirilmelidir. RX 7800 XT
için sonraki optimizasyon: device-local image/buffer ve staging, paylaşımlı bellek
tiling, piramit araması, fp16/subgroup desteğini sorgulayıp isteğe bağlı kullanma.
Mevcut host-coherent buffer tasarımı GPU performans ölçümü için nihai tasarım değildir.

DX11/DX12: Vulkan layer DirectX oyunlarını doğrudan yakalayamaz. Ayrı DXGI
capture/present entegrasyonu ve gerekirse external memory/semaphore interop
tasarımı gerekir. Windows Graphics Capture + ayrı pencere bir araştırma alternatifi;
fullscreen, HDR, gecikme ve korumalı içerik açısından ayrıca sınanmalıdır.

### Geniş API uyumluluğu hedefi

Kullanıcı hedefi OpenGL ve DirectX 1–12 dönemlerindeki oyunları da kapsar.
DirectX 4 yayımlanmamıştır; DirectX sürüm numarası ile Direct3D arayüz sürümü
her zaman aynı kavram değildir. Uyumluluk eski API DLL adlarının varlığıyla
değil, seçilmiş gerçek oyunların yakalanıp doğru sunulmasıyla doğrulanacak.

| Kaynak | Planlanan giriş yolu | Bugünkü durum |
|---|---|---|
| Vulkan | Opt-in layer + izinli swapchain kopyası | Yalnız offline compute çekirdeği var |
| DXGI / Direct3D 10, 11, 12 | Windows capture öncelikli; gerekirse ayrı DXGI backend | Uygulanmadı |
| OpenGL | Windows'un yakalayabildiği pencere/ekran çıktısı | Uygulanmadı |
| Direct3D 9, 8 ve daha eski Direct3D/DirectDraw | Modern Windows'ta çalışan oyunun pencere/ekran çıktısı | Uygulanmadı |
| Vulkan'sız donanım | CPU referansı; gelecekte capture ile bağlantı | Yalnız offline CPU çekirdeği var |

Ortak Windows capture yolu oyun API'sinden gelen ayrıntıları gizleyebilir;
ama tüm oyunlarda capture mümkün olduğu anlamına gelmez. Exclusive fullscreen,
overlay, DRM/korumalı içerik ve anti-cheat sınırlamaları ayrıca değerlendirilecek.
Uygun izinle yakalanan görüntü önce ortak RGBA/history biçimine dönüştürülecek;
compute backend ve sunum bundan sonra ortak olacak. Gerçek zamanlı Windows
Graphics Capture yolu modern Windows gerektirir; eski API desteği Windows 95
veya dönem donanımında çalışma sözü değildir. İlk sürümde API wrapper'ları
otomatik indirmek veya oyunun DLL'lerini değiştirmek yoktur.

## Aşamalar ve kabul ölçütleri

1. **Offline çekirdek (bugün):** shader derlemesi/SPIR-V doğrulaması, Vulkan
   compute, CPU referansı, yedi analitik sahne ve hata yolu testleri, Windows
   x64 çapraz derlemesi. Gerçek Windows/GPU testi henüz yapılmadı.
2. **Kalite:** çok ölçekli flow; subpixel, obje/arka plan ayrışması, örtülme,
   sahne kesimi ve HUD veri seti. Sadece kolay sahnelerdeki başarı yeterli değil.
3. **Kontrollü canlı test uygulaması:** Vulkan swapchain ile gerçek A/G/B,
   ölçülen GPU bütçesi ve scanout/pacing. 100→200 hedefi ancak burada sınanabilir.
4. **Opt-in layer:** önce pass-through ve capture; sonra kontrollü oyunlarda
   history ve presentation. Validation layer hatasızlığı, resize/çoklu kuyruk testleri.
5. **Donanım matrisi:** RX 7800 XT fiziksel test; ardından ayrı Intel/AMD/NVIDIA
   cihazlar. Her model, sürücü ve çözünürlük ayrı raporlanacak.
6. **DX11/DX12 araştırması:** ölçülmüş Vulkan prototipinden sonra ayrı backend.


## Masaüstü canlı önizleme aşaması

`desktop/` katmanı gerçek X11 pencere capture, bağımsız thread, tek güncel grup
kuyruğu, kalıcı native `--stream` motoru ve Tk önizleme callback'lerini bağlar.
Windows GDI adaptörü kaynakta bulunur; test edilmedi. Bu, oyun swapchain'ine
kare ekleme veya 200 Hz fiziksel sunum değildir. Detaylar ve eksik üretim
özellikleri [desktop.md](desktop.md) içindedir.
