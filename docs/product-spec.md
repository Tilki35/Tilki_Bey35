# EVRKare — EVRENSEL KARE ARTIRMA (EKA)
# UniFrame — Universal Frame Generation
# TEK PARÇA ANA CODEX GELİŞTİRME PROMPTU

# 1. PROJENİN AMACI

Sıfırdan, bağımsız, gerçekten çalışan, düşük kaynak tüketimli, çok dilli ve çok platformlu bir kare işleme yazılımı geliştir.

Türkçe kısa ürün adı: EVRKare.

Türkçe tam adı: Evrensel Kare Artırma.

Türkçe kısaltması: EKA.

Uluslararası adı: UniFrame.

Program şu yaklaşımları tek bir uygulamada birleştirecek:

- AMD AFMF 2.1'in kamuya açıklanmış kare üretim ve optimizasyon prensipleri.
- Lossless Scaling benzeri kolay kullanım ve oyun profili yaklaşımı.
- Bağımsız gerçek kare üretimi.
- Bağımsız Tekrar Kare teknolojisi.
- Kare Harmanlama.
- Uyarlanabilir kare işleme.
- Düşük CPU ve GPU tüketimi.
- APU ve entegre GPU optimizasyonu.
- Düşük elektrik tüketimi.
- Çok dilli program isimlendirmesi.
- Türk dünyasına özel yerelleştirme.
- Genişletilebilir donanım ve grafik API desteği.

Bu teknolojileri bağımsız olarak geliştir.

AMD veya Lossless Scaling'in kapalı kaynak kodlarına erişimin olduğunu varsayma.

Ticari yazılımların kodlarını, arayüz varlıklarını veya özel algoritmalarını kopyalama.

Programın kendi teknik mimarisi ve görsel kimliği olsun.

# 2. EN ÖNEMLİ TASARIM KURALI: KOLAY KULLANIM

Program teknik bilgisi olmayan kullanıcılar için de kolay kullanılmalı.

Kullanıcı yalnızca şu işlemleri yaparak programı çalıştırabilsin:

1. EVRKare'yi aç.
2. Oyun profilini seç.
3. Kare işleme modunu seç veya Otomatik bırak.
4. Başlat düğmesine bas.

Gereksiz karmaşık menüler oluşturma.

Gelişmiş ayarları ayrı bir bölümde sakla.

Program ilk açıldığında güvenli ve dengeli varsayılan ayarlarla çalışsın.

# 3. ARAYÜZ TASARIMI — LOSSLESS SCALING BENZERİ

Kullanıcının sağladığı Lossless Scaling 3.2.2 ekran görüntüsünü görsel yerleşim referansı olarak kullan.

Birebir kopyalama yapma.

EVRKare'nin özgün arayüzünü oluştur.

Ana pencere iki bölüme ayrılacak.

**Sol bölüm:**

- Oyun Profilleri.
- Varsayılan profil.
- Yeni profil ekleme.
- Profil düzenleme.
- Profil silme.
- Ana Sayfa.
- Performans.
- Kılavuz.
- Ayarlar.

**Sağ bölüm:**

- Seçilen profilin adı.
- Kare Oluşturucu.
- Tekrar Kare.
- Kare Harmanlama.
- Ölçekleme.
- Yakalama.
- İşleyici.
- Güç Tasarrufu.
- Performans Ayarları.

Ayarlar, ekran görüntüsündeki gibi düzenli kartlar içerisinde gösterilecek.

Her kartta kısa başlıklar, açılır menüler, kaydırma çubukları ve açma-kapama düğmeleri bulunacak.

# 4. ARAYÜZÜN GÖRÜNÜMÜ

Tasarım özellikleri:

- Modern koyu tema.
- Açık tema seçeneği.
- Yuvarlatılmış kartlar.
- Okunabilir yazılar.
- Düzenli boşluklar.
- Kolay erişilen ayarlar.
- Küçük açıklama simgeleri.
- Türkçe arayüz.
- İsteğe bağlı gelişmiş görünüm.
- Pencere boyutuna uyumlu yerleşim.

Varsayılan tema koyu olsun.

EVRKare'ye özgü vurgu rengi kullanılabilsin.

