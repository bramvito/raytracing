#ifndef AABB_H
#define AABB_H

#include "rtweekend.h"

class aabb {
    public:
        aabb() : _min(infinity, infinity, infinity), _max(-infinity, -infinity , -infinity) {}
        aabb(const point3& a, const point3& b) { _min = a; _max = b; }

        point3 min() const { return _min; }
        point3 max() const { return _max; }

        unsigned max_axis_idx() const {
            vec3 size = _max - _min;

            if (size.x() >= size.y() && size.x() >= size.z())
                return 0;
            if (size.y() >= size.x() && size.y() >= size.z())
                return 1;

            return 2;
        }

        float intersect(const ray& r, double tmin, double tmax) const {
            for (int a = 0; a < 3; a++) {
                auto invD = 1.0f / r.direction()[a];
                auto t0 = (min()[a] - r.origin()[a]) * invD;
                auto t1 = (max()[a] - r.origin()[a]) * invD;
                if (invD < 0.0f)
                    std::swap(t0, t1);
                tmin = t0 > tmin ? t0 : tmin;
                tmax = t1 < tmax ? t1 : tmax;
                if (tmax <= tmin)
                    return -1.0f;
            }
            return tmin;
        }

        float half_surface_area() const {
            vec3 offset = _max - _min;
            return offset.x() * offset.y() + offset.y() * offset.z() + offset.z() * offset.x();
        }

        point3 _min;
        point3 _max;
};

// aabb surrounding_box(aabb box0, aabb box1) {
//     point3 small(fmin(box0.min().x(), box1.min().x()),
//                  fmin(box0.min().y(), box1.min().y()),
//                  fmin(box0.min().z(), box1.min().z()));

//     point3 big(fmax(box0.max().x(), box1.max().x()),
//                fmax(box0.max().y(), box1.max().y()),
//                fmax(box0.max().z(), box1.max().z()));

//     return aabb(small, big);
// }

aabb surrounding_box(const aabb box, const point3 p) {
    aabb new_box = box;
    for (unsigned i = 0; i < 3; i++) {
        if (p[i] < box._min[i])
            new_box._min[i] = p[i];
        if (p[i] > box._max[i])
            new_box._max[i] = p[i];
    }
    return new_box;
}

aabb surrounding_box(const aabb& box0, const aabb& box1) {
    aabb result;
    for (int i = 0; i < 3; i++) {
        result._min[i] = std::min(box0._min[i], box1._min[i]);
        result._max[i] = std::max(box0._max[i], box1._max[i]);
    }
    return result;
}

#endif