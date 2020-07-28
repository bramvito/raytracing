#ifndef CAMERA_H
#define CAMERA_H

#include "rtweekend.h"

class camera {
    public:
    	camera() { }

    	camera(point3 lookfrom, point3 lookat, vec3 vup, double vfov, double aspect_ratio) {
			_lookfrom = lookfrom;
			_lookat = lookat;
			_vup = vup;
			_vfov = vfov;
			_aspect_ratio = aspect_ratio;

			setup_camera();
		}

		void setup_camera() {
			auto theta = degrees_to_radians(_vfov);
			auto h = tan(theta/2);
			auto viewport_height = 2.0 * h;
			auto viewport_width = viewport_height * _aspect_ratio;
			auto focal_length = 1.0;

			auto w = unit_vector(_lookfrom - _lookat);
			auto u = unit_vector(cross(_vup, w));
			auto v = cross(w, u);

			origin = _lookfrom;
			horizontal = viewport_width * u;
			vertical = viewport_height * v;
			lower_left_corner = origin - vertical/2 - horizontal/2 - w*focal_length;
		}

		void lookfrom(point3 lookfrom) {
			_lookfrom = lookfrom;
			setup_camera();
		}

		void lookat(point3 lookat) {
			_lookat = lookat;
			setup_camera();
		}

		void vfov(double vfov) {
			_vfov = vfov;
			setup_camera();
		}

		void aspect_ratio(double aspect_ratio) {
			_aspect_ratio = aspect_ratio;
			setup_camera();
		}

		ray get_ray(double u, double v) const {
			return ray(origin, lower_left_corner + u*horizontal + v*vertical - origin);
		}

		point3 _lookfrom;
		point3 _lookat;
		vec3 _vup;
		double _vfov;
		double _aspect_ratio;

	private:
		point3 origin;
		point3 lower_left_corner;
		vec3 horizontal;
		vec3 vertical;
};

#endif