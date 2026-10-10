#include "Renderer.h"
#include <QDebug>
#include <QOpenGLContext>

static QByteArray glslHeader()
{
    return QOpenGLContext::currentContext()->isOpenGLES()
               ? QByteArray("#version 300 es\nprecision highp float;\nprecision highp int;\n")
               : QByteArray("#version 330 core\n");
}

static const char* kVert = R"GLSL(
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uVP;
uniform mat3 uNormalMat;
out vec3 vWorld;
out vec3 vNormal;
out vec3 vLocal;
void main() {
    vec4 w = uModel * vec4(aPos, 1.0);
    vWorld = w.xyz;
    vNormal = normalize(uNormalMat * aNormal);
    vLocal = aPos;
    gl_Position = uVP * w;
}
)GLSL";

static const char* kFrag = R"GLSL(
in vec3 vWorld;
in vec3 vNormal;
in vec3 vLocal;
out vec4 FragColor;
uniform vec3 uColor;
uniform vec3 uEmissive;
uniform vec3 uCamPos;
uniform vec3 uSunDir;
uniform vec3 uFogColor;
uniform float uShine;
uniform float uSpec;
uniform float uAlpha;
uniform float uRim;
uniform float uPattern;
uniform int uMode;

void main() {
    if (uMode == 2) { // yumuşak blob gölge
        float r = length(vLocal.xz);
        FragColor = vec4(0.0, 0.0, 0.05, uAlpha * smoothstep(0.5, 0.1, r));
        return;
    }
    vec3 N = normalize(vNormal);
    vec3 base = uColor;
    if (uPattern > 0.5) { // yordamsal panel/ızgara dokusu
        vec2 uv = abs(N.y) > 0.5 ? vWorld.xz : (abs(N.x) > 0.5 ? vWorld.zy : vWorld.xy);
        vec2 f = fract(uv * 0.5);
        float e = min(min(f.x, 1.0 - f.x), min(f.y, 1.0 - f.y));
        float line = smoothstep(0.0, 0.035, e);
        vec2 c = floor(uv * 0.5);
        float chk = mod(c.x + c.y, 2.0);
        base *= mix(0.80, 1.0, line) * (0.95 + 0.05 * chk);
        if (abs(N.y) < 0.5) base *= 0.72;
    }
    vec3 col;
    if (uMode == 1) {
        col = base + uEmissive;
    } else {
        vec3 V = normalize(uCamPos - vWorld);
        float ndl = max(dot(N, uSunDir), 0.0);
        vec3 amb = mix(vec3(0.30, 0.32, 0.46), vec3(0.66, 0.76, 1.0), N.y * 0.5 + 0.5);
        col = base * (amb * 0.72 + vec3(1.0, 0.96, 0.88) * ndl * 0.95);
        vec3 H = normalize(uSunDir + V);
        col += vec3(1.0, 0.97, 0.9) * pow(max(dot(N, H), 0.0), uShine) * uSpec * step(0.001, ndl);
        col += uRim * pow(1.0 - max(dot(N, V), 0.0), 3.0) * vec3(0.45, 0.8, 1.0);
        col += uEmissive;
    }
    float d = length(uCamPos - vWorld);
    float fog = clamp(1.0 - exp(-d * 0.012), 0.0, 0.8);
    col = mix(col, uFogColor, fog);
    FragColor = vec4(col, uAlpha);
}
)GLSL";

static const char* kSkyVert = R"GLSL(
out vec2 vNdc;
void main() {
    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vNdc = p * 2.0 - 1.0;
    gl_Position = vec4(vNdc, 0.0, 1.0);
}
)GLSL";

static const char* kSkyFrag = R"GLSL(
in vec2 vNdc;
out vec4 FragColor;
uniform mat4 uInvVP;
uniform vec3 uSunDir;
void main() {
    vec4 n = uInvVP * vec4(vNdc, -1.0, 1.0);
    vec4 f = uInvVP * vec4(vNdc, 1.0, 1.0);
    vec3 dir = normalize(f.xyz / f.w - n.xyz / n.w);
    float h = dir.y;
    vec3 horizon = vec3(0.78, 0.88, 1.0);
    vec3 zenith = vec3(0.10, 0.30, 0.78);
    vec3 below = vec3(0.22, 0.30, 0.55);
    vec3 col = mix(horizon, zenith, pow(clamp(h, 0.0, 1.0), 0.55));
    if (h < 0.0) col = mix(horizon, below, pow(clamp(-h * 3.0, 0.0, 1.0), 0.6));
    float s = max(dot(dir, uSunDir), 0.0);
    col += vec3(1.0, 0.85, 0.55) * (pow(s, 600.0) * 3.0 + pow(s, 12.0) * 0.18);
    FragColor = vec4(col, 1.0);
}
)GLSL";

