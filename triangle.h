#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "rtweekend.h"
#include "hittable.h"
#include "vec3.h"
#include "material.h"


class triangle: public hittable {
    public:
        triangle() {}
        triangle(point3 v0, point3 v1, point3 v2, shared_ptr<material> m) : v0(v0), v1(v1), v2(v2), mat_ptr(m) {
        	centroid = (v0 + v1 + v2) / 3;
        };

        bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
			D(num_ray_triangle_tests++);
			// Moller Trumbore algorithm 
			vec3 v0v1 = v1 - v0; 
			vec3 v0v2 = v2 - v0; 
			vec3 pvec = cross(r.direction(), v0v2); 
			double det = dot(v0v1, pvec);

			// ray and triangle are parallel if det is close to 0
			if (fabs(det) < kEpsilon)
				return false;

			double invDet = 1 / det; 

			vec3 tvec = r.origin() - v0; 
			auto u = dot(tvec, pvec) * invDet; 
			if (u < 0 || u > 1)
				return false;

			vec3 qvec = cross(tvec, v0v1); 
			auto v = dot(r.direction(), qvec) * invDet; 
			if (v < 0 || u + v > 1)
				return false;

			auto temp = dot(v0v2, qvec) * invDet;
			if (temp < t_min || temp > t_max)
				return false;

			// set hit record details
			rec.t = temp;
			rec.p = r.at(rec.t);
			vec3 outward_normal = unit_vector(cross(v0v1, v0v2));
			rec.set_face_normal(r, outward_normal);
			rec.mat_ptr = mat_ptr;

			D(num_ray_triangle_intersections++);
			return true;
		}

        aabb bounding_box() const {
			point3 small(
				fmin(v0.x(), fmin(v1.x(), v2.x())),
				fmin(v0.y(), fmin(v1.y(), v2.y())),
				fmin(v0.z(), fmin(v1.z(), v2.z())));

			point3 big(
				fmax(v0.x(), fmax(v1.x(), v2.x())),
				fmax(v0.y(), fmax(v1.y(), v2.y())),
				fmax(v0.z(), fmax(v1.z(), v2.z())));

			// if the bounding box is to small on one of the dimensions
			// make it a little bigger on that dimension
			auto delta = big - small;
			for (int i = 0; i < 3; ++i) {
				if (delta.e[i] < kEpsilon)
					big.e[i] = small.e[i] + kEpsilon;
			}

			return aabb(small, big);
		}

    public:
        point3 v0;
        point3 v1;
        point3 v2;
        shared_ptr<material> mat_ptr;
};

#endif