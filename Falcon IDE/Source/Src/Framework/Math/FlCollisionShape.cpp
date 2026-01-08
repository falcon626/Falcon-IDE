#include "FlCollisionShape.h"

bool FlCollisionShape::Intersects(const FlCollisionShape& other, float* outDistance, float maxRayDistance) const
{
    using namespace DirectX;

    // None は常に非ヒット
    if (type == Type::None || other.type == Type::None) return false;

    switch (type)
    {
    case Type::Ray:
    {
        // Ray は Box と Sphere のみに対応（Ray vs Ray は未対応）
        if (other.type == Type::Box) {
            auto d{ Def::FloatZero };
            bool hit = other.box.box.Intersects(ray.origin, ray.direction, outDistance ? *outDistance : d);
            if (!hit) return false;
            // 最大距離制限
            float useD = outDistance ? *outDistance : d;
            return useD <= maxRayDistance;
        }
        if (other.type == Type::Sphere) {
            auto d{ Def::FloatZero };
            bool hit = other.sphere.sphere.Intersects(ray.origin, ray.direction, outDistance ? *outDistance : d);
            if (!hit) return false;
            float useD = outDistance ? *outDistance : d;
            return useD <= maxRayDistance;
        }
        return false;
    }

    case Type::Box:
    {
        if (other.type == Type::Box)   return box.box.Intersects(other.box.box);
        if (other.type == Type::Sphere) return box.box.Intersects(other.sphere.sphere);
        // Box vs Ray は Ray 側で処理するのが自然。ここでは非対応にしておく。
        return false;
    }

    case Type::Sphere:
    {
        if (other.type == Type::Sphere) return sphere.sphere.Intersects(other.sphere.sphere);
        if (other.type == Type::Box)    return sphere.sphere.Intersects(other.box.box);
        return false;
    }

    default:
        return false;
    }
}
