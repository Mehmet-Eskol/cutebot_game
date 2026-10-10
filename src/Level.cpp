#include "Level.h"
#include <QRandomGenerator>
#include <algorithm>

void Level::update(float t)
{
    for (auto& p : platforms) p.update(t);
    for (auto& d : decor) d.update(t);
    for (auto& s : spikes) {
        // 3.2 sn'lik döngü: 0-1.4 açık, 1.4-1.6 iniş, 1.6-2.8 kapalı (2.2'den itibaren uyarı), 2.8-3.2 çıkış
        const float c = std::fmod(t + s.phase, 3.2f);
        s.warn = (c > 2.1f && c < 2.8f);
        if (c < 1.4f) s.ext = 1.f;
        else if (c < 1.6f) s.ext = 1.f - (c - 1.4f) / 0.2f;
        else if (c < 2.8f) s.ext = 0.f;
        else s.ext = (c - 2.8f) / 0.4f;
    }
}

float Level::groundBelow(float x, float z, float maxY) const
{
    float best = -1000.f;
    for (const auto& p : platforms) {
        const AABB b = p.box();
        if (x >= b.min.x() && x <= b.max.x() && z >= b.min.z() && z <= b.max.z()) {
            const float top = b.max.y();
            if (top <= maxY + 0.05f) best = std::max(best, top);
        }
    }
    return best;
}

