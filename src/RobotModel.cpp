#include "RobotModel.h"
#include <QMatrix4x4>
#include <algorithm>
#include <cmath>

namespace {
using Mat = Renderer::Material;

inline float mixf(float a, float b, float t) { return a + (b - a) * t; }

Mat mk(const QVector3D& c, float shine = 40.f, float spec = 0.5f, const QVector3D& em = QVector3D(0, 0, 0), float rim = 0.25f)
{
    Mat m;
    m.color = c;
    m.shine = shine;
    m.spec = spec;
    m.emissive = em;
    m.rim = rim;
    return m;
}

// M * T(t) * S(s) ile çiz
void part(Renderer& r, const Mesh& mesh, QMatrix4x4 M, const QVector3D& t, const QVector3D& s, const Mat& mat)
{
    M.translate(t);
    M.scale(s);
    r.draw(mesh, M, mat);
}
} // namespace

void drawRobot(Renderer& r, const Player& p, float t)
{
    // ---- renk paleti ----
    const QVector3D orange(0.97f, 0.47f, 0.10f), white(0.93f, 0.95f, 0.98f), steel(0.45f, 0.50f, 0.60f),
        dark(0.13f, 0.15f, 0.21f), navy(0.04f, 0.07f, 0.15f), cyan(0.10f, 0.95f, 1.0f);
    const Mat mOrange = mk(orange), mWhite = mk(white), mSteel = mk(steel, 60.f, 0.7f), mDark = mk(dark, 30.f, 0.4f),
              mVisor = mk(navy, 140.f, 1.0f, QVector3D(0, 0, 0), 0.5f);
    const float pulse = 0.65f + 0.35f * std::sin(t * 4.f);
    const Mat mGlow = mk(QVector3D(0.05f, 0.2f, 0.22f), 10.f, 0.f, cyan * (pulse * 1.15f), 0.f);

    // ---- poz hesabı ----
    const bool spinning = p.spinTimer > 0.f;
    const float sp = std::min(p.speedNorm, 1.7f);
    const float spc = std::min(sp, 1.2f);
    const float ph = p.walkPhase;
    const float idle = 1.f - std::min(sp, 1.f);
    const float sway = std::sin(t * 1.7f);

    // yerde
    float thL = std::sin(ph) * 34.f * std::min(sp, 1.15f), thR = -thL;
    float knL = std::max(0.f, std::cos(ph)) * 62.f * spc, knR = std::max(0.f, -std::cos(ph)) * 62.f * spc;
    float arL = -thL * 1.1f + sway * 3.f * idle, arR = -thR * 1.1f - sway * 3.f * idle;
    float elL = 12.f + 28.f * std::min(sp, 1.f), elR = elL;
    float abd = 6.f + 4.f * sway * idle;
    float lean = std::min(sp, 1.3f) * 9.f;

    // havada
    if (p.airBlend > 0.001f) {
        const float jp = p.jumpPose;
        // yükselirken: sağ yumruk yukarı, sol bacak öne bükülü
        const float rThL = 48, rKnL = 75, rThR = -18, rKnR = 30, rArL = -25, rArR = 150, rElL = 20, rElR = 25, rAbd = 12, rLean = 6;
        // düşerken: kollar açık, bacaklar hafif açılmış
        const float fThL = 22, fKnL = 35, fThR = -8, fKnR = 30, fArL = 45, fArR = 45, fElL = 15, fElR = 15, fAbd = 62, fLean = 2;
        const float aTh1 = mixf(fThL, rThL, jp), aKn1 = mixf(fKnL, rKnL, jp), aTh2 = mixf(fThR, rThR, jp),
                    aKn2 = mixf(fKnR, rKnR, jp), aAr1 = mixf(fArL, rArL, jp), aAr2 = mixf(fArR, rArR, jp),
                    aEl1 = mixf(fElL, rElL, jp), aEl2 = mixf(fElR, rElR, jp), aAbd = mixf(fAbd, rAbd, jp),
                    aLean = mixf(fLean, rLean, jp);
        const float b = p.airBlend;
        thL = mixf(thL, aTh1, b); knL = mixf(knL, aKn1, b); thR = mixf(thR, aTh2, b); knR = mixf(knR, aKn2, b);
        arL = mixf(arL, aAr1, b); arR = mixf(arR, aAr2, b); elL = mixf(elL, aEl1, b); elR = mixf(elR, aEl2, b);
        abd = mixf(abd, aAbd, b); lean = mixf(lean, aLean, b);
    }
    // dönme saldırısı: kollar yatay açık
    float spinAngle = 0.f;
    if (spinning) {
        const float k = 1.f - p.spinTimer / PC::kSpinTime;
        spinAngle = k * 720.f;
        arL = arR = 0.f; elL = elR = 0.f; abd = 88.f; thL = 12.f; thR = -12.f; knL = knR = 20.f; lean = 0.f;
    }

    const float bob = std::sin(ph * 2.f) * 0.035f * std::min(sp, 1.f) * (1.f - p.airBlend);
    const float breath = std::sin(t * 2.2f) * 0.01f * idle;
    const float blink = std::fmod(t, 3.4f) > 3.27f ? 0.15f : 1.f;

    // ---- kök matris ----
    QMatrix4x4 root;
    root.translate(p.pos);
    root.rotate(p.yaw, 0, 1, 0);
    if (p.flipAngle > 0.f) { // üçüncü zıplamada ileri takla
        root.translate(0, 0.95f, 0);
        root.rotate(p.flipAngle, 1, 0, 0);
        root.translate(0, -0.95f, 0);
    }
    if (spinning) root.rotate(spinAngle, 0, 1, 0);
    root.scale(1.f + p.squash * 0.5f, 1.f - p.squash, 1.f + p.squash * 0.5f);

    // ---- gövde ----
    QMatrix4x4 U = root; // üst gövde (öne eğilir)
    U.translate(0, bob + breath, 0);
    U.translate(0, 0.82f, 0);
    U.rotate(lean, 1, 0, 0);
    U.rotate(std::sin(ph) * 6.f * std::min(sp, 1.f), 0, 1, 0);
    U.translate(0, -0.82f, 0);

    QMatrix4x4 Bt = root; // kalça seviyesi (eğilmez)
    Bt.translate(0, bob, 0);

    part(r, r.roundBox, Bt, {0, 0.84f, 0}, {0.40f, 0.20f, 0.26f}, mDark);                   // pelvis
    part(r, r.roundBox, U, {0, 1.14f, 0}, {0.58f, 0.50f, 0.38f}, mOrange);                  // gövde
    part(r, r.roundBox, U, {0, 1.18f, 0.17f}, {0.42f, 0.30f, 0.12f}, mWhite);               // göğüs plakası
    part(r, r.sphere, U, {0, 1.18f, 0.235f}, {0.17f, 0.17f, 0.17f}, mGlow);                 // enerji çekirdeği
    part(r, r.roundBox, U, {0, 1.16f, -0.26f}, {0.34f, 0.38f, 0.14f}, mDark);               // sırt çantası
    part(r, r.box, U, {0, 1.24f, -0.335f}, {0.20f, 0.05f, 0.02f}, mGlow);                   // havalandırma şeridi
    part(r, r.cylinder, U, {0, 1.42f, 0}, {0.14f, 0.10f, 0.14f}, mSteel);                   // boyun
    for (int s = -1; s <= 1; s += 2)                                                         // omuz pedleri
        part(r, r.sphere, U, {s * 0.38f, 1.32f, 0}, {0.24f, 0.20f, 0.24f}, mOrange);

    // ---- baş ----
    QMatrix4x4 H = U;
    H.translate(0, 1.44f, 0);
    H.rotate(std::sin(t * 0.7f) * 8.f * idle, 0, 1, 0);        // etrafa bakınma
    H.rotate(-lean * 0.5f, 1, 0, 0);                            // koşarken başı dik tut
    part(r, r.roundBox, H, {0, 0.22f, 0}, {0.50f, 0.42f, 0.46f}, mWhite);                  // kafa
    part(r, r.roundBox, H, {0, 0.24f, 0.17f}, {0.40f, 0.22f, 0.18f}, mVisor);               // siperlik
    for (int s = -1; s <= 1; s += 2)                                                         // gözler
        part(r, r.roundBox, H, {s * 0.095f, 0.25f, 0.262f}, {0.11f, 0.075f * blink, 0.03f},
             mk(QVector3D(0.1f, 0.3f, 0.3f), 10.f, 0.f, cyan * 1.3f, 0.f));
    for (int s = -1; s <= 1; s += 2) {                                                       // kulak ünitesi
        QMatrix4x4 E = H;
        E.translate(s * 0.27f, 0.2f, 0);
        E.rotate(90, 0, 0, 1);
        E.scale(0.20f, 0.07f, 0.20f);
        r.draw(r.cylinder, E, mOrange);
    }
    part(r, r.cylinder, H, {0.12f, 0.50f, -0.05f}, {0.025f, 0.18f, 0.025f}, mSteel);        // anten
    part(r, r.sphere, H, {0.12f, 0.62f, -0.05f}, {0.075f, 0.075f, 0.075f},
         mk(QVector3D(0.3f, 0.12f, 0.05f), 10.f, 0.f, QVector3D(1.f, 0.5f, 0.1f) * (0.5f + 0.5f * pulse), 0.f));

    // ---- kollar ----
    for (int s = -1; s <= 1; s += 2) {
        const float swing = s > 0 ? arL : arR, elbow = s > 0 ? elL : elR;
        QMatrix4x4 A = U;
        A.translate(s * 0.40f, 1.30f, 0);
        A.rotate(-swing, 1, 0, 0);
        A.rotate(s * abd, 0, 0, 1);
        part(r, r.cylinder, A, {0, -0.17f, 0}, {0.11f, 0.34f, 0.11f}, mSteel);               // üst kol
        part(r, r.sphere, A, {0, -0.34f, 0}, {0.14f, 0.14f, 0.14f}, mDark);                  // dirsek
        QMatrix4x4 Fa = A;
        Fa.translate(0, -0.34f, 0);
        Fa.rotate(-elbow, 1, 0, 0);
        part(r, r.cylinder, Fa, {0, -0.15f, 0}, {0.125f, 0.30f, 0.125f}, mOrange);           // ön kol
        part(r, r.cylinder, Fa, {0, -0.27f, 0}, {0.15f, 0.04f, 0.15f}, mGlow);               // bilek halkası
        part(r, r.sphere, Fa, {0, -0.34f, 0}, {0.17f, 0.17f, 0.17f}, mDark);                 // el
    }

    // ---- bacaklar ----
    for (int s = -1; s <= 1; s += 2) {
        const float th = s > 0 ? thL : thR, kn = s > 0 ? knL : knR;
        QMatrix4x4 L = Bt;
        L.translate(s * 0.16f, 0.82f, 0);
        L.rotate(-th, 1, 0, 0);
        part(r, r.cylinder, L, {0, -0.19f, 0}, {0.14f, 0.38f, 0.14f}, mWhite);               // uyluk
        part(r, r.sphere, L, {0, -0.38f, 0}, {0.16f, 0.16f, 0.16f}, mDark);                  // diz
        QMatrix4x4 K = L;
        K.translate(0, -0.38f, 0);
        K.rotate(kn, 1, 0, 0);
        part(r, r.cylinder, K, {0, -0.18f, 0}, {0.13f, 0.36f, 0.13f}, mOrange);              // baldır
        QMatrix4x4 F = K;
        F.translate(0, -0.36f, 0);
        F.rotate((th - kn) * 0.8f, 1, 0, 0);                                                  // ayağı yere paralel tut
        part(r, r.roundBox, F, {0, -0.02f, 0.07f}, {0.22f, 0.11f, 0.36f}, mDark);             // ayak
        part(r, r.box, F, {0, -0.065f, 0.09f}, {0.16f, 0.015f, 0.30f}, mGlow);               // taban ışığı
    }
}
