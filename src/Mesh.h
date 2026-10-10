#pragma once
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <cstdint>
#include <vector>

// Köşe düzeni: x y z nx ny nz
struct MeshData {
    std::vector<float> v;
    std::vector<uint32_t> idx;
    void addVertex(float x, float y, float z, float nx, float ny, float nz)
    {
        v.insert(v.end(), {x, y, z, nx, ny, nz});
    }
    void addTri(uint32_t a, uint32_t b, uint32_t c) { idx.insert(idx.end(), {a, b, c}); }
};

// Birim boyutlu, orijin merkezli yordamsal şekiller (model matrisiyle ölçeklenir).
namespace MeshGen {
MeshData box();                                   // [-0.5, 0.5]^3
MeshData superEllipsoid(int stacks, int slices, float n); // n=2 küre, n>2 yuvarlatılmış kutu; çap 1
MeshData cylinder(int slices);                    // Y ekseni, yarıçap 0.5, boy 1
MeshData cone(int slices);                        // Y ekseni, taban yarıçapı 0.5, boy 1, uç +Y
MeshData torus(float R, float r, int seg, int sides); // XY düzleminde halka (Z ekseni)
MeshData disc(int slices);                        // XZ düzleminde, yarıçap 0.5, normal +Y
} // namespace MeshGen

class Mesh {
public:
    void create(QOpenGLFunctions* f, const MeshData& d);
    void draw(QOpenGLFunctions* f) const;
    void destroy();

private:
    mutable QOpenGLVertexArrayObject vao_;
    QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer ibo_{QOpenGLBuffer::IndexBuffer};
    int count_ = 0;
};
