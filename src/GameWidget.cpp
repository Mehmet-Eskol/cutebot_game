#include "GameWidget.h"
#include "RobotModel.h"
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>
#include <cmath>

using Mat = Renderer::Material;

static Mat lit(const QVector3D& c, float shine = 16.f, float spec = 0.25f, const QVector3D& em = QVector3D(0, 0, 0), float rim = 0.f)
{
    Mat m;
    m.color = c; m.shine = shine; m.spec = spec; m.emissive = em; m.rim = rim;
    return m;
}
static Mat glow(const QVector3D& c, float alpha = 1.f)
{
    Mat m;
    m.color = QVector3D(0, 0, 0);
    m.emissive = c;
    m.mode = 1;
    m.alpha = alpha;
    return m;
}

GameWidget::GameWidget(QWidget* parent) : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    // Sürekli render: her kare takas edilince bir sonrakini iste (V-Sync ile eşzamanlı)
    connect(this, &QOpenGLWidget::frameSwapped, this, qOverload<>(&QWidget::update));
}

GameWidget::~GameWidget()
{
    makeCurrent();
    r_.destroy();
    doneCurrent();
}

void GameWidget::initializeGL()
{
    r_.init();
    clock_.start();
    lastNs_ = clock_.nsecsElapsed();
}

// ---------------------------------------------------------------------------
// Giriş
// ---------------------------------------------------------------------------
Input GameWidget::makeInput()
{
    Input in;
    float fwd = 0, rt = 0;
    if (keys_.contains(Qt::Key_W)) fwd += 1;
    if (keys_.contains(Qt::Key_S)) fwd -= 1;
    if (keys_.contains(Qt::Key_D)) rt += 1;
    if (keys_.contains(Qt::Key_A)) rt -= 1;
    const float s = std::sin(game_.camYaw), c = std::cos(game_.camYaw);
    // ileri = (-sin, 0, -cos), sağ = (cos, 0, -sin)
    QVector3D d(-s * fwd + c * rt, 0, -c * fwd - s * rt);
    if (d.length() > 1.f) d.normalize();
    in.dirX = d.x();
    in.dirZ = d.z();
    in.run = keys_.contains(Qt::Key_Shift);
    in.jumpHeld = keys_.contains(Qt::Key_Space);
    in.jumpPressed = jumpEdge_;
    in.spinPressed = spinEdge_;
    jumpEdge_ = spinEdge_ = false;
    if (demo_) { // geliştirici demo modu: ileri koş, aralıklı zıpla, bir kez dön
        in.dirX = -s; in.dirZ = -c; in.run = true; in.jumpHeld = true;
        const float ph = std::fmod(demoTime_, 1.4f);
        in.jumpPressed = ph < 0.02f;
        in.spinPressed = demoTime_ > 5.f && demoTime_ < 5.05f;
    }
    return in;
}

void GameWidget::keyPressEvent(QKeyEvent* e)
{
    if (e->isAutoRepeat()) return;
    keys_.insert(e->key());
    switch (e->key()) {
    case Qt::Key_Space:
        jumpEdge_ = true;
        if (game_.state == Game::State::Title) game_.startPlaying();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (game_.state == Game::State::Title) game_.startPlaying();
        else if (game_.state == Game::State::Won || game_.state == Game::State::Lost) { game_.newGame(); game_.startPlaying(); }
        break;
    case Qt::Key_F:
    case Qt::Key_J:
    case Qt::Key_X:
        spinEdge_ = true;
        break;
    case Qt::Key_P:
    case Qt::Key_Escape:
        game_.togglePause();
        break;
    case Qt::Key_R:
        game_.newGame();
        game_.startPlaying();
        break;
    case Qt::Key_H:
        showHelp_ = !showHelp_;
        helpTimer_ = 14.f;
        break;
    case Qt::Key_F11:
        window()->isFullScreen() ? window()->showNormal() : window()->showFullScreen();
        break;
    default:
        break;
    }
}

