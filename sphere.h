#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"
#include "material.h"

class sphere: public hittable {
    public:
        sphere() {}
        sphere(point3 cen, double r, shared_ptr<material> m) : center(cen), radius(r), mat_ptr(m) {
        	centroid = center;
        };

        virtual bool hit(const ray& r, double tmin, double tmax, hit_record& rec) const;
        virtual aabb bounding_box() const;
        bool hit(const ray& r, double t_min, double t_max, hit_record& rec);

    public:
        point3 center;
        double radius;
        shared_ptr<material> mat_ptr;
};

bool sphere::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
    auto oc = r.origin() - center;
	auto a = r.direction().squared_length();
	auto half_b = dot(r.direction(), oc);
	auto c = oc.squared_length() - radius*radius;
	auto discriminant = half_b*half_b - a*c;

	if (discriminant > 0) {
		auto root = sqrt(discriminant);
		auto temp = (-half_b - root) / a;
		if (temp < t_max && temp > t_min) {
			rec.t = temp;
			rec.p = r.at(rec.t);
			vec3 outward_normal = (rec.p - center) / radius;
			rec.set_face_normal(r, outward_normal);
			rec.mat_ptr = mat_ptr;
			return true;
		}
		temp = (-half_b + root) / a;
		if (temp < t_max && temp > t_min) {
			rec.t = temp;
			rec.p = r.at(rec.t);
			vec3 outward_normal = (rec.p - center) / radius;
			rec.set_face_normal(r, outward_normal);
			rec.mat_ptr = mat_ptr;
			return true;
		}
	}
	return false;
}

aabb sphere::bounding_box() const {
	return aabb(
		center - vec3(radius, radius, radius), 
		center + vec3(radius, radius, radius));
}

#endif