Kullanıcı isterse vurgu rengini değiştirebilsin.

# 5. TEK TUŞLA BAŞLATMA

Ana pencerenin sağ üst köşesinde büyük ve kolay görülen bir düğme bulunsun.

Düğme adı:

**Başlat**

İşlem devam ederken:

**Durdur**

Program başlatıldığında:

- Seçili oyunu algıla.
- Donanımı kontrol et.
- Uygun yakalama yöntemini seç.
- Seçili kare işleme modunu etkinleştir.
- Kare zamanlamasını başlat.
- Performans ölçümlerini göster.

Desteklenmeyen durumlarda anlaşılır hata mesajı ver.

# 6. BASİT VE GELİŞMİŞ MENÜLER

İki kullanım görünümü oluştur.

**Basit Mod**

Yalnızca şu ayarlar görünsün:

- Oyun.
- Kare İşleme Modu.
- Çarpan.
- Kalite.
- Güç Profili.
- Başlat / Durdur.

**Gelişmiş Mod**

Şu seçenekler de açılsın:

- Search Mode.
- Performance Mode.
- Optical Flow ayarları.
- Yakalama API'si.
- Hedef kare kuyruğu.
- Kare gecikmesi.
- Frame Pacing.
- Fallback.
- Ghosting Reduction.
- Temporal Tracking.
- HDR.
- VRR.
- FPS sınırı.
- CPU/GPU işlem yöntemi.

Kullanıcı istediği zaman iki görünüm arasında geçiş yapabilsin.

# 7. KARE OLUŞTURUCU KARTI

Ana arayüzde Kare Oluşturucu adlı kart bulunsun.

Ayarlar:

**Tür**

- Gerçek Kare Üretimi.
- Tekrar Kare.
- Kare Harmanlama.
- Uyarlanabilir Mod.
- Kapalı.

**Mod**

- Otomatik.
- Sabit.
- Uyarlanabilir.

**Çarpan**

- 2x.
- 3x.
- 4x.
- Desteklenen durumlarda özel değer.

**Akış Ölçeği**

Optical Flow işleminin çözünürlüğünü ve hesaplama maliyetini ayarlayan seçenek.

**Performans**

- Eco.
- Balanced.
- Performance.
- Quality.

Seçilen moda göre kullanılmayan ayarları gizle veya pasifleştir.

# 8. GERÇEK KARE ÜRETİMİ — EKA-KÜ

Optical Flow ve Motion Estimation kullanarak iki gerçek kare arasındaki hareketi analiz et.

Örnek:

A → AB → B → BC → C

AB ve BC, hareket tahminiyle oluşturulan yeni ara karelerdir.

Özellikler:

- Gerçek zamanlı hareket analizi.
- Hareket vektörleri.
- Ara kare sentezleme.
- Nesne kenarı koruma.
- Ghosting azaltma.
- Temporal Tracking.
- Sahne değişimi algılama.
- HUD koruma.
- Metin koruma.
- Düşük gecikme optimizasyonu.
- GPU hızlandırması.
- CPU alternatifi.
- Frame Pacing.

Gerçek kare üretimi yalnızca FPS sayacını değiştirmemeli.

Yeni görüntü kareleri oluşturmalıdır.

# 9. TEKRAR KARE — EKA-TK

Tekrar Kare bağımsız bir özellik olacaktır.

Mevcut kareleri yeniden sunacaktır.

Örnek:

Normal:

A → B → C

2x:

A → A → B → B → C → C

3x:

A → A → A → B → B → B

4x:

A → A → A → A → B → B → B → B

Seçenekler:

- Otomatik.
- 2x.
- 3x.
- 4x.
- Özel tekrar oranı.

Özellikle düşük CPU ve GPU kullanımı hedefle.

Gereksiz görüntü kopyalamalarını azalt.

Tekrar Kare'nin yeni hareket bilgisi oluşturmadığını açıkça belirt.

60 gerçek FPS'nin 2x tekrarlanması, 120 farklı hareket karesi anlamına gelmez.

# 10. KARE HARMANLAMA — EKA-KH

İki komşu kareyi belirli ağırlıklarla birleştir.

Harmanlama oranı ayarlanabilsin.