void GameWidget::keyReleaseEvent(QKeyEvent* e)
{
    if (!e->isAutoRepeat()) keys_.remove(e->key());
}

void GameWidget::focusOutEvent(QFocusEvent* e)
{
    keys_.clear();
    QOpenGLWidget::focusOutEvent(e);
}

void GameWidget::mousePressEvent(QMouseEvent* e) { lastMouse_ = e->pos(); }

void GameWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (e->buttons() & (Qt::LeftButton | Qt::RightButton)) {
        const QPoint d = e->pos() - lastMouse_;
        game_.camYaw -= float(d.x()) * 0.008f;
        game_.camPitch = std::clamp(game_.camPitch + float(d.y()) * 0.006f, 0.05f, 1.25f);
    }
    lastMouse_ = e->pos();
}

void GameWidget::wheelEvent(QWheelEvent* e)
{
    game_.camDist = std::clamp(game_.camDist - float(e->angleDelta().y()) * 0.004f, 4.f, 18.f);
}

// ---------------------------------------------------------------------------
// Kare döngüsü
// ---------------------------------------------------------------------------
void GameWidget::paintGL()
{
    const qint64 now = clock_.nsecsElapsed();
    const float dt = std::min(0.1f, float(double(now - lastNs_) * 1e-9));
    lastNs_ = now;
    if (dt > 0.f) fps_ += (1.f / dt - fps_) * 0.05f;

    if (autoStart_ && game_.state == Game::State::Title) game_.startPlaying();
    if (!dbgApplied_ && game_.state == Game::State::Playing) { // geliştirici: oyuncuyu/kamerayı yerleştir
        dbgApplied_ = true;
        if (dbgHasAt_) { game_.player.pos = dbgAt_; game_.camTarget = dbgAt_ + QVector3D(0, 1.4f, 0); game_.respawn = dbgAt_; }
        game_.camYaw = dbgYaw_;
        if (dbgPitch_ >= 0.f) game_.camPitch = dbgPitch_;
        if (dbgDist_ > 0.f) game_.camDist = dbgDist_;
    }

    // kamera klavyesi: ←/→ ve Q/E döndür, ↑/↓ eğim
    const float camSpeed = 2.0f;
    if (keys_.contains(Qt::Key_Left) || keys_.contains(Qt::Key_Q)) game_.camYaw -= camSpeed * dt;
    if (keys_.contains(Qt::Key_Right) || keys_.contains(Qt::Key_E)) game_.camYaw += camSpeed * dt;
    if (keys_.contains(Qt::Key_Up)) game_.camPitch = std::min(1.25f, game_.camPitch + 1.0f * dt);
    if (keys_.contains(Qt::Key_Down)) game_.camPitch = std::max(0.05f, game_.camPitch - 1.0f * dt);

    demoTime_ += dt;
    game_.update(dt, makeInput());
    game_.updateCamera(dt);
    if (helpTimer_ > 0.f && game_.state == Game::State::Playing) {
        helpTimer_ -= dt;
        if (helpTimer_ <= 0.f) showHelp_ = false;
    }

    renderWorld();

    QPainter p(this);
    drawHud(p);
    p.end();

    if (shotFrames_ > 0 && ++frameCount_ >= shotFrames_) {
        grabFramebuffer().save(shotPath_);
        shotFrames_ = 0;
        QTimer::singleShot(0, qApp, &QApplication::quit); // paintGL içinden doğrudan çıkma
    }
}

