// Başsız simülasyon testleri (OpenGL gerekmez):
//  1) Bot seviyeyi baştan sona geçip portala girebiliyor mu?
//  2) Bonus platformlara (zincirleme zıplama ile) çıkılabiliyor mu?
#include "Game.h"
#include <cmath>
#include <cstdio>
#include <vector>

struct WP { int plat; float x, z; };

// Bot: hedefe doğru koşar, kenarda zıplar. 'chainHack' açıkken yerdeyken zincir penceresini açık tutar.
static bool drive(Game& g, const std::vector<WP>& wps, float maxT, bool chainHack, bool verbose)
{
    int cur = 0;
    float t = 0.f;
    const float dt = Game::kStep;
    const int n = int(wps.size());
    while (t < maxT && g.state == Game::State::Playing) {
        Player& p = g.player;
        const WP& w = wps[size_t(cur)];
        float tx = w.x, tz = w.z;
        const Platform& tp = g.level.platforms[size_t(w.plat)];
        if (tp.moving()) { tx = tp.center.x(); tz = tp.center.z(); }
        const float dx = tx - p.pos.x(), dz = tz - p.pos.z();
        const float dist = std::hypot(dx, dz);
        Input in;
        in.run = true;
        in.jumpHeld = true;
        if (dist > 0.05f) { in.dirX = dx / dist; in.dirZ = dz / dist; }
        if (chainHack && p.onGround) { p.chainTimer = 0.2f; p.chain = p.chain; }
        if (p.onGround && dist > 0.8f) {
            const float ax = p.pos.x() + in.dirX * 0.55f, az = p.pos.z() + in.dirZ * 0.55f;
            if (g.level.groundBelow(ax, az, p.pos.y() + 0.4f) < p.pos.y() - 0.5f) in.jumpPressed = true;
        }
        g.update(dt, in);
        t += dt;
        const bool last = cur == n - 1;
        if (p.onGround && p.groundPlat == w.plat && dist < (last ? 0.9f : 1.2f)) {
            if (verbose) std::printf("  t=%6.2f  waypoint %2d (platform %2d) tamam, hücre=%d\n", t, cur, w.plat, g.cells);
            if (last) return true;
            ++cur;
        }
        if (p.pos.y() < -20.f) {
            if (verbose) std::printf("  t=%6.2f  DÜŞTÜ waypoint %d (%.1f, %.1f, %.1f)\n", t, cur, p.pos.x(), p.pos.y(), p.pos.z());
            return false;
        }
    }
    return g.state == Game::State::Won;
}

int main()
{
    int failed = 0;

    // ---------- 1) Tam geçiş ----------
    {
        std::printf("[1] Ana yol + arena çevresi hücreleri + portal\n");
        Game g;
        g.invincible = true;
        g.startPlaying();
        std::vector<WP> wps = {
            {0, 0, -5}, {1, 0, -11}, {2, 3, -17}, {3, -2, -23}, {4, 0, -30}, {5, 0, -40},
            {6, 0, -49}, {7, 0, -54}, {8, 0, -59}, {9, 0, -66}, {10, 0, -82}, {11, 0, -95},
        };
        // arena çevresindeki hücre halkası (r=5.5, merkez z=-95)
        for (int i = 0; i < 10; ++i) {
            const float a = 6.2831853f * float(i) / 10.f;
            wps.push_back({11, 5.5f * std::cos(a), -95.f + 5.5f * std::sin(a)});
        }
        wps.push_back({11, 0, -100.5f}); // portal
        const bool ok = drive(g, wps, 150.f, false, true) && g.state == Game::State::Won;
        std::printf("  -> %s  (durum=%d, hücre=%d/%d, süre=%.1fs)\n", ok ? "BAŞARILI" : "BAŞARISIZ", int(g.state), g.cells,
                    g.level.totalCells, g.runTime);
        if (!ok) ++failed;
    }

    // ---------- 2) Bonus platformlar (ikinci zıplama = zincir 1) ----------
    struct Bonus { const char* name; int fromPlat; QVector3D start; int toPlat; float tx, tz; };
    const Bonus bonuses[] = {
        {"B1 (-7, 3.2, -12)  <- platform 1", 1, {0.5f, 0.8f, -11.f}, 12, -7.f, -12.f},
        {"B2 (8.5, 5.4, -43) <- platform 5", 5, {3.0f, 3.0f, -43.f}, 13, 8.5f, -43.f},
        {"B3 (-10, 9, -96)   <- arena",       11, {-3.0f, 7.0f, -96.f}, 14, -10.f, -96.f},
    };
    std::printf("[2] Bonus platformlar (zincirli zıplama)\n");
    for (const auto& b : bonuses) {
        Game g;
        g.invincible = true;
        g.startPlaying();
        g.player.pos = b.start;
        g.player.vel = QVector3D(0, 0, 0);
        std::vector<WP> wps = {{b.toPlat, b.tx, b.tz}};
        const bool ok = drive(g, wps, 6.f, true, false);
        std::printf("  %-36s -> %s\n", b.name, ok ? "ULAŞILDI" : "ULAŞILAMADI");
        if (!ok) ++failed;
    }

    std::printf("\n%s\n", failed == 0 ? "TÜM TESTLER GEÇTİ" : "BAZI TESTLER BAŞARISIZ");
    return failed == 0 ? 0 : 1;
}
