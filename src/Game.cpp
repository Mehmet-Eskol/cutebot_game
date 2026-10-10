#include "Game.h"
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>
#include <cmath>

using namespace PC;

static float frand(float a, float b) { return a + float(QRandomGenerator::global()->generateDouble()) * (b - a); }

Game::Game() { newGame(); }

void Game::newGame()
{
    level = buildLevel();
    player.reset(level.start);
    player.yaw = 180.f;
    particles.clear();
    animTime = runTime = winTime = 0.f;
    cells = kills = 0;
    respawn = level.start;
    toast.clear();
    toastTimer = 0.f;
    acc_ = 0.f;
    camYaw = 0.f;
    camPitch = 0.38f;
    camTarget = player.pos + QVector3D(0, 1.4f, 0);
    state = State::Title;
}

void Game::startPlaying()
{
    if (state == State::Title) {
        camYaw = 0.f;
        state = State::Playing;
        showToast(QStringLiteral("Portala ulaşmak için enerji hücrelerini topla!"), 3.f);
    }
}

void Game::togglePause()
{
    if (state == State::Playing) state = State::Paused;
    else if (state == State::Paused) state = State::Playing;
}

void Game::update(float dt, Input in)
{
    if (state == State::Paused) return;
    acc_ += std::min(dt, 0.05f);
    int guard = 0;
    while (acc_ >= kStep && guard++ < 12) {
        stepFixed(kStep, in);
        in.jumpPressed = false; // kenar tetikli girişler yalnızca ilk adımda geçerli
        in.spinPressed = false;
        acc_ -= kStep;
    }
    if (toastTimer > 0.f) toastTimer -= dt;
}

void Game::showToast(const QString& s, float t)
{
    toast = s;
    toastTimer = t;
}

void Game::burst(const QVector3D& p, const QVector3D& color, int n, float speed, float size, float life)
{
    for (int i = 0; i < n; ++i) {
        Particle q;
        const float a = frand(0.f, 6.2831853f);
        const float r = frand(0.2f, 1.f);
        q.vel = QVector3D(std::cos(a) * r, frand(0.2f, 1.1f), std::sin(a) * r) * speed * frand(0.5f, 1.f);
        q.pos = p;
        q.color = color;
        q.maxLife = q.life = life * frand(0.6f, 1.f);
        q.size = size * frand(0.6f, 1.2f);
        particles.push_back(q);
    }
    if (particles.size() > 600) particles.erase(particles.begin(), particles.begin() + 200);
}

void Game::hurtPlayer(const QVector3D& from)
{
    if (invincible || player.invuln > 0.f || state != State::Playing) return;
    player.hp--;
    player.invuln = 1.8f;
    player.stun = 0.25f;
    QVector3D away = player.pos - from;
    away.setY(0.f);
    if (away.length() < 0.01f) away = QVector3D(0, 0, 1);
    away.normalize();
    player.vel = QVector3D(away.x() * 7.f, 8.f, away.z() * 7.f);
    player.onGround = false;
    burst(player.pos + QVector3D(0, 1.0f, 0), QVector3D(1.f, 0.45f, 0.1f), 14, 5.f, 0.12f);
    if (player.hp <= 0) {
        state = State::Lost;
        player.visible = false;
        burst(player.pos + QVector3D(0, 1.0f, 0), QVector3D(0.2f, 0.9f, 1.f), 40, 8.f, 0.16f, 1.2f);
    }
}

void Game::respawnPlayer()
{
    player.pos = respawn;
    player.vel = QVector3D(0, 0, 0);
    player.onGround = false;
    player.stun = 0.f;
    player.invuln = 1.5f;
    camTarget = player.pos + QVector3D(0, 1.4f, 0);
}

void Game::updateEnemies(float dt)
{
    for (auto& e : level.enemies) {
        if (!e.alive) {
            if (e.deadTimer > 0.f) e.deadTimer -= dt;
            continue;
        }
        const QVector3D target = e.dir > 0 ? e.b : e.a;
        QVector3D d = target - e.pos;
        const float dist = d.length();
        const float step = e.speed * dt;
        if (dist <= step) {
            e.pos = target;
            e.dir = -e.dir;
        } else {
            d /= dist;
            e.pos += d * step;
            e.yaw = qRadiansToDegrees(std::atan2(d.x(), d.z()));
        }
    }
}

