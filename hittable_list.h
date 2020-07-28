#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"

#include <memory>
#include <vector>

using std::shared_ptr;
using std::make_shared;

class hittable_list: public hittable {
    public:
        hittable_list() {}
        hittable_list(shared_ptr<hittable> object) { add(object); }

        void clear() {
            objects.clear();
        }

        void add(shared_ptr<hittable> object) {
            objects.push_back(object);
        }

        void add(hittable_list objects_to_add) {
            for (const auto& object : objects_to_add.objects) {
                add(object);
            }
        }

        //virtual bool hit(const ray& r, double tmin, double tmax, hit_record& rec) const;

        bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
            hit_record temp_rec;
            bool hit_anything = false;
            auto closest_so_far = t_max;

            for (const auto& object : objects) {
                if (object->hit(r, t_min, closest_so_far, temp_rec)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
            }

            return hit_anything;
        }

        aabb bounding_box() const {
            aabb output_box;
            aabb temp_box;
            bool first_box = true;

            for (const auto& object : objects) {
                temp_box = object->bounding_box();
                output_box = first_box ? temp_box : surrounding_box(output_box, temp_box);
                first_box = false;
            }

            return output_box;
        }

    public:
        std::vector<shared_ptr<hittable>> objects;
};

#endif