Level buildLevel()
{
    Level L;

    auto plat = [&](float cx, float topY, float cz, float w, float d, QVector3D col, float thick = 2.0f) {
        Platform p;
        p.size = QVector3D(w, thick, d);
        p.origin = p.center = QVector3D(cx, topY - thick * 0.5f, cz);
        p.color = col;
        L.platforms.push_back(p);
        return int(L.platforms.size()) - 1;
    };
    auto mover = [&](float cx, float topY, float cz, float w, float d, QVector3D axis, float amp, float speed,
                     QVector3D col = QVector3D(0.25f, 0.55f, 0.92f)) {
        const int i = plat(cx, topY, cz, w, d, col, 1.0f);
        L.platforms[i].axis = axis;
        L.platforms[i].amp = amp;
        L.platforms[i].speed = speed;
        return i;
    };
    auto coin = [&](float x, float y, float z, int value = 1) {
        Coin c;
        c.pos = QVector3D(x, y, z);
        c.value = value;
        c.phase = float(L.coins.size()) * 0.7f;
        L.coins.push_back(c);
    };
    auto coinLine = [&](QVector3D a, QVector3D b, int n) {
        for (int i = 0; i < n; ++i) {
            const float t = n > 1 ? float(i) / float(n - 1) : 0.f;
            const QVector3D p = a + (b - a) * t;
            coin(p.x(), p.y(), p.z());
        }
    };
    auto coinRing = [&](QVector3D c, float r, int n) {
        for (int i = 0; i < n; ++i) {
            const float a = 6.2831853f * float(i) / float(n);
            coin(c.x() + r * std::cos(a), c.y(), c.z() + r * std::sin(a));
        }
    };
    auto enemy = [&](QVector3D a, QVector3D b, float speed) {
        Enemy e;
        e.a = a; e.b = b; e.pos = a; e.speed = speed;
        e.phase = float(L.enemies.size()) * 1.3f;
        L.enemies.push_back(e);
    };
    auto spike = [&](QVector3D p, float phase) {
        SpikeTrap s;
        s.pos = p; s.phase = phase;
        L.spikes.push_back(s);
    };

    const QVector3D green(0.30f, 0.68f, 0.45f), sand(0.90f, 0.66f, 0.36f), slate(0.42f, 0.52f, 0.78f),
        rose(0.86f, 0.42f, 0.52f), mint(0.36f, 0.80f, 0.72f), violet(0.58f, 0.46f, 0.86f);

    // --- Ana yol (bu sıra test botu için sabit: indeksler 0..11) ---------------------
    plat(0, 0, 0, 14, 14, green);              // 0 başlangıç adası
    plat(0, 0.8f, -11, 5, 5, sand);            // 1
    plat(3, 1.8f, -17, 4, 4, sand);            // 2
    plat(-2, 3.0f, -23, 4, 4, sand);           // 3
    mover(0, 3.0f, -30, 4, 4, QVector3D(1, 0, 0), 5.0f, 0.9f); // 4 yatay hareketli
    plat(0, 3.0f, -40, 12, 12, slate);         // 5 büyük platform
    plat(0, 4.0f, -49, 4, 4, rose);            // 6
    plat(0, 5.0f, -54, 4, 4, rose);            // 7
    plat(0, 6.0f, -59, 4, 4, rose);            // 8
    mover(0, 6.5f, -66, 4, 4, QVector3D(0, 1, 0), 1.2f, 1.1f); // 9 dikey hareketli
    plat(0, 7.0f, -78, 2.6f, 14, violet);      // 10 köprü
    plat(0, 7.0f, -95, 16, 16, mint);          // 11 son arena

    // --- Bonus platformlar (üçlü zıplama ile) ---------------------------------------
    plat(-7.0f, 3.2f, -12, 3, 3, mint, 1.0f);  // 12
    plat(8.5f, 5.4f, -43, 3, 3, mint, 1.0f);   // 13
    plat(-10.0f, 9.0f, -96, 3, 3, mint, 1.0f); // 14

    // --- Hücreler ----------------------------------------------------------------------
    coinLine({0, 1.0f, -1}, {0, 1.0f, -5.5f}, 4);
    coinLine({-1, 1.8f, -11}, {1, 1.8f, -11}, 3);
    coinLine({2, 2.8f, -17}, {4, 2.8f, -17}, 3);
    coinLine({-3, 4.0f, -23}, {-1, 4.0f, -23}, 3);
    coinLine({-3, 5.2f, -27}, {3, 5.2f, -33}, 4);       // hareketli platform üstü
    coinRing({0, 4.0f, -40}, 3.4f, 8);
    coinLine({0, 4.0f, -36}, {0, 4.0f, -44}, 4);
    coin(0, 5.0f, -49); coin(0, 6.0f, -54); coin(0, 7.0f, -59);
    coin(0, 8.6f, -66); coin(0, 8.6f, -63);
    coinLine({0, 8.0f, -72}, {0, 8.0f, -84}, 6);
    coinRing({0, 8.0f, -95}, 5.5f, 10);
    coin(-7.0f, 4.4f, -12, 5);   // büyük hücreler (bonus platformlar)
    coin(8.5f, 6.6f, -43, 5);
    coin(-10.0f, 10.2f, -96, 5);
    coinLine({-7.5f, 4.2f, -11}, {-6.5f, 4.2f, -13}, 2);
    coinLine({8.0f, 6.4f, -42}, {9.0f, 6.4f, -44}, 2);

    // --- Düşmanlar ----------------------------------------------------------------------
    enemy({-4, 0, -3}, {4, 0, -3}, 1.6f);                  // öğretici düşman
    enemy({-4, 3.0f, -37}, {4, 3.0f, -37}, 2.2f);
    enemy({4, 3.0f, -43}, {-4, 3.0f, -43}, 2.8f);
    enemy({0, 7.0f, -73}, {0, 7.0f, -83}, 2.0f);           // köprü
    enemy({-6, 7.0f, -91.5f}, {6, 7.0f, -91.5f}, 3.0f);
    enemy({6, 7.0f, -98.5f}, {-6, 7.0f, -98.5f}, 3.5f);

    // --- Dikenli tuzaklar ------------------------------------------------------------------
    spike({0, 3.0f, -40}, 0.0f);
    spike({-3, 3.0f, -34.5f}, 1.2f);
    spike({3, 7.0f, -95}, 0.0f);
    spike({-3, 7.0f, -95}, 1.6f);

    // --- Kontrol noktaları -----------------------------------------------------------------
    L.checkpoints.push_back({{-4.5f, 3.0f, -44.5f}, false});
    L.checkpoints.push_back({{6.0f, 7.0f, -89.5f}, false});

    // --- Hedef portal ----------------------------------------------------------------------
    L.goal.pos = QVector3D(0, 7.0f, -101);
    L.goal.radius = 1.5f;
    L.start = QVector3D(0, 0, 3);

    for (const auto& c : L.coins) L.totalCells += c.value;

    // --- Süs adaları (çarpışmasız) ---------------------------------------------------------
    QRandomGenerator rng(1234);
    for (int i = 0; i < 46; ++i) {
        Platform d;
        const float side = (i % 2 == 0) ? 1.f : -1.f;
        const float x = side * (22.f + float(rng.generateDouble()) * 55.f);
        const float z = -150.f + float(rng.generateDouble()) * 190.f;
        const float y = -14.f + float(rng.generateDouble()) * 30.f;
        const float s = 3.f + float(rng.generateDouble()) * 9.f;
        d.size = QVector3D(s, 1.f + s * 0.45f, s);
        d.origin = d.center = QVector3D(x, y, z);
        d.color = QVector3D(0.30f + float(rng.generateDouble()) * 0.25f,
                            0.38f + float(rng.generateDouble()) * 0.25f,
                            0.62f + float(rng.generateDouble()) * 0.25f);
        d.axis = QVector3D(0, 1, 0);
        d.amp = 0.6f + float(rng.generateDouble()) * 1.2f;
        d.speed = 0.3f + float(rng.generateDouble()) * 0.5f;
        d.phase = float(rng.generateDouble()) * 6.28f;
        L.decor.push_back(d);
    }

    L.update(0.f);
    return L;
}
