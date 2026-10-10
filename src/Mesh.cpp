#include "Mesh.h"
#include <QVector3D>
#include <cmath>

static const float kPi = 3.14159265358979f;

// ---------------------------------------------------------------------------
namespace MeshGen {

MeshData box()
{
    MeshData m;
    // normal n, teğet u, ikinci teğet v  (u x v = n  ->  dışarıdan bakınca CCW)
    struct Face { float n[3], u[3], v[3]; };
    const Face faces[6] = {
        {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},  {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
        {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},  {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
        {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},   {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
    };
    const float su[4] = {-1, 1, 1, -1}, sv[4] = {-1, -1, 1, 1};
    for (const auto& f : faces) {
        const uint32_t base = uint32_t(m.v.size() / 6);
        for (int k = 0; k < 4; ++k) {
            m.addVertex(0.5f * (f.n[0] + su[k] * f.u[0] + sv[k] * f.v[0]),
                        0.5f * (f.n[1] + su[k] * f.u[1] + sv[k] * f.v[1]),
                        0.5f * (f.n[2] + su[k] * f.u[2] + sv[k] * f.v[2]), f.n[0], f.n[1], f.n[2]);
        }
        m.addTri(base, base + 1, base + 2);
        m.addTri(base, base + 2, base + 3);
    }
    return m;
}

MeshData superEllipsoid(int stacks, int slices, float n)
{
    MeshData m;
    auto g = [&](float c) { return std::copysign(std::pow(std::fabs(c), n - 1.0f), c); };
    for (int i = 0; i <= stacks; ++i) {
        const float th = kPi * float(i) / float(stacks);
        const float st = std::sin(th), ct = std::cos(th);
        for (int j = 0; j <= slices; ++j) {
            const float ph = 2.f * kPi * float(j) / float(slices);
            const float dx = st * std::cos(ph), dy = ct, dz = st * std::sin(ph);
            const float s = std::pow(std::pow(std::fabs(dx), n) + std::pow(std::fabs(dy), n) + std::pow(std::fabs(dz), n), 1.0f / n);
            const float x = dx / s, y = dy / s, z = dz / s;
            QVector3D nrm(g(x), g(y), g(z));
            nrm.normalize();
            m.addVertex(x * 0.5f, y * 0.5f, z * 0.5f, nrm.x(), nrm.y(), nrm.z());
        }
    }
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j) {
            const uint32_t a = uint32_t(i * (slices + 1) + j), b = a + uint32_t(slices + 1);
            m.addTri(a, a + 1, b);
            m.addTri(a + 1, b + 1, b);
        }
    return m;
}

MeshData cylinder(int slices)
{
    MeshData m;
    // yan yüzey
    for (int j = 0; j <= slices; ++j) {
        const float a = 2.f * kPi * float(j) / float(slices);
        const float c = std::cos(a), s = std::sin(a);
        m.addVertex(0.5f * c, -0.5f, 0.5f * s, c, 0, s);
        m.addVertex(0.5f * c, 0.5f, 0.5f * s, c, 0, s);
    }
    for (int j = 0; j < slices; ++j) {
        const uint32_t b0 = uint32_t(j * 2), t0 = b0 + 1, b1 = b0 + 2, t1 = b0 + 3;
        m.addTri(b0, t0, b1);
        m.addTri(t0, t1, b1);
    }
    // kapaklar
    for (int cap = 0; cap < 2; ++cap) {
        const float y = cap ? 0.5f : -0.5f, ny = cap ? 1.f : -1.f;
        const uint32_t c = uint32_t(m.v.size() / 6);
        m.addVertex(0, y, 0, 0, ny, 0);
        for (int j = 0; j <= slices; ++j) {
            const float a = 2.f * kPi * float(j) / float(slices);
            m.addVertex(0.5f * std::cos(a), y, 0.5f * std::sin(a), 0, ny, 0);
        }
        for (int j = 0; j < slices; ++j) {
            const uint32_t r0 = c + 1 + uint32_t(j), r1 = r0 + 1;
            if (cap) m.addTri(c, r1, r0);
            else m.addTri(c, r0, r1);
        }
    }
    return m;
}

MeshData cone(int slices)
{
    MeshData m;
    for (int j = 0; j < slices; ++j) {
        const float a0 = 2.f * kPi * float(j) / float(slices);
        const float a1 = 2.f * kPi * float(j + 1) / float(slices);
        const float am = 0.5f * (a0 + a1);
        auto nrm = [](float a) { QVector3D n(std::cos(a), 0.5f, std::sin(a)); n.normalize(); return n; };
        const uint32_t b = uint32_t(m.v.size() / 6);
        QVector3D n0 = nrm(a0), nm = nrm(am), n1 = nrm(a1);
        m.addVertex(0.5f * std::cos(a0), -0.5f, 0.5f * std::sin(a0), n0.x(), n0.y(), n0.z());
        m.addVertex(0, 0.5f, 0, nm.x(), nm.y(), nm.z());
        m.addVertex(0.5f * std::cos(a1), -0.5f, 0.5f * std::sin(a1), n1.x(), n1.y(), n1.z());
        m.addTri(b, b + 1, b + 2);
    }
    const uint32_t c = uint32_t(m.v.size() / 6);
    m.addVertex(0, -0.5f, 0, 0, -1, 0);
    for (int j = 0; j <= slices; ++j) {
        const float a = 2.f * kPi * float(j) / float(slices);
        m.addVertex(0.5f * std::cos(a), -0.5f, 0.5f * std::sin(a), 0, -1, 0);
    }
    for (int j = 0; j < slices; ++j) m.addTri(c, c + 1 + uint32_t(j), c + 2 + uint32_t(j));
    return m;
}

MeshData torus(float R, float r, int seg, int sides)
{
    MeshData m;
    for (int i = 0; i <= seg; ++i) {
        const float u = 2.f * kPi * float(i) / float(seg);
        const float cu = std::cos(u), su = std::sin(u);
        for (int j = 0; j <= sides; ++j) {
            const float v = 2.f * kPi * float(j) / float(sides);
            const float cv = std::cos(v), sv = std::sin(v);
            m.addVertex((R + r * cv) * cu, (R + r * cv) * su, r * sv, cv * cu, cv * su, sv);
        }
    }
    for (int i = 0; i < seg; ++i)
        for (int j = 0; j < sides; ++j) {
            const uint32_t a = uint32_t(i * (sides + 1) + j), b = a + uint32_t(sides + 1);
            m.addTri(a, b, a + 1);
            m.addTri(a + 1, b, b + 1);
        }
    return m;
}

MeshData disc(int slices)
{
    MeshData m;
    m.addVertex(0, 0, 0, 0, 1, 0);
    for (int j = 0; j <= slices; ++j) {
        const float a = 2.f * kPi * float(j) / float(slices);
        m.addVertex(0.5f * std::cos(a), 0, 0.5f * std::sin(a), 0, 1, 0);
    }
    for (int j = 0; j < slices; ++j) m.addTri(0, uint32_t(j + 2), uint32_t(j + 1));
    return m;
}

} // namespace MeshGen

// ---------------------------------------------------------------------------
void Mesh::create(QOpenGLFunctions* f, const MeshData& d)
{
    vao_.create();
    QOpenGLVertexArrayObject::Binder binder(&vao_);
    vbo_.create();
    vbo_.bind();
    vbo_.allocate(d.v.data(), int(d.v.size() * sizeof(float)));
    ibo_.create();
    ibo_.bind(); // VAO'nun parçası olarak kaydedilir; VAO bağlıyken çözülmemeli
    ibo_.allocate(d.idx.data(), int(d.idx.size() * sizeof(uint32_t)));
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    count_ = int(d.idx.size());
}

void Mesh::draw(QOpenGLFunctions* f) const
{
    QOpenGLVertexArrayObject::Binder binder(&vao_);
    f->glDrawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
}

void Mesh::destroy()
{
    if (vbo_.isCreated()) vbo_.destroy();
    if (ibo_.isCreated()) ibo_.destroy();
    if (vao_.isCreated()) vao_.destroy();
}
