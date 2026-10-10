#pragma once
#include <QVector3D>
#include <cmath>
#include <vector>

struct AABB {
    QVector3D min, max;
};

// Kutu şeklinde platform. Hareketli ise origin etrafında axis boyunca salınır.
struct Platform {
    QVector3D center;        // anlık merkez
    QVector3D size;          // tam boyutlar
    QVector3D color;
    QVector3D origin;        // hareketsiz merkez
    QVector3D axis{0, 0, 0}; // hareket ekseni (birim)
    float amp = 0.f, speed = 0.f, phase = 0.f;
    QVector3D delta{0, 0, 0}; // son güncellemedeki yer değiştirme

    bool moving() const { return amp > 0.f; }
    AABB box() const {
        const QVector3D h = size * 0.5f;
        return {center - h, center + h};
    }
    void update(float t) {
        const QVector3D old = center;
        if (amp > 0.f)
            center = origin + axis * (amp * std::sin(t * speed + phase));
        delta = center - old;
    }
};

struct Coin {
    QVector3D pos;
    int value = 1; // 1 = küçük hücre, 5 = büyük hücre
    bool taken = false;
    float phase = 0.f;
};

struct Enemy {
    QVector3D a, b;   // devriye uçları
    QVector3D pos;
    float speed = 2.f;
    int dir = 1;
    bool alive = true;
    float deadTimer = 0.f;
    float yaw = 0.f;
    float phase = 0.f;
};

struct SpikeTrap {
    QVector3D pos;
    float phase = 0.f;
    float ext = 0.f;  // 0..1 çıkıklık
    bool warn = false;
};

struct Checkpoint {
    QVector3D pos;
    bool active = false;
};

struct Goal {
    QVector3D pos;
    float radius = 1.5f;
};

struct Level {
    std::vector<Platform> platforms;
    std::vector<Platform> decor; // çarpışmasız süs adaları
    std::vector<Coin> coins;
    std::vector<Enemy> enemies;
    std::vector<SpikeTrap> spikes;
    std::vector<Checkpoint> checkpoints;
    Goal goal;
    QVector3D start{0, 0, 3};
    float killY = -24.f;
    int requiredCells = 20;
    int totalCells = 0;

    void update(float t);
    // (x,z) noktasının altındaki (maxY'yi aşmayan) en yüksek yüzeyin y değeri; yoksa -1000
    float groundBelow(float x, float z, float maxY) const;
};

Level buildLevel();