void Game::stepFixed(float dt, const Input& in)
{
    animTime += dt;
    level.update(animTime);
    updateEnemies(dt);

    if (state == State::Playing) {
        // hareketli platformun taşıması
        if (player.onGround && player.groundPlat >= 0)
            player.pos += level.platforms[size_t(player.groundPlat)].delta;

        runTime += dt;
        player.step(dt, in, level);

        // --- oyuncu olayları -> parçacıklar ---
        const QVector3D feet = player.pos + QVector3D(0, 0.05f, 0);
        if (player.evJump) {
            burst(feet, QVector3D(0.8f, 0.85f, 0.95f), 6, 2.f, 0.09f, 0.4f);
            player.evJump = false;
        }
        if (player.evLand) {
            burst(feet, QVector3D(0.8f, 0.85f, 0.95f), std::min(12, 4 + int(player.landSpeed * 0.4f)), 2.5f, 0.09f, 0.4f);
            player.evLand = false;
        }
        if (player.evSpin) {
            burst(player.pos + QVector3D(0, 0.9f, 0), QVector3D(0.2f, 0.95f, 1.f), 16, 6.f, 0.1f, 0.5f);
            player.evSpin = false;
        }

        const QVector3D body = player.pos + QVector3D(0, 0.95f, 0);

        // --- hücreler ---
        for (auto& c : level.coins) {
            if (c.taken) continue;
            const float r = c.value > 1 ? 1.3f : 0.9f;
            if ((c.pos - body).lengthSquared() < r * r) {
                c.taken = true;
                cells += c.value;
                burst(c.pos, c.value > 1 ? QVector3D(1.f, 0.9f, 0.3f) : QVector3D(1.f, 0.8f, 0.15f),
                      c.value > 1 ? 18 : 7, 3.5f, 0.1f, 0.5f);
            }
        }

        // --- düşmanlar ---
        for (auto& e : level.enemies) {
            if (!e.alive) continue;
            bool killed = false;
            const float dx = player.pos.x() - e.pos.x();
            const float dz = player.pos.z() - e.pos.z();
            const float hd = std::sqrt(dx * dx + dz * dz);

            if (player.spinTimer > 0.f) {
                const QVector3D ec = e.pos + QVector3D(0, 0.45f, 0);
                if ((ec - body).length() < 2.1f) {
                    killed = true;
                }
            }
            if (!killed && hd < kHW + 0.55f && player.pos.y() < e.pos.y() + 0.85f && player.pos.y() + kH > e.pos.y()) {
                if (player.vel.y() < 0.f && player.pos.y() > e.pos.y() + 0.45f) {
                    killed = true;
                    player.vel.setY(in.jumpHeld ? 13.5f : 9.5f);
                    player.onGround = false;
                    player.jumping = in.jumpHeld;
                    player.chainTimer = 0.f;
                } else {
                    hurtPlayer(e.pos);
                }
            }
            if (killed) {
                e.alive = false;
                e.deadTimer = 0.5f;
                kills++;
                burst(e.pos + QVector3D(0, 0.5f, 0), QVector3D(1.f, 0.35f, 0.15f), 20, 5.f, 0.13f, 0.7f);
            }
            if (state != State::Playing) return;
        }

        // --- diken tuzakları ---
        for (const auto& s : level.spikes) {
            if (s.ext < 0.6f) continue;
            if (std::fabs(player.pos.x() - s.pos.x()) < kHW + 0.6f && std::fabs(player.pos.z() - s.pos.z()) < kHW + 0.6f &&
                player.pos.y() < s.pos.y() + 0.7f && player.pos.y() + kH > s.pos.y())
                hurtPlayer(s.pos);
        }
        if (state != State::Playing) return;

        // --- kontrol noktaları ---
        for (auto& cp : level.checkpoints) {
            if (cp.active) continue;
            const float dx = player.pos.x() - cp.pos.x(), dz = player.pos.z() - cp.pos.z();
            if (std::sqrt(dx * dx + dz * dz) < 1.6f && std::fabs(player.pos.y() - cp.pos.y()) < 2.f) {
                cp.active = true;
                respawn = cp.pos + QVector3D(0, 0.05f, 0);
                burst(cp.pos + QVector3D(0, 1.f, 0), QVector3D(0.3f, 1.f, 0.5f), 18, 4.f, 0.1f, 0.8f);
                showToast(QStringLiteral("Kontrol noktası kaydedildi"));
            }
        }

        // --- hedef portal ---
        {
            const QVector3D g = level.goal.pos;
            const float dx = player.pos.x() - g.x(), dz = player.pos.z() - g.z();
            if (std::sqrt(dx * dx + dz * dz) < level.goal.radius && player.pos.y() > g.y() - 0.6f && player.pos.y() < g.y() + 2.5f) {
                if (cells >= level.requiredCells) {
                    state = State::Won;
                    winTime = runTime;
                    burst(g + QVector3D(0, 1.7f, 0), QVector3D(0.3f, 1.f, 0.9f), 60, 9.f, 0.16f, 1.4f);
                    burst(g + QVector3D(0, 1.7f, 0), QVector3D(1.f, 0.85f, 0.3f), 40, 7.f, 0.14f, 1.2f);
                    return;
                } else if (toastTimer <= 0.f) {
                    showToast(QStringLiteral("Portal kilitli: %1 hücre daha lazım").arg(level.requiredCells - cells), 1.6f);
                }
            }
        }

        // --- düşme ---
        if (player.pos.y() < level.killY) {
            if (invincible) {
                respawnPlayer();
            } else {
                player.hp--;
                if (player.hp <= 0) {
                    state = State::Lost;
                    player.visible = false;
                } else {
                    respawnPlayer();
                    showToast(QStringLiteral("Düştün! Son kontrol noktasına dönüldü"));
                }
            }
        }
    }

    // --- parçacıklar ---
    for (auto& p : particles) {
        p.pos += p.vel * dt;
        p.vel.setY(p.vel.y() - 12.f * dt);
        p.life -= dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0.f; }),
                    particles.end());
}

void Game::updateCamera(float dt)
{
    if (state == State::Title) camYaw += dt * 0.25f;
    if (state != State::Lost) {
        const QVector3D want = player.pos + QVector3D(0, 1.4f, 0);
        const float k = 1.f - std::exp(-dt * 7.f);
        camTarget += (want - camTarget) * k;
    }
}

QVector3D Game::camPos() const
{
    const float cp = std::cos(camPitch);
    return camTarget + QVector3D(std::sin(camYaw) * cp, std::sin(camPitch), std::cos(camYaw) * cp) * camDist;
}
