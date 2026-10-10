#pragma once
#include "Level.h"
#include <QVector3D>

namespace PC {
constexpr float kHW = 0.32f;      // yarı genişlik
constexpr float kH = 1.8f;        // yükseklik
constexpr float kWalk = 6.0f;
constexpr float kRun = 10.0f;
constexpr float kStepUp = 0.32f;  // otomatik basamak çıkma
constexpr float kSpinTime = 0.45f;
} // namespace PC

struct Input {
    float dirX = 0.f, dirZ = 0.f; // dünya uzayında hareket yönü (uzunluk <= 1)
    bool run = false;
    bool jumpPressed = false;     // bu adımda basıldı
    bool jumpHeld = false;
    bool spinPressed = false;
};

struct Player {
    QVector3D pos{0, 0, 0};       // ayak merkezi
    QVector3D vel{0, 0, 0};
    float yaw = 180.f;            // derece; 0 = +Z'ye bakar
    bool onGround = false;
    int groundPlat = -1;
    bool visible = true;

    // oyun durumu
    int hp = 3;
    float invuln = 0.f;
    float stun = 0.f;
    float spinTimer = 0.f, spinCd = 0.f;

    // zıplama mantığı
    float coyote = 0.f, jumpBuf = 0.f, chainTimer = 0.f;
    int chain = 0;
    bool jumping = false;
    bool flipping = false;
    float flipAngle = 0.f;

    // animasyon
    float walkPhase = 0.f, speedNorm = 0.f, squash = 0.f, airBlend = 0.f, jumpPose = 0.f;

    // olaylar (Game tüketir)
    bool evJump = false, evLand = false, evSpin = false;
    float landSpeed = 0.f;

    void reset(const QVector3D& start);
    void step(float dt, const Input& in, const Level& lv);
};
