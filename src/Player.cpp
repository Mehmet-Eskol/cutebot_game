#include "Player.h"
#include <QVector2D>
#include <QtMath>
#include <algorithm>
#include <cmath>

using namespace PC;

static float approach(float v, float target, float maxDelta)
{
    if (v < target) return std::min(v + maxDelta, target);
    return std::max(v - maxDelta, target);
}

void Player::reset(const QVector3D& start)
{
    *this = Player();
    pos = start;
}

void Player::step(float dt, const Input& in, const Level& lv)
{
    // --- zamanlayıcılar ---
    coyote -= dt;
    jumpBuf -= dt;
    chainTimer -= dt;
    spinCd -= dt;
    if (spinTimer > 0.f) spinTimer -= dt;
    if (invuln > 0.f) invuln -= dt;
    if (stun > 0.f) stun -= dt;
    const bool ctrl = stun <= 0.f;

    if (in.jumpPressed) jumpBuf = 0.13f;
    if (ctrl && in.spinPressed && spinCd <= 0.f) {
        spinTimer = kSpinTime;
        spinCd = 0.65f;
        evSpin = true;
        if (!onGround && vel.y() < 3.f) vel.setY(3.f); // havada küçük bir "tutunma"
    }

    // --- yatay hareket ---
    QVector2D v(vel.x(), vel.z());
    const QVector2D wish(in.dirX, in.dirZ);
    const float mag = std::min(1.f, wish.length());
    if (ctrl) {
        QVector2D target(0, 0);
        if (mag > 0.01f) target = wish.normalized() * ((in.run ? kRun : kWalk) * mag);
        const float acc = mag > 0.01f ? (onGround ? 80.f : 38.f) : (onGround ? 60.f : 5.f);
        QVector2D d = target - v;
        const float len = d.length();
        if (len > acc * dt) d *= acc * dt / len;
        v += d;
        if (mag > 0.1f) {
            const float targetYaw = qRadiansToDegrees(std::atan2(wish.x(), wish.y()));
            const float diff = std::fmod(targetYaw - yaw + 540.f, 360.f) - 180.f;
            yaw += diff * std::min(1.f, dt * 16.f);
        }
    }
    vel.setX(v.x());
    vel.setZ(v.y());

    // --- zıplama (Mario tarzı: 1-2-3 zincir, ağırlıklı yerçekimi, değişken yükseklik) ---
    if (jumpBuf > 0.f && (onGround || coyote > 0.f) && ctrl) {
        const float hs = std::hypot(vel.x(), vel.z());
        if (chainTimer > 0.f && hs > 2.5f) chain = std::min(chain + 1, 2);
        else chain = 0;
        static const float jv[3] = {11.0f, 12.6f, 15.2f};
        vel.setY(jv[chain]);
        jumping = true;
        jumpBuf = 0.f;
        coyote = 0.f;
        onGround = false;
        evJump = true;
        squash = -0.14f;
        chainTimer = 0.f;
        if (chain == 2) { flipping = true; flipAngle = 0.f; }
    }
    if (jumping && !in.jumpHeld && vel.y() > 0.f) { vel.setY(vel.y() * 0.5f); jumping = false; }
    if (vel.y() <= 0.f) jumping = false;
    if (flipping) {
        flipAngle += dt * (360.f / 0.75f);
        if (flipAngle >= 360.f) { flipAngle = 0.f; flipping = false; }
    }

    // --- yerçekimi ---
    vel.setY(std::max(-40.f, vel.y() - (vel.y() < 0.f ? 44.f : 30.f) * dt));

    // --- çarpışma: eksen eksen ---
    auto overlaps = [&](const AABB& b) {
        return pos.x() + kHW > b.min.x() && pos.x() - kHW < b.max.x() &&
               pos.y() + kH > b.min.y() && pos.y() < b.max.y() &&
               pos.z() + kHW > b.min.z() && pos.z() - kHW < b.max.z();
    };

    // X
    pos.setX(pos.x() + vel.x() * dt);
    for (const auto& pl : lv.platforms) {
        const AABB b = pl.box();
        if (!overlaps(b)) continue;
        const float rise = b.max.y() - pos.y();
        if (onGround && rise > 0.f && rise <= kStepUp) { pos.setY(b.max.y()); continue; }
        const float c = (b.min.x() + b.max.x()) * 0.5f;
        const bool fromLeft = vel.x() != 0.f ? vel.x() > 0.f : pos.x() < c;
        pos.setX(fromLeft ? b.min.x() - kHW - 1e-4f : b.max.x() + kHW + 1e-4f);
        vel.setX(0.f);
    }
    // Z
    pos.setZ(pos.z() + vel.z() * dt);
    for (const auto& pl : lv.platforms) {
        const AABB b = pl.box();
        if (!overlaps(b)) continue;
        const float rise = b.max.y() - pos.y();
        if (onGround && rise > 0.f && rise <= kStepUp) { pos.setY(b.max.y()); continue; }
        const float c = (b.min.z() + b.max.z()) * 0.5f;
        const bool fromFront = vel.z() != 0.f ? vel.z() > 0.f : pos.z() < c;
        pos.setZ(fromFront ? b.min.z() - kHW - 1e-4f : b.max.z() + kHW + 1e-4f);
        vel.setZ(0.f);
    }
    // Y
    const float preVy = vel.y();
    pos.setY(pos.y() + vel.y() * dt);
    const bool was = onGround;
    onGround = false;
    groundPlat = -1;
    for (int i = 0; i < int(lv.platforms.size()); ++i) {
        const AABB b = lv.platforms[size_t(i)].box();
        if (!overlaps(b)) continue;
        if (vel.y() <= 0.f) {
            pos.setY(b.max.y());
            vel.setY(0.f);
            onGround = true;
            groundPlat = i;
        } else {
            pos.setY(b.min.y() - kH);
            vel.setY(0.f);
        }
    }
    if (!was && onGround) {
        evLand = true;
        landSpeed = -preVy;
        chainTimer = 0.28f;
        flipping = false;
        flipAngle = 0.f;
        squash = std::min(0.22f, 0.04f + landSpeed * 0.011f);
    }
    if (was && !onGround && vel.y() <= 0.f) coyote = 0.1f;

    // --- animasyon durumu ---
    const float hs = std::hypot(vel.x(), vel.z());
    speedNorm = hs / kWalk;
    if (onGround) walkPhase += dt * hs * 1.9f;
    airBlend = approach(airBlend, onGround ? 0.f : 1.f, dt * 10.f);
    jumpPose = approach(jumpPose, vel.y() > 0.f ? 1.f : 0.f, dt * 8.f);
    squash *= std::exp(-dt * 12.f);
}