GPU ve CPU tabanlı yöntemleri değerlendir.

Görüntü bulanıklığını ve ghosting etkisini ölç.

# 11. UYARLANABİLİR MOD — EKA-UM

Donanım performansını ve görüntü hareketini analiz et.

Gerektiğinde uygun kare işleme yöntemini seç.

Karar verirken şunları değerlendir:

- CPU kullanımı.
- GPU kullanımı.
- RAM ve VRAM tüketimi.
- Kare üretim süresi.
- Hareket yoğunluğu.
- Görüntü kalitesi.
- Ekran yenileme hızı.
- Enerji tüketimi.

Mod geçişleri sırasında takılmaları azalt.

# 12. AFMF 2.1 BENZERİ ÖZELLİKLER

AMD AFMF 2.1'in kamuya açıklanmış özelliklerini incele.

Bağımsız karşılıklarını geliştirmeyi hedefle.

Kapsam:

- Search Mode.
- Performance Mode.
- Hızlı hareket optimizasyonu.
- Düşük gecikme optimizasyonu.
- Frame Pacing.
- Fallback.
- Hareket tahmini.
- Optical Flow.
- Ghosting Reduction.
- Temporal Tracking.
- Sahne değişimi algılama.
- Entegre GPU optimizasyonu.
- GPU iş yükü optimizasyonu.
- Oyun bazlı profiller.
- Tam ekran desteği.
- Kenarlıksız tam ekran desteği.
- Otomatik performans profilleri.

AFMF 2.1'in resmî özellikleri ile EVRKare'nin ek özelliklerini belgelerde ayır.

# 13. SEARCH MODE

Seçenekler:

- Auto.
- Standard.
- High.

Hızlı hareketlerde ve ani kamera dönüşlerinde görüntü bozulmalarını azaltmaya çalış.

# 14. PERFORMANCE MODE

Seçenekler:

- Auto.
- Quality.
- Performance.

Düşük güçlü donanımlarda daha hafif hesaplama yöntemleri kullan.

# 15. FALLBACK VE GÖRÜNTÜ KALİTESİ

Hareket tahmininin güvenilir olmadığı durumları algıla.

Örnekler:

- Ani sahne değişimi.
- Çok hızlı hareket.
- Hareket vektörü hataları.
- Yetersiz işlem süresi.

Bozuk ara kare yerine güvenli alternatif kullan.

Ghosting, HUD bozulmaları ve hareket hatalarını test et.

# 16. ÖLÇEKLEME KARTI

Gönderilen arayüzdeki Ölçekleme bölümüne benzer, bağımsız bir ölçekleme sistemi geliştir.

Seçenekler:

**Tür**

- Kapalı.
- Otomatik.
- Uzamsal ölçekleme.
- Destekleniyorsa ek bağımsız algoritmalar.

**Mod**

- Otomatik.
- Kalite.
- Dengeli.
- Performans.

**Görüntü Oranı**

- En boy oranını koru.
- Ekrana sığdır.
- Ekranı doldur.
- Özel oran.

Ölçekleme ile kare üretimi birbirinden bağımsız açılıp kapatılabilsin.

Başka yazılımların özel algoritmalarını izin olmadan kopyalama.

# 17. YAKALAMA KARTI

Ekran görüntüsündeki Yakala bölümüne benzer bir kart oluştur.

Ayarlar:

**Yakalama API'si**

- Otomatik.
- Windows Graphics Capture.
- Desteklenen diğer platform yakalama yöntemleri.

**Hedef Kuyruğu**

- Otomatik.
- Düşük gecikme.
- Dengeli.
- Özel değer.

Görüntü yakalama yönteminin işletim sistemi ve oyun görüntüleme moduyla uyumunu kontrol et.

# 18. İŞLEYİCİ KARTI

Ayarlar:

- Eşitleme Modu.
- Azami Kare Gecikmesi.
- HDR Desteği.
- VRR Desteği.
- G-SYNC Compatible uyumluluğu.
- FreeSync uyumluluğu.
- FPS Göstergesi.
- Düşük Gecikme Modu.
- Kare Zamanlaması.

HDR ve ekran senkronizasyonu özelliklerini yalnızca gerçekten desteklenen durumlarda etkinleştir.

