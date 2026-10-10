#pragma once
#include "Level.h"
#include "Player.h"
#include <QString>
#include <QVector3D>
#include <vector>

struct Particle {
    QVector3D pos, vel, color;
    float life = 0.f, maxLife = 1.f, size = 0.1f;
};

class Game {
public:
    enum class State { Title, Playing, Paused, Won, Lost };

    Game();

    void newGame();            // seviyeyi sıfırla (başlık ekranına döner)
    void startPlaying();
    void togglePause();
    void update(float dt, Input in);     // sabit adımlı simülasyon
    void updateCamera(float dt);
    QVector3D camPos() const;

    // --- veri (render ve HUD okur) ---
    State state = State::Title;
    Level level;
    Player player;
    std::vector<Particle> particles;
    float animTime = 0.f, runTime = 0.f, winTime = 0.f;
    int cells = 0, kills = 0;
    QVector3D respawn;
    QString toast;
    float toastTimer = 0.f;
    bool invincible = false; // yalnızca test için

    // kamera
    float camYaw = 0.f, camPitch = 0.38f, camDist = 9.5f;
    QVector3D camTarget;

    static constexpr float kStep = 1.f / 120.f;

private:
    void stepFixed(float dt, const Input& in);
    void updateEnemies(float dt);
    void hurtPlayer(const QVector3D& from);
    void respawnPlayer();
    void burst(const QVector3D& p, const QVector3D& color, int n, float speed, float size, float life = 0.7f);
    void showToast(const QString& s, float t = 2.2f);

    float acc_ = 0.f;
};
