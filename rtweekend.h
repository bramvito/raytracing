#ifndef RTWEEKEND_H
#define RTWEEKEND_H

#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <atomic>

//#define BVH_HEATMAP
//#define DEBUG
#ifdef DEBUG 
#define D(x) (x)
#else 
#define D(x) do{}while(0)
#endif


// Usings

using std::shared_ptr;
using std::make_shared;
using std::unique_ptr;
using std::make_unique;
using std::sqrt;

// Constants

const float infinity = std::numeric_limits<float>::infinity();
const double pi = 3.1415926535897932385;
constexpr double kEpsilon = 1e-5; 

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180;
}

inline double random_int(int min, int max) {
	// Returns a random int in [min,max].
	return rand()%(max-min + 1) + min;
}

inline double random_double() {
	// Returns a random real in [0,1).
	return rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
    // Returns a random real in [min,max).
    return min + (max-min)*random_double();
}

inline double clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

// Common Headers

#include "ray.h"
#include "vec3.h"

// used for statistics
std::atomic<uint32_t> num_ray_triangle_tests(0); 
std::atomic<uint32_t> num_ray_triangle_intersections(0); 
std::atomic<uint32_t> num_primary_rays(0);
std::atomic<uint32_t> num_triangles(0);
std::atomic<uint32_t> num_ray_bvh_aabb_tests(0);
std::atomic<uint32_t> num_ray_bvh_aabb_intersections(0);
std::atomic<uint32_t> num_bvh_nodes(0); 
std::atomic<uint32_t> num_bvh_leaf_nodes(0); 
std::atomic<uint32_t> num_ray_bvh_leaf_tests(0);
std::atomic<uint32_t> num_ray_bvh_leaf_intersections(0);

#endif