# 19. DÜŞÜK CPU KULLANIMI

CPU üzerindeki gereksiz işlem yükünü azalt.

Yöntemler:

- Olay tabanlı işleme.
- Uyarlanabilir iş parçacıkları.
- Gereksiz döngüleri kaldırma.
- SIMD optimizasyonları.
- Bellek tahsislerini azaltma.
- Gereksiz veri kopyalarını önleme.
- Uygun CPU/GPU iş bölümü.
- Boşta işlem yapmama.

# 20. DÜŞÜK GPU KULLANIMI

GPU işlem maliyetini azalt.

Yöntemler:

- Compute Shader optimizasyonu.
- Uygun iş grubu boyutları.
- Bellek bant genişliği optimizasyonu.
- Gereksiz görüntü kopyalarını kaldırma.
- Düşük maliyetli Optical Flow seçenekleri.
- İşlem yoğunluğuna göre kalite ayarı.
- GPU yüküne göre otomatik algoritma seçimi.

# 21. APU VE ENTEGRE GPU OPTİMİZASYONU

AMD APU ve Intel entegre GPU'lar için özel işlem yolları oluştur.

Özellikle:

- Paylaşımlı bellek kullanımı.
- Bellek bant genişliği.
- Düşük güçlü GPU mimarileri.
- CPU ile GPU'nun ortak güç bütçesi.
- Düşük enerji tüketimi.
- Isınma nedeniyle performans düşüşleri.

APU üzerinde gereksiz CPU ve GPU yükü oluşturma.

# 22. GÜÇ TASARRUFU KARTI

Ana arayüzde Güç Tasarrufu bölümü bulunsun.

Profiller:

**Eco**

En düşük kaynak ve enerji tüketimini hedefle.

**Balanced**

Güç tüketimi ile görüntü kalitesini dengele.

**Performance**

Kare üretim kapasitesine öncelik ver.

**Quality**

Görüntü kalitesine öncelik ver.

**Auto**

Donanıma ve oyun yüküne göre otomatik seçim yap.

Desteklenen donanımlarda güç tüketimini ölç.

Ölçülmeyen değerleri gerçek ölçüm gibi gösterme.

# 23. OYUN PROFİLLERİ

Lossless Scaling ekran görüntüsündeki gibi sol tarafta oyun profilleri listesi oluştur.

Özellikler:

- Yeni profil.
- Profil düzenleme.
- Profil silme.
- Profil kopyalama.
- Varsayılan profil.
- Oyuna özel ayarlar.
- Son kullanılan profil.
- Oyun çalışırken uygun profili algılama.

Kullanıcı istediği kadar profil oluşturabilsin.

# 24. FPS GÖSTERGESİ

Ekran üzerinde isteğe bağlı FPS göstergesi oluştur.

Ayrı ayrı göster:

- Gerçek FPS.
- Üretilen yeni kareler.
- Tekrar edilen kareler.
- Sunulan toplam kare sayısı.
- Kare üretim süresi.
- CPU kullanımı.
- GPU kullanımı.
- RAM tüketimi.
- VRAM tüketimi.
- Destekleniyorsa güç tüketimi.
- Kullanılan kare işleme modu.

Tekrar edilen kareleri gerçek ara kare olarak sayma.

# 25. GECİKME VE FRAME PACING

Kareleri düzenli aralıklarla sun.

Gereksiz tamponlamayı azalt.

CPU ve GPU işlemlerini uygun biçimde zamanla.

Giriş gecikmesini yalnızca güvenilir biçimde ölçebiliyorsan raporla.

Kare üretiminin oyun motorunun gerçek güncelleme hızını artırmadığını dikkate al.

# 26. DONANIM DESTEĞİ

Hedefler:

- AMD Radeon.
- NVIDIA GeForce.
- Intel Arc.
- Intel entegre GPU.
- AMD APU.
- Apple Silicon.
- CPU tabanlı işleme.

Eski donanımlar için hafif algoritmalar geliştir.

Desteklenmeyen özellikleri otomatik devre dışı bırak.

# 27. GRAFİK API DESTEĞİ

Hedefler:

