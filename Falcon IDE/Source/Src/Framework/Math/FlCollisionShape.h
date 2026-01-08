#pragma once

struct FlCollisionShape
{
    enum class Type { None, Ray, Box, Sphere };

    struct RayData { DirectX::XMVECTOR origin; DirectX::XMVECTOR direction; };
    struct BoxData { DirectX::BoundingBox box; };
    struct SphereData { DirectX::BoundingSphere sphere; };

    Type type{ Type::None };
    RayData    ray{};
    BoxData    box{};
    SphereData sphere{};

    static FlCollisionShape CreateRay(const DirectX::XMFLOAT3& originF3, const DirectX::XMFLOAT3& dirF3)
    {
        using namespace DirectX;
        FlCollisionShape s;
        s.type = Type::Ray;

        auto o = XMVectorSet(originF3.x, originF3.y, originF3.z, 0.0f);
        auto d = XMVectorSet(dirF3.x, dirF3.y, dirF3.z, 0.0f);

        // ê≥ãKâªÇ∆É[Éçï˚å¸ëŒçÙ
        float lenSq;
        XMStoreFloat(&lenSq, XMVector3LengthSq(d));
        if (lenSq < 1e-12f) {
            s.type = Type::None;
            return s;
        }
        d = XMVector3Normalize(d);

        s.ray.origin = o;
        s.ray.direction = d;
        return s;
    }

    static FlCollisionShape CreateBox(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& size)
    {
        FlCollisionShape s;
        s.type = Type::Box;

        DirectX::XMFLOAT3 extents{ size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
        s.box.box = DirectX::BoundingBox(center, extents);
        return s;
    }

    static FlCollisionShape CreateSphere(const DirectX::XMFLOAT3& center, float radius)
    {
        FlCollisionShape s;
        s.type = Type::Sphere;
        if (radius <= 0.0f) {
            s.type = Type::None;
            return s;
        }
        s.sphere.sphere = DirectX::BoundingSphere(center, radius);
        return s;
    }

    bool Intersects(const FlCollisionShape& other, float* outDistance = nullptr, float maxRayDistance = std::numeric_limits<float>::infinity()) const;
};

