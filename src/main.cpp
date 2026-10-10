#include "GameWidget.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QStringList>
#include <QtMath>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    const QStringList args = QCoreApplication::arguments();

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    fmt.setSamples(4);
    fmt.setSwapInterval(1);
    if (args.contains("--gles")) { // Raspberry Pi vb. için: OpenGL ES 3.0
        fmt.setRenderableType(QSurfaceFormat::OpenGLES);
        fmt.setVersion(3, 0);
    } else {
        fmt.setRenderableType(QSurfaceFormat::OpenGL);
        fmt.setVersion(3, 3);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
    }
    QSurfaceFormat::setDefaultFormat(fmt);

    GameWidget w;
    w.setWindowTitle(QStringLiteral("VOLT — Devre Macerası"));
    w.resize(1280, 720);

    // geliştirici seçenekleri: --shot dosya.png [kare]  --autostart
    const int si = args.indexOf("--shot");
    if (si >= 0 && si + 1 < args.size()) {
        const int frames = (si + 2 < args.size() && args[si + 2].toInt() > 0) ? args[si + 2].toInt() : 90;
        w.setScreenshot(args[si + 1], frames);
    }
    {   // --at x,y,z  --cam yaw,pitch,dist  (derece, derece, metre)
        QVector3D at; bool hasAt = false; float yaw = 0, pitch = -1, dist = -1;
        const int ai = args.indexOf("--at");
        if (ai >= 0 && ai + 1 < args.size()) {
            const QStringList v = args[ai + 1].split(',');
            if (v.size() == 3) { at = QVector3D(v[0].toFloat(), v[1].toFloat(), v[2].toFloat()); hasAt = true; }
        }
        const int ci = args.indexOf("--cam");
        if (ci >= 0 && ci + 1 < args.size()) {
            const QStringList v = args[ci + 1].split(',');
            if (v.size() == 3) { yaw = qDegreesToRadians(v[0].toFloat()); pitch = qDegreesToRadians(v[1].toFloat()); dist = v[2].toFloat(); }
        }
        if (hasAt || ci >= 0) { w.setDebugView(at, yaw, pitch, dist, hasAt); w.setAutoStart(true); }
    }
    if (args.contains("--autostart")) w.setAutoStart(true);
    if (args.contains("--demo")) w.setDemo(true);

    w.show();
    return app.exec();
}