- DirectX 9.
- DirectX 10.
- DirectX 11.
- DirectX 12.
- Vulkan.
- OpenGL.
- Metal.

Her API için gerçek entegrasyon ve uyumluluk testleri yap.

Desteklenmeyen API'leri destekleniyor diye gösterme.

# 28. İŞLETİM SİSTEMLERİ

Hedefler:

- Windows 10.
- Windows 11.
- Linux.
- macOS.

Linux üzerinde X11 ve Wayland farklılıklarını dikkate al.

macOS üzerinde Metal ve uygun yakalama yöntemlerini değerlendir.

# 29. OYUN MOTORUNDAN BAĞIMSIZ ÇALIŞMA

Desteklenen oyunlarda özel oyun motoru entegrasyonu gerektirmeden çalışabilecek yöntemler geliştir.

Tam ekran, kenarlıksız tam ekran ve pencere modlarını değerlendir.

Anti-cheat korumalarını atlatmaya yönelik yöntemler geliştirme.

# 30. TÜRKÇE VE ULUSLARARASI PROGRAM İSİMLERİ

Programın görünen adı işletim sisteminin diline göre otomatik değişsin.

Temel isimler:

- Türkçe: EVRKare.
- Türkçe tam ad: Evrensel Kare Artırma.
- İngilizce: UniFrame.

Kullanıcı isterse görünen adı manuel seçebilsin.

# 31. TÜRK DEVLETLERİNE ÖZEL İSİMLER

| Dil | Program adı |
|---|---|
| Türkçe | EVRKare |
| Azerbaycanca | KainatKadr |
| Kazakça | ӘмКадр |
| Kırgızca | АаламКадр |
| Özbekçe | OlamKadr |
| Türkmence | ÄlemKadr |

Kuzey Kıbrıs Türk toplumunun Türkçe kullanımında EVRKare adını kullan.

# 32. DİĞER TÜRK DİLLERİNE ÖZEL İSİMLER

| Dil | Program adı |
|---|---|
| Tatarca | ГаләмКадр |
| Başkurtça | ҒаләмКадр |
| Uygurca | ئالەمكادىر |
| Karakalpakça | ÁlemKadr |
| Kırım Tatarcası | AlemKadr |
| Gagavuzca | DünnäKadr |
| Nogayca | AlemKadr |
| Kumukça | AlemKadr |
| Karaçay-Balkarca | AlemKadr |
| Çuvaşça | ТӗнчеКадр |
| Sahaca / Yakutça | ДойдуКадр |
| Tuvaca | ДелегейКадр |
| Hakasça | ЧирКадр |
| Altayca | ЈерКадр |
| Dolganca | ДолганКадр |
| Şorca | ШорКадр |
| Salarca | SalarKadr |
| Halaçça | XeleçKadr |
| Sibirya Tatarcası | СеберКадр |
| İli Türkçesi | İliKadr |

Bunlar başlangıç marka önerileridir.

Doğrulanmış çeviriler olarak sunma.

Ana dili konuşurları ve güvenilir dil kaynaklarıyla doğrulanabilecek şekilde sakla.

Diğer belgelenmiş Türk dillerini de ekleyebilecek altyapı geliştir.

# 33. DÜNYA DİLLERİNE ÖZEL İSİMLER

| Dil | Program adı |
|---|---|
| İngilizce | UniFrame |
| Almanca | UniversalBild |
| Fransızca | UniImage |
| İspanyolca | UniFotograma |
| Portekizce | UniQuadro |
| İtalyanca | UniFotogramma |
| Rusça | УниКадр |
| Ukraynaca | УніКадр |
| Lehçe | UniKlatka |
| Japonca | 万能フレーム |
| Çince | 万象帧 |
| Korece | 만능프레임 |

Arapça, Farsça, Hintçe, Bengalce, Urduca, Pencapça, Tamilce, Telugu, Marathi, Nepalce, Tayca, Vietnamca, Endonezce, Malayca, Filipince, Svahili, Amharca, İbranice, Ermenice, Gürcüce, Fince, İsveççe, Norveççe, Danca, Hollandaca, Çekçe, Slovakça, Macarca, Romence, Bulgarca, Sırpça, Hırvatça, Boşnakça, Arnavutça, Estonca, Letonca, Litvanca, İzlandaca, İrlandaca, Galce, Baskça, Katalanca, Kürtçe, Peştuca, Moğolca, Tibetçe ve diğer belgelenmiş diller için de yerel isimlendirme altyapısı geliştir.

