#pragma once
#include "Game.h"
#include "Renderer.h"
#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QPoint>
#include <QSet>

class QPainter;

class GameWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit GameWidget(QWidget* parent = nullptr);
    ~GameWidget() override;

    // Geliştirici seçenekleri (komut satırından)
    void setScreenshot(const QString& path, int afterFrames) { shotPath_ = path; shotFrames_ = afterFrames; }
    void setAutoStart(bool on) { autoStart_ = on; }
    void setDebugView(const QVector3D& at, float yaw, float pitch, float dist, bool hasAt) { dbgAt_ = at; dbgYaw_ = yaw; dbgPitch_ = pitch; dbgDist_ = dist; dbgHasAt_ = hasAt; }
    void setDemo(bool on) { demo_ = on; autoStart_ = autoStart_ || on; }

protected:
    void initializeGL() override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;

private:
    Input makeInput();
    void renderWorld();
    void drawEnemy(const Enemy& e, float t);
    void drawHud(QPainter& p);

    Renderer r_;
    Game game_;
    QElapsedTimer clock_;
    qint64 lastNs_ = 0;
    QSet<int> keys_;
    bool jumpEdge_ = false, spinEdge_ = false;
    QPoint lastMouse_;
    bool showHelp_ = true;
    float helpTimer_ = 14.f;
    float fps_ = 60.f;

    QString shotPath_;
    int shotFrames_ = 0, frameCount_ = 0;
    bool autoStart_ = false, demo_ = false;
    float demoTime_ = 0.f;
    QVector3D dbgAt_;
    float dbgYaw_ = 0.f, dbgPitch_ = -1.f, dbgDist_ = -1.f;
    bool dbgHasAt_ = false, dbgApplied_ = false;
};