QVector3D Renderer::sunDir()
{
    return QVector3D(0.45f, 1.0f, 0.35f).normalized();
}

void Renderer::init()
{
    initializeOpenGLFunctions();

    auto build = [&](QOpenGLShaderProgram& p, const char* vs, const char* fs, const char* name) {
        const QByteArray hdr = glslHeader();
        p.addShaderFromSourceCode(QOpenGLShader::Vertex, hdr + vs);
        p.addShaderFromSourceCode(QOpenGLShader::Fragment, hdr + fs);
        if (!p.link()) qWarning() << "Shader link hatası (" << name << "):" << p.log();
    };
    build(prog_, kVert, kFrag, "main");
    build(sky_, kSkyVert, kSkyFrag, "sky");

    locModel_ = prog_.uniformLocation("uModel");
    locNormal_ = prog_.uniformLocation("uNormalMat");
    locColor_ = prog_.uniformLocation("uColor");
    locEmissive_ = prog_.uniformLocation("uEmissive");
    locShine_ = prog_.uniformLocation("uShine");
    locSpec_ = prog_.uniformLocation("uSpec");
    locAlpha_ = prog_.uniformLocation("uAlpha");
    locRim_ = prog_.uniformLocation("uRim");
    locPattern_ = prog_.uniformLocation("uPattern");
    locMode_ = prog_.uniformLocation("uMode");

    emptyVao_.create();

    box.create(this, MeshGen::box());
    sphere.create(this, MeshGen::superEllipsoid(20, 32, 2.0f));
    roundBox.create(this, MeshGen::superEllipsoid(20, 32, 3.4f));
    cylinder.create(this, MeshGen::cylinder(28));
    cone.create(this, MeshGen::cone(20));
    torus.create(this, MeshGen::torus(0.5f, 0.05f, 56, 14));
    disc.create(this, MeshGen::disc(36));
}

void Renderer::destroy()
{
    for (Mesh* m : {&box, &sphere, &roundBox, &cylinder, &cone, &torus, &disc}) m->destroy();
    prog_.removeAllShaders();
    sky_.removeAllShaders();
    if (emptyVao_.isCreated()) emptyVao_.destroy();
}

void Renderer::beginFrame(const QMatrix4x4& view, const QMatrix4x4& proj, const QVector3D& camPos, float time)
{
    view_ = view;
    proj_ = proj;
    vp_ = proj * view;
    camPos_ = camPos;
    time_ = time;
}

void Renderer::drawSky()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    sky_.bind();
    sky_.setUniformValue("uInvVP", vp_.inverted());
    sky_.setUniformValue("uSunDir", sunDir());
    {
        QOpenGLVertexArrayObject::Binder b(&emptyVao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    sky_.release();
}

void Renderer::begin3D()
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    prog_.bind();
    prog_.setUniformValue("uVP", vp_);
    prog_.setUniformValue("uCamPos", camPos_);
    prog_.setUniformValue("uSunDir", sunDir());
    prog_.setUniformValue("uFogColor", fogColor());
}

void Renderer::setCull(bool on)
{
    if (on) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
}

void Renderer::draw(const Mesh& m, const QMatrix4x4& model, const Material& mat)
{
    prog_.setUniformValue(locModel_, model);
    prog_.setUniformValue(locNormal_, model.normalMatrix());
    prog_.setUniformValue(locColor_, mat.color);
    prog_.setUniformValue(locEmissive_, mat.emissive);
    prog_.setUniformValue(locShine_, mat.shine);
    prog_.setUniformValue(locSpec_, mat.spec);
    prog_.setUniformValue(locAlpha_, mat.alpha);
    prog_.setUniformValue(locRim_, mat.rim);
    prog_.setUniformValue(locPattern_, mat.pattern);
    prog_.setUniformValue(locMode_, mat.mode);
    m.draw(this);
}

void Renderer::endFrame()
{
    prog_.release();
    glDepthMask(GL_TRUE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}