Doğrulanmamış isimleri kesin çeviri olarak gösterme.

Doğrulanmış isim bulunmuyorsa UniFrame kullan.

# 34. OTOMATİK DİL ALGILAMA

İşletim sisteminin tercih edilen görüntüleme dilini algıla.

Örnek:

- tr-TR → EVRKare.
- az-AZ → KainatKadr.
- kk-KZ → ӘмКадр.
- ky-KG → АаламКадр.
- uz-UZ → OlamKadr.
- tk-TM → ÄlemKadr.
- en-US → UniFrame.
- ru-RU → УниКадр.
- de-DE → UniversalBild.
- fr-FR → UniImage.
- ja-JP → 万能フレーム.
- cv-RU → ТӗнчеКадр.
- sah-RU → ДойдуКадр.
- tyv-RU → ДелегейКадр.
- kjh-RU → ЧирКадр.
- alt-RU → ЈерКадр.
- dlg-RU → ДолганКадр.
- cjs-RU → ШорКадр.

Ülke konumundan dil veya etnik kimlik tahmini yapma.

Manuel seçim otomatik algılamadan öncelikli olsun.

# 35. YERELLEŞTİRME ALTYAPISI

BCP 47, Unicode ve CLDR standartlarından yararlan.

Her dil için şu bilgileri sakla:

- Dil kodu.
- Dilin yerel adı.
- Programın yerel adı.
- Kısaltma.
- Tam açılım.
- Kullanılan alfabe.
- Bölge.
- Çeviri durumu.
- İsim doğrulama durumu.

Program adı ile arayüz dili ayrı ayrı seçilebilsin.

Sağdan sola yazılan diller desteklensin.

# 36. TURAN BAYRAĞI

Kullanıcının sağladığı Turan bayrağı görselini programa ekle.

Türk dillerinden biri seçildiğinde varsayılan olarak göster.

Seçenekler:

- Otomatik.
- Açık.
- Kapalı.

Bayrağı devletlerin resmî bayraklarının yerine kullanma.

Görsel henüz sağlanmadıysa kullanıcıdan iste.

# 37. TÜRK DÜNYASI BÖLÜMÜ

Programda Türk Dünyası adlı bölüm oluştur.

Burada:

- Türk dilleri.
- Yerel program isimleri.
- Alfabeler.
- Dil kodları.
- Çeviri durumları.
- İsim doğrulama durumları.

gösterilsin.

Bağımsız devletler, özerk bölgeler ve diğer ülkelerdeki Türk dilli topluluklar kapsansın.

# 38. TÜRK KÖKENLİ PROJE KİMLİĞİ

Hakkında bölümünde şu açıklama bulunsun:

"EVRKare, Türk geliştirici tarafından başlatılmış, evrensel kare artırma ve kare üretim teknolojisi geliştirmeyi amaçlayan bağımsız bir yazılım projesidir."

İngilizce açıklama:

"UniFrame is an independent software project initiated by a Turkish developer, aiming to develop universal frame processing and frame generation technology."

# 39. YAZILIM MİMARİSİ

Modüler yapı geliştir.

Bileşenler:

- Core.
- Frame Capture.
- Frame Generation.
- Frame Repetition.
- Frame Blending.
- Adaptive Processing.
- Optical Flow.
- Motion Estimation.
- Search Mode.
- Performance Mode.
- Power Optimization.
- CPU Optimization.
- GPU Optimization.
- APU Optimization.
- Fallback Controller.
- Temporal Tracking.
- Ghosting Reduction.
- HUD Protection.
- Frame Pacing.
- Frame Presentation.
- Scaling.
- Hardware Detection.
- Multi-GPU Management.
- Platform Integration.
- Localization.
- Cultural Identity.
- User Interface.
- Benchmark.
- Settings.

# 40. GELİŞTİRME SIRASI