// ---------------------------------------------------------------------------
// Dünya çizimi
// ---------------------------------------------------------------------------
void GameWidget::renderWorld()
{
    const float aspect = float(width()) / float(std::max(1, height()));
    QMatrix4x4 proj;
    proj.perspective(58.f, aspect, 0.15f, 500.f);
    QMatrix4x4 view;
    const QVector3D cam = game_.camPos();
    view.lookAt(cam, game_.camTarget, QVector3D(0, 1, 0));
    const float t = game_.animTime;

    r_.beginFrame(view, proj, cam, t);
    const QVector3D fog = Renderer::fogColor();
    r_.glClearColor(fog.x(), fog.y(), fog.z(), 1.f);
    r_.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    r_.drawSky();
    r_.begin3D();

    const Level& L = game_.level;

    // --- süs adaları ---
    for (const auto& d : L.decor) {
        QMatrix4x4 M;
        M.translate(d.center);
        M.scale(d.size);
        Mat m = lit(d.color, 8.f, 0.1f);
        m.pattern = 1.f;
        r_.draw(r_.box, M, m);
    }

    // --- platformlar ---
    for (const auto& pl : L.platforms) {
        QMatrix4x4 M;
        M.translate(pl.center);
        M.scale(pl.size);
        Mat m = lit(pl.color, 10.f, 0.12f);
        m.pattern = 1.f;
        if (pl.moving()) m.emissive = QVector3D(0.0f, 0.06f, 0.12f);
        r_.draw(r_.box, M, m);
        if (pl.moving()) { // üst yüzde parlayan şerit
            QMatrix4x4 S;
            S.translate(pl.center + QVector3D(0, pl.size.y() * 0.5f + 0.015f, 0));
            S.scale(pl.size.x() * 0.86f, 0.03f, pl.size.z() * 0.86f);
            r_.draw(r_.box, S, glow(QVector3D(0.1f, 0.55f, 0.75f)));
        }
    }

    // --- hücreler ---
    for (const auto& c : L.coins) {
        if (c.taken) continue;
        const float k = c.value > 1 ? 1.9f : 1.f;
        const float bob = std::sin(t * 2.5f + c.phase) * 0.12f;
        QMatrix4x4 M;
        M.translate(c.pos + QVector3D(0, bob, 0));
        M.rotate(t * 160.f + c.phase * 40.f, 0, 1, 0);
        M.rotate(90, 1, 0, 0);
        QMatrix4x4 Mo = M;
        Mo.scale(0.55f * k, 0.09f * k, 0.55f * k);
        r_.draw(r_.cylinder, Mo, lit(QVector3D(1.f, 0.78f, 0.15f), 60.f, 0.8f, QVector3D(0.55f, 0.38f, 0.02f)));
        QMatrix4x4 Mi = M;
        Mi.scale(0.30f * k, 0.12f * k, 0.30f * k);
        r_.draw(r_.cylinder, Mi, glow(QVector3D(0.2f, 0.95f, 1.0f)));
    }

    // --- dikenli tuzaklar ---
    for (const auto& s : L.spikes) {
        QMatrix4x4 B;
        B.translate(s.pos + QVector3D(0, 0.06f, 0));
        B.scale(1.5f, 0.12f, 1.5f);
        r_.draw(r_.box, B, lit(QVector3D(0.25f, 0.27f, 0.33f), 30.f, 0.4f));
        if (s.warn) {
            QMatrix4x4 W;
            W.translate(s.pos + QVector3D(0, 0.13f, 0));
            W.scale(1.3f, 1.f, 1.3f);
            r_.draw(r_.disc, W, glow(QVector3D(1.f, 0.1f, 0.05f) * (0.5f + 0.5f * std::sin(t * 24.f))));
        }
        const float h = 0.9f * s.ext;
        if (h > 0.02f)
            for (int ix = -1; ix <= 1; ix += 2)
                for (int iz = -1; iz <= 1; iz += 2) {
                    QMatrix4x4 C;
                    C.translate(s.pos + QVector3D(ix * 0.35f, 0.12f + h * 0.5f, iz * 0.35f));
                    C.scale(0.30f, h, 0.30f);
                    r_.draw(r_.cone, C, lit(QVector3D(0.85f, 0.88f, 0.95f), 90.f, 1.0f));
                }
    }

    // --- kontrol noktaları ---
    for (const auto& cp : L.checkpoints) {
        const QVector3D col = cp.active ? QVector3D(0.2f, 1.f, 0.45f) : QVector3D(0.6f, 0.62f, 0.7f);
        QMatrix4x4 P;
        P.translate(cp.pos + QVector3D(0, 1.0f, 0));
        P.scale(0.08f, 2.0f, 0.08f);
        r_.draw(r_.cylinder, P, lit(QVector3D(0.7f, 0.72f, 0.8f), 60.f, 0.8f));
        QMatrix4x4 F;
        F.translate(cp.pos + QVector3D(0.38f, 1.7f + (cp.active ? 0.05f * std::sin(t * 5.f) : 0.f), 0));
        F.scale(0.7f, 0.4f, 0.05f);
        r_.draw(r_.box, F, cp.active ? glow(col) : lit(col, 20.f, 0.2f));
        QMatrix4x4 Bs;
        Bs.translate(cp.pos + QVector3D(0, 0.04f, 0));
        Bs.scale(1.2f, 0.08f, 1.2f);
        r_.draw(r_.cylinder, Bs, cp.active ? glow(col * 0.7f) : lit(QVector3D(0.35f, 0.37f, 0.45f)));
    }

    // --- hedef portalı ---
    {
        const bool open = game_.cells >= L.requiredCells;
        const float pulse = 0.6f + 0.4f * std::sin(t * 3.f);
        const QVector3D ringCol = open ? QVector3D(0.2f, 1.f, 0.9f) : QVector3D(1.f, 0.2f, 0.15f);
        const QVector3D c = L.goal.pos;
        for (int s = -1; s <= 1; s += 2) { // dikmeler
            QMatrix4x4 P;
            P.translate(c + QVector3D(s * 1.9f, 1.4f, 0));
            P.scale(0.35f, 2.8f, 0.35f);
            r_.draw(r_.box, P, lit(QVector3D(0.22f, 0.25f, 0.34f), 40.f, 0.6f));
            QMatrix4x4 T;
            T.translate(c + QVector3D(s * 1.9f, 2.95f, 0));
            T.scale(0.28f, 0.28f, 0.28f);
            r_.draw(r_.sphere, T, glow(ringCol * pulse));
        }
        QMatrix4x4 R;
        R.translate(c + QVector3D(0, 1.7f, 0));
        R.rotate(t * 40.f, 0, 0, 1);
        R.scale(3.4f);
        r_.draw(r_.torus, R, glow(ringCol * (0.5f + 0.5f * pulse)));
        QMatrix4x4 R2;
        R2.translate(c + QVector3D(0, 1.7f, 0));
        R2.rotate(-t * 70.f, 0, 0, 1);
        R2.scale(2.7f);
        r_.draw(r_.torus, R2, glow(ringCol * 0.55f));
    }

    // --- düşmanlar ---
    for (const auto& e : L.enemies) {
        if (!e.alive && e.deadTimer <= 0.f) continue;
        drawEnemy(e, t);
    }

    // --- oyuncu ---
    const Player& pl = game_.player;
    const bool blinkHide = pl.invuln > 0.f && std::fmod(t, 0.16f) < 0.08f;
    if (pl.visible && !blinkHide) drawRobot(r_, pl, t);

    // --- parçacıklar ---
    for (const auto& q : game_.particles) {
        const float k = std::max(0.f, q.life / q.maxLife);
        QMatrix4x4 M;
        M.translate(q.pos);
        M.rotate(q.life * 400.f, 1, 1, 0);
        M.scale(q.size * (0.3f + 0.7f * k));
        r_.draw(r_.box, M, glow(q.color));
    }

    // --- saydam öğeler (en son) ---
    r_.setDepthWrite(false);
    r_.setCull(false);
    {
        // blob gölge
        const float gy = L.groundBelow(pl.pos.x(), pl.pos.z(), pl.pos.y() + 0.1f);
        if (pl.visible && gy > -500.f) {
            const float h = std::max(0.f, pl.pos.y() - gy);
            const float rad = 0.95f / (1.f + h * 0.12f);
            QMatrix4x4 M;
            M.translate(pl.pos.x(), gy + 0.03f, pl.pos.z());
            M.scale(rad * 2.f, 1.f, rad * 2.f);
            Mat m;
            m.mode = 2;
            m.alpha = 0.5f / (1.f + h * 0.15f);
            r_.draw(r_.disc, M, m);
        }
        // portal yüzeyi
        const bool open = game_.cells >= L.requiredCells;
        const float pulse = 0.5f + 0.5f * std::sin(t * 3.f);
        QMatrix4x4 D;
        D.translate(L.goal.pos + QVector3D(0, 1.7f, 0));
        D.rotate(90, 1, 0, 0);
        D.scale(3.2f, 1.f, 3.2f);
        r_.draw(r_.disc, D,
                glow(open ? QVector3D(0.1f, 0.8f, 0.9f) : QVector3D(0.7f, 0.08f, 0.05f), open ? 0.28f + 0.2f * pulse : 0.16f));
    }
    r_.setCull(true);
    r_.setDepthWrite(true);
    r_.endFrame();
}

