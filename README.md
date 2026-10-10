# VOLT — Devre Macerası

Qt 6 + OpenGL ile yazılmış, Mario tarzı bir 3D platform oyunu. Karakter, tamamen kodla üretilen özgün bir hümanoid robot olan **VOLT**'tur (turuncu-beyaz gövde, cam-mavisi gözler, enerji çekirdeği, anten).

Model, doku ve ses dosyası yoktur. Her şey yordamsal olarak üretilir. Telifli hiçbir karakter ya da varlık kullanılmaz.

## İçindekiler

1. [Gereksinimler](#gereksinimler)
2. [Qt Creator ile açıp çalıştırma](#qt-creator-ile-açıp-çalıştırma)
3. [Komut satırından derleme](#komut-satırından-derleme)
4. [Kontroller](#kontroller)
5. [Oyun kuralları](#oyun-kuralları)
6. [Proje yapısı](#proje-yapısı)
7. [Test](#test)
8. [Özelleştirme](#özelleştirme)
9. [Geliştirici seçenekleri](#geliştirici-seçenekleri)
10. [Sorun giderme](#sorun-giderme)

## Gereksinimler

| Bileşen | Sürüm |
|---|---|
| Qt | 6.4 veya üstü (Core, Gui, Widgets, OpenGLWidgets modülleri) |
| CMake | 3.16 veya üstü |
| C++ derleyici | C++17 destekli (GCC, Clang, MSVC) |
| Grafik | OpenGL 3.3 core destekli ekran kartı/sürücü |

Qt'yi [Qt Online Installer](https://www.qt.io/download-qt-installer) ile kurabilirsiniz. Kurulumda **Qt 6.x → Desktop** kitini ve **Qt Creator**'ı seçin. Bu proje Qt 6.4.2 ile derlenip test edilmiştir.

## Qt Creator ile açıp çalıştırma

1. Repoyu indirin:
   ```bash
   git clone https://github.com/Mehmet-Eskol/cutebot_game.git
   ```
   Git kullanmıyorsanız GitHub sayfasında **Code → Download ZIP** ile indirip klasöre çıkarın.
2. Qt Creator'ı açın ve **File → Open File or Project…** menüsünü seçin.
3. Reponun kökündeki **`CMakeLists.txt`** dosyasını seçin.
4. **Configure Project** ekranında bir Qt 6 kiti (ör. *Desktop Qt 6.x GCC 64bit* ya da *MSVC*) seçip **Configure Project**'e basın.
5. Sol alttaki derleme türünü **Release** yapın (Debug'da oyun daha yavaş çalışır).
6. Sol alttaki **Run** hedef listesinden **`volt`** seçin.
7. **Ctrl+R** ile (ya da yeşil ▶ düğmesiyle) derleyip çalıştırın.

Hedef listesinde iki çalıştırılabilir dosya görünür:

- `volt`: oyunun kendisi.
- `volt_sim`: pencere açmadan seviyenin geçilebilirliğini sınayan test programı.

## Komut satırından derleme

**Linux (Ubuntu/Debian)**
```bash
sudo apt install build-essential cmake qt6-base-dev libqt6opengl6-dev libgl1-mesa-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/volt
```

**Windows / macOS:** Qt'yi kurun ve CMake'e Qt'nin yerini gösterin:
```bash
cmake -B build -DCMAKE_PREFIX_PATH=<Qt kurulum klasörü>/6.x/<kit>
cmake --build build --config Release
```

**Raspberry Pi vb.:** OpenGL ES 3.0 ile çalıştırmak için `./volt --gles` komutunu kullanın. Bu yol derlenip denenmemiştir. Masaüstü OpenGL 3.3 yolu denenmiştir.

## Kontroller

| Tuş | İşlev |
|---|---|
| `W A S D` | Hareket (kameraya göre) |
| `Boşluk` | Zıpla. Art arda zıplayınca 2. ve 3. zıplama daha yüksek olur, 3.'de takla atılır |
| `Shift` | Koş |
| `F` (veya `J`, `X`) | Dönme saldırısı |
| `←` `→` / `Q` `E` / fare sürükleme | Kamerayı döndür (`↑` `↓` eğim, fare tekerleği yakınlık) |
| `P` / `Esc` | Duraklat |
| `R` | Yeniden başla |
| `H` | Yardım |
| `F11` | Tam ekran |

## Oyun kuralları

- Amaç yolu geçip **portala** ulaşmaktır. Portal, **20 enerji hücresi** toplanınca açılır. Seviyede toplam 69 puanlık hücre vardır (büyük hücreler 5 değerindedir).
- Düşmanları **üstlerine zıplayarak** ya da **dönme saldırısıyla** yenin. Dikenli tuzaklar kırmızı yanıp sönerek uyarır.
- 3 canınız (pil) vardır. Düşmek 1 can götürür ve sizi son kontrol noktasına döndürür.
- Bonus platformlara (3 adet, büyük hücre içerir) art arda zıplayarak çıkılır.

## Proje yapısı

```
.
├── CMakeLists.txt        Derleme tanımı (Qt Creator bu dosyayı açar)
├── src/
│   ├── main.cpp          Giriş noktası, pencere ve komut satırı seçenekleri
│   ├── Level.*           Platformlar, hücreler, düşmanlar, tuzaklar, seviye yerleşimi (buildLevel())
│   ├── Player.*          Fizik: eksen eksen AABB çarpışma, coyote time, zıplama tamponu, zincirli zıplama
│   ├── Game.*            Oyun mantığı: durumlar, etkileşimler, parçacıklar, kamera (OpenGL'den bağımsız)
│   ├── Mesh.*            Yordamsal şekiller (kutu, süperelipsoid, silindir, koni, halka) ve VAO/VBO
│   ├── Renderer.*        Shader'lar (Blinn-Phong, sis, panel dokusu, gökyüzü) ve çizim yardımcıları
│   ├── RobotModel.*      VOLT'un hiyerarşik animasyonu (yürüme, koşma, zıplama, düşme, dönme, takla)
│   └── GameWidget.*      QOpenGLWidget: giriş, dünya çizimi, HUD (QPainter)
└── tests/
    └── sim_test.cpp      Başsız test: bot seviyeyi bitirebiliyor mu, bonus platformlara çıkılabiliyor mu
```

Derleme üç hedeften oluşur:

- `volt_core`: oyun mantığı kütüphanesi. OpenGL'e bağlı değildir, bu yüzden test edilebilir.
- `volt`: OpenGL çizimi ve pencere.
- `volt_sim`: simülasyon testi. CMake'te `-DVOLT_BUILD_TESTS=OFF` ile kapatılabilir.

## Test

```bash
./build/volt_sim
```

OpenGL ya da ekran gerekmez. Program iki şeyi sınar:

1. Bir bot seviyeyi baştan sona geçebiliyor mu?
2. 3 bonus platforma zincirli zıplamayla çıkılabiliyor mu?

Her şey yolundaysa çıktının sonunda `TÜM TESTLER GEÇTİ` yazar. Seviyeyi değiştirdiyseniz bu testi çalıştırın.

## Özelleştirme

- **Zorluk ve oyun hissi:** `Player.h` içindeki `PC::` sabitleri (yürüme/koşma hızı), `Player.cpp` içindeki zıplama hızları (`jv[]`) ve yerçekimi.
- **Seviye:** `Level.cpp` içindeki `buildLevel()`. `plat()`, `mover()`, `coin*()`, `enemy()` ve `spike()` yardımcılarını kullanın. Değişiklikten sonra `volt_sim` ile geçilebilirliği doğrulayın.
- **Robot görünümü:** `RobotModel.cpp` içindeki renk paleti ve parça ölçüleri.
- **Kendi Blender modeliniz:** glTF olarak dışa aktarıp yükleyebilir ve `drawRobot()` yerine koyabilirsiniz. İskelet animasyonu için bu yolda kendi glTF yükleyicinizi yazmanız gerekir.

## Geliştirici seçenekleri

Qt Creator'da **Projects → Run → Command line arguments** alanına yazabilirsiniz.

| Seçenek | Anlamı |
|---|---|
| `--shot dosya.png [kare]` | Belirtilen kare sonunda ekran görüntüsü alıp çıkar |
| `--demo` | Demo modu |
| `--at x,y,z` | Başlangıç konumu |
| `--cam yaw,pitch,mesafe` | Kamera açısı (derece, derece, metre) |
| `--autostart` | Menüyü atlayıp oyunu başlat |
| `--gles` | OpenGL ES 3.0 kullan |

## Sorun giderme

- **`Could not find a package configuration file provided by "Qt6"`:** Qt Creator'da bir Qt 6 kiti seçili değil. *Projects → Build & Run* bölümünden bir Qt 6 kiti seçin. Komut satırında `-DCMAKE_PREFIX_PATH` verin.
- **`Qt6OpenGLWidgets` bulunamıyor (Linux):** `sudo apt install libqt6opengl6-dev` komutunu çalıştırın.
- **Pencere siyah açılıyor ya da OpenGL hatası veriyor:** Ekran kartı sürücünüzü güncelleyin. OpenGL 3.3 core gerekir. Eski ya da gömülü kartlarda `--gles` seçeneğini deneyin.
- **Oyun yavaş çalışıyor:** Derleme türünü **Release** yapın.

## Yazar ve iletişim

Bu proje Mehmet Eşkol tarafından yazılmıştır.

İletişim için [mehmeteskol@gmail.com](mailto:mehmeteskol@gmail.com) adresine e-posta atabilirsiniz.

## Lisans

Bu proje [BSD 3-Clause](LICENSE) lisansı ile dağıtılmaktadır.
