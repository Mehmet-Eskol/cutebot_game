#pragma once
#include "Mesh.h"
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>

// Küçük bir OpenGL 3.3 core / GLES 3.0 çizim katmanı: shader'lar, mesh kütüphanesi, gökyüzü.
class Renderer : public QOpenGLFunctions {
public:
    struct Material {
        QVector3D color{1, 1, 1};
        QVector3D emissive{0, 0, 0};
        float shine = 16.f, spec = 0.25f, alpha = 1.f, rim = 0.f, pattern = 0.f;
        int mode = 0; // 0 = aydınlatmalı, 1 = ışıksız, 2 = yumuşak blob gölge
    };

    // birim şekiller
    Mesh box, sphere, roundBox, cylinder, cone, torus, disc;

    void init();    // geçerli GL bağlamı gerekir
    void destroy(); // bağlam hâlâ geçerliyken GL kaynaklarını serbest bırakır
    void beginFrame(const QMatrix4x4& view, const QMatrix4x4& proj, const QVector3D& camPos, float time);
    void drawSky();
    void begin3D();
    void draw(const Mesh& m, const QMatrix4x4& model, const Material& mat);
    void setDepthWrite(bool on) { glDepthMask(on ? GL_TRUE : GL_FALSE); }
    void setCull(bool on);
    void endFrame();

    static QVector3D sunDir();
    static QVector3D fogColor() { return QVector3D(0.78f, 0.88f, 1.0f); }

private:
    QOpenGLShaderProgram prog_, sky_;
    QOpenGLVertexArrayObject emptyVao_;
    QMatrix4x4 vp_, view_, proj_;
    QVector3D camPos_;
    float time_ = 0.f;
    int locModel_ = -1, locNormal_ = -1, locColor_ = -1, locEmissive_ = -1, locShine_ = -1, locSpec_ = -1,
        locAlpha_ = -1, locRim_ = -1, locPattern_ = -1, locMode_ = -1;
};