1. Çalışan temel uygulama.
2. Gönderilen ekran görüntüsünden esinlenen özgün arayüz.
3. Basit ve gelişmiş menüler.
4. Oyun profilleri.
5. Gerçek görüntü yakalama.
6. Tekrar Kare sistemi.
7. Kare Harmanlama.
8. Optical Flow.
9. Gerçek ara kare üretimi.
10. Uyarlanabilir Mod.
11. Search Mode.
12. Performance Mode.
13. Fallback.
14. Ghosting Reduction.
15. Temporal Tracking.
16. HUD koruma.
17. Düşük gecikme.
18. Frame Pacing.
19. CPU optimizasyonu.
20. GPU optimizasyonu.
21. APU optimizasyonu.
22. Güç tasarrufu profilleri.
23. Ölçekleme sistemi.
24. FPS göstergesi.
25. Oyun entegrasyonları.
26. Otomatik program isimlendirmesi.
27. Türk dillerine özel isimler.
28. Dünya dillerine özel isimler.
29. Turan bayrağı ve Türk dünyası bölümü.
30. Donanım ve işletim sistemi testleri.
31. Kararlı sürüm.

# 41. GELİŞTİRME KURALLARI

- Gerçek çalışan kod yaz.
- Yalnızca arayüz oluşturup projeyi tamamlanmış sayma.
- Sahte FPS değerleri üretme.
- Tekrar Kare ile gerçek kare üretimini karıştırma.
- Ölçmeden düşük güç tüketimi garantisi verme.
- Tamamlanmamış özellikleri tamamlanmış gösterme.
- AMD'nin veya başka yazılımların özel algoritmalarını kopyalama.
- Başka yazılımların arayüzünü birebir kopyalama.
- Desteklenmeyen donanımları destekleniyor diye gösterme.
- Gereksiz bağımlılıklardan kaçın.
- Kaynak kodunu düzenli tut.
- Derleme ve çalıştırma talimatlarını hazırla.
- Gerçek test sonuçlarını raporla.

# 42. TESLİMAT

Geliştirme sonunda:

1. Çalışan kaynak kodu.
2. Derleme talimatları.
3. Kullanımı kolay arayüz.
4. Oyun profilleri.
5. Gerçek Kare Üretimi testleri.
6. Tekrar Kare testleri.
7. Kare Harmanlama testleri.
8. CPU kullanım ölçümleri.
9. GPU kullanım ölçümleri.
10. RAM ve VRAM ölçümleri.
11. Destekleniyorsa güç tüketimi ölçümleri.
12. FPS ve gecikme testleri.
13. Desteklenen donanım listesi.
14. Desteklenen grafik API listesi.
15. Desteklenen işletim sistemleri.
16. Desteklenen diller ve yerel isimler.
17. Tamamlanmamış özellikler.
18. Sonraki sürüm geliştirme planı.

# 43. SON TALİMAT

Yalnızca açıklama veya proje planı hazırlama.

Gerçek çalışan kod yazmaya başla.

İlk olarak kullanıcının sağladığı Lossless Scaling ekran görüntüsünden esinlenen, ancak özgün tasarıma sahip, basit ve gelişmiş kullanım modları bulunan EVRKare arayüzünü geliştir.

Ardından gerçek görüntü yakalama ve kare işleme motorlarını oluştur.

Programın temel kullanımı mümkün olduğunca kolay olmalı.

**Kullanıcı oyunu seçip Başlat düğmesine bastığında desteklenen sistemlerde kare işleme başlayabilmeli.**

Gerçek kare üretimi, tekrar kare, kare harmanlama ve uyarlanabilir mod birbirinden bağımsız çalışmalı.

CPU, GPU, APU, RAM, VRAM ve enerji tüketimi mümkün olduğunca düşük tutulmalı.

Bütün önceki özellikler aynı program içerisinde birleştirilmeli.

**TÜRKÇE AD: EVRKare**

**TAM AD: Evrensel Kare Artırma**

**KISALTMA: EKA**

**İNGİLİZCE AD: UniFrame**

**HEDEF: AFMF 2.1 benzeri gelişmiş işlevleri, Lossless Scaling benzeri kolay kullanımı ve bağımsız kare işleme teknolojilerini birleştiren, düşük kaynak tüketimli, gerçekten çalışan bir yazılım.**