void GameWidget::drawEnemy(const Enemy& e, float t)
{
    const float bob = e.alive ? 0.08f * std::sin(t * 5.f + e.phase) : 0.f;
    QMatrix4x4 R;
    R.translate(e.pos);
    R.rotate(e.yaw, 0, 1, 0);
    if (!e.alive) R.scale(1.35f, std::max(0.1f, e.deadTimer * 0.35f), 1.35f); // ezildi

    auto part = [&](const Mesh& mesh, const QVector3D& tr, const QVector3D& sc, const Mat& m) {
        QMatrix4x4 M = R;
        M.translate(tr);
        M.scale(sc);
        r_.draw(mesh, M, m);
    };
    const float pulse = 0.55f + 0.45f * std::sin(t * 8.f + e.phase);
    part(r_.sphere, {0, 0.42f + bob, 0}, {1.05f, 0.82f, 1.05f}, lit(QVector3D(0.78f, 0.13f, 0.17f), 50.f, 0.7f, QVector3D(0, 0, 0), 0.2f));
    part(r_.cylinder, {0, 0.12f, 0}, {1.0f, 0.18f, 1.0f}, lit(QVector3D(0.13f, 0.15f, 0.21f), 30.f, 0.4f));
    part(r_.roundBox, {0, 0.50f + bob, 0.38f}, {0.62f, 0.22f, 0.30f}, lit(QVector3D(0.04f, 0.05f, 0.1f), 120.f, 1.f));
    part(r_.sphere, {0, 0.50f + bob, 0.5f}, {0.14f, 0.14f, 0.14f}, glow(QVector3D(1.f, 0.15f, 0.1f) * (0.6f + 0.6f * pulse)));
    for (int s = -1; s <= 1; s += 2) {
        part(r_.box, {s * 0.42f, 0.10f, 0}, {0.2f, 0.2f, 0.72f}, lit(QVector3D(0.13f, 0.15f, 0.21f), 30.f, 0.4f));
        part(r_.cone, {s * 0.30f, 0.90f + bob, -0.10f}, {0.16f, 0.34f, 0.16f}, lit(QVector3D(0.8f, 0.82f, 0.9f), 80.f, 0.9f));
    }
    part(r_.cone, {0, 0.98f + bob, 0}, {0.18f, 0.38f, 0.18f}, lit(QVector3D(0.8f, 0.82f, 0.9f), 80.f, 0.9f));
}

