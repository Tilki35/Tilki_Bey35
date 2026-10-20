# EVRKare geliştirme öncelikleri

EVRKare (Evrensel Kare Artırma / EKA, uluslararası UniFrame) birincil olarak
kare üretimi yazılımıdır; çözünürlük yükseltme ürünü değildir. Hedef akış
A → AB → B → BC → C, AB/BC hareket tahminiyle üretilen görüntülerdir.
60 gerçek FPS → 120 fiziksel sunulan FPS bir hedeftir; mevcut sonuç değildir.

1. Gerçek görüntü yakalama: Linux X11 çalışıyor; Windows GDI kodu doğrulanmadı.
2. Gerçek zamanlı sunum: Tk araştırma önizlemesi var; GPU swapchain, sunum
   zaman damgaları ve oyun hızında sunum henüz yok.
3. Optical Flow / Motion Estimation: blok arama ve yoğun Lucas–Kanade deneyleri var.
4. Gerçek ara kare: CPU/yazılım Vulkan kaynak çözünürlüğünde doğrulandı;
   gerçek zamanlı fiziksel GPU performansı doğrulanmadı.
5. Frame Pacing / düşük gecikme: sınırlandırılmış kuyruğa ve önizleme zamanlamasına
   rağmen iki kareli interpolasyon lookahead ve CPU/pipe maliyeti getiriyor.
6. Ghosting Reduction: ileri/geri tutarlılık ve statik bölge koruması var;
   kapsamlı gerçek oyun kalite ölçümü ve temporal tracking eksik.
7. Fallback / Uyarlanabilir Mod: kesim ve maliyet nedeniyle tekrar/geçiş var.
8. Tekrar Kare / Harmanlama: ayrı yardımcı modlar; `generated_count` dışında.
9. CPU/GPU optimizasyonu: RGB hızlı yolları ve luma tekrar kullanımı; sonraki
   hedef GPU'da capture→flow→synthesis→swapchain, gereksiz readback olmadan.
10. Güç, profiller, UI, isteğe bağlı ölçekleme, diğer platformlar ve
    yerelleştirme, gerçek kare üretiminin önüne geçmez.

Kare tekrarı, harmanlama, çözünürlük büyütme veya FPS sayacını değiştirmek
hareket telafili gerçek ara kare üretimi olarak kabul edilmez. Önizlemeye
başvuru sayısı fiziksel gösterim sayısı veya oyunun gerçek FPS'si değildir.
Algılanmayan ölçümler null kalır. Kamuya açıklanmamış AFMF 2.1 algoritmalarını
bildiğimiz veya birebir uyguladığımız iddia edilmez.