// ---------------------------------------------------------------------------
// HUD (QPainter)
// ---------------------------------------------------------------------------
static QString fmtTime(float s)
{
    const int m = int(s) / 60;
    const float sec = s - float(m * 60);
    return QStringLiteral("%1:%2").arg(m).arg(sec, 4, 'f', 1, QLatin1Char('0'));
}

void GameWidget::drawHud(QPainter& p)
{
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const int W = width(), H = height();
    const Level& L = game_.level;
    const Player& pl = game_.player;

    auto font = [&](int px, bool bold = true) {
        QFont f = p.font();
        f.setPixelSize(px);
        f.setBold(bold);
        return f;
    };
    auto shadowText = [&](const QRect& r, int flags, const QString& s, const QColor& c) {
        p.setPen(QColor(0, 0, 0, 160));
        p.drawText(r.translated(2, 2), flags, s);
        p.setPen(c);
        p.drawText(r, flags, s);
    };

    const bool playing = game_.state != Game::State::Title;
    if (playing) {
        // --- sol üst panel ---
        const QRect panel(16, 16, 290, 104);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(8, 12, 28, 165));
        p.drawRoundedRect(panel, 14, 14);
        // enerji pilleri (can)
        for (int i = 0; i < 3; ++i) {
            const QRect b(panel.left() + 16 + i * 44, panel.top() + 14, 36, 20);
            p.setPen(QPen(QColor(200, 220, 255), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(b, 5, 5);
            p.drawRect(QRect(b.right() + 1, b.top() + 6, 3, 8));
            if (i < pl.hp) {
                p.setPen(Qt::NoPen);
                p.setBrush(pl.hp == 1 ? QColor(255, 80, 60) : QColor(60, 235, 150));
                p.drawRoundedRect(b.adjusted(3, 3, -3, -3), 3, 3);
            }
        }
        // hücre sayacı
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 200, 40));
        p.drawEllipse(QPoint(panel.left() + 28, panel.top() + 58), 11, 11);
        p.setBrush(QColor(40, 230, 255));
        p.drawEllipse(QPoint(panel.left() + 28, panel.top() + 58), 5, 5);
        p.setFont(font(22));
        const bool open = game_.cells >= L.requiredCells;
        shadowText(QRect(panel.left() + 48, panel.top() + 42, 230, 32), Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1 / %2").arg(game_.cells).arg(L.totalCells), QColor(255, 235, 170));
        p.setFont(font(13, false));
        shadowText(QRect(panel.left() + 16, panel.top() + 76, 262, 22), Qt::AlignVCenter | Qt::AlignLeft,
                   open ? QStringLiteral("Portal AÇIK!  ·  süre %1").arg(fmtTime(game_.state == Game::State::Won ? game_.winTime : game_.runTime))
                        : QStringLiteral("Portal için %1 hücre  ·  süre %2").arg(L.requiredCells).arg(fmtTime(game_.runTime)),
                   open ? QColor(120, 255, 220) : QColor(200, 215, 240));

        // yardım
        if (showHelp_) {
            p.setFont(font(14, false));
            const QString help = QStringLiteral(
                "WASD hareket   ·   BOŞLUK zıpla (art arda: 2./3. zıplama daha yüksek)   ·   SHIFT koş   ·   F dönme saldırısı\n"
                "Düşmanın üstüne zıpla   ·   ←/→ veya fare sürükle: kamera   ·   P duraklat   ·   R yeniden   ·   F11 tam ekran   ·   H yardım");
            const QRect r(W / 2 - 480, H - 70, 960, 52);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(8, 12, 28, 150));
            p.drawRoundedRect(r, 12, 12);
            p.setPen(QColor(225, 235, 255));
            p.drawText(r, Qt::AlignCenter, help);
        }
        // toast
        if (game_.toastTimer > 0.f && !game_.toast.isEmpty()) {
            p.setFont(font(22));
            const float a = std::min(1.f, game_.toastTimer * 2.f);
            shadowText(QRect(0, 28, W, 40), Qt::AlignCenter, game_.toast, QColor(255, 255, 255, int(255 * a)));
        }
        // fps
        p.setFont(font(11, false));
        p.setPen(QColor(255, 255, 255, 120));
        p.drawText(QRect(W - 90, 10, 80, 18), Qt::AlignRight, QStringLiteral("%1 FPS").arg(int(fps_ + 0.5f)));
        // hasar vinyeti
        if (pl.invuln > 1.2f && game_.state == Game::State::Playing) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 40, 20, int(110 * (pl.invuln - 1.2f) / 0.6f)));
            p.drawRect(0, 0, W, H);
        }
    }

    auto overlay = [&](int alpha) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(5, 8, 20, alpha));
        p.drawRect(0, 0, W, H);
    };

    if (game_.state == Game::State::Title) {
        overlay(70);
        p.setFont(font(std::max(60, H / 8)));
        shadowText(QRect(0, H / 5, W, H / 6), Qt::AlignCenter, QStringLiteral("VOLT"), QColor(255, 150, 50));
        p.setFont(font(std::max(24, H / 24)));
        shadowText(QRect(0, H / 5 + H / 6, W, H / 14), Qt::AlignCenter, QStringLiteral("Devre Macerası"), QColor(120, 240, 255));
        p.setFont(font(22, false));
        const float blink = 0.55f + 0.45f * std::sin(game_.animTime * 3.f);
        shadowText(QRect(0, H * 3 / 4, W, 40), Qt::AlignCenter, QStringLiteral("BAŞLAMAK İÇİN  ENTER  VEYA  BOŞLUK"),
                   QColor(255, 255, 255, int(255 * blink)));
        p.setFont(font(15, false));
        shadowText(QRect(0, H * 3 / 4 + 44, W, 28), Qt::AlignCenter,
                   QStringLiteral("Qt 6 · OpenGL 3.3  —  WASD / BOŞLUK / SHIFT / F"), QColor(210, 225, 250));
    } else if (game_.state == Game::State::Paused) {
        overlay(120);
        p.setFont(font(54));
        shadowText(QRect(0, H / 3, W, 80), Qt::AlignCenter, QStringLiteral("DURAKLATILDI"), QColor(255, 255, 255));
        p.setFont(font(20, false));
        shadowText(QRect(0, H / 3 + 84, W, 36), Qt::AlignCenter, QStringLiteral("P veya ESC ile devam"), QColor(210, 225, 250));
    } else if (game_.state == Game::State::Won) {
        overlay(110);
        p.setFont(font(58));
        shadowText(QRect(0, H / 4, W, 80), Qt::AlignCenter, QStringLiteral("TEBRİKLER!"), QColor(120, 255, 220));
        p.setFont(font(24, false));
        shadowText(QRect(0, H / 4 + 90, W, 120), Qt::AlignHCenter | Qt::AlignTop,
                   QStringLiteral("Süre: %1\nHücre: %2 / %3\nYenilen düşman: %4")
                       .arg(fmtTime(game_.winTime)).arg(game_.cells).arg(L.totalCells).arg(game_.kills),
                   QColor(255, 255, 255));
        p.setFont(font(20, false));
        shadowText(QRect(0, H * 3 / 4, W, 36), Qt::AlignCenter, QStringLiteral("ENTER veya R: tekrar oyna"), QColor(210, 225, 250));
    } else if (game_.state == Game::State::Lost) {
        overlay(120);
        p.setFont(font(58));
        shadowText(QRect(0, H / 3, W, 80), Qt::AlignCenter, QStringLiteral("ENERJİ BİTTİ"), QColor(255, 100, 80));
        p.setFont(font(20, false));
        shadowText(QRect(0, H / 3 + 90, W, 36), Qt::AlignCenter, QStringLiteral("ENTER veya R: tekrar dene"), QColor(210, 225, 250));
    }
}
