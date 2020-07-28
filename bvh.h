#ifndef BVH_H
#define BVH_H

//#define BVH_ITERATIVE
#define BVH_RECURSIVE_FAST
//#define BVH_RECURSIVE_SLOW
#define BVH_SPLIT_SAH
//#define BVH_SPLIT_MEDIAN

#include <algorithm>
#include <cstring>

#include "hittable.h"

inline bool box_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis) {
	aabb box_a = a->bounding_box();
	aabb box_b = b->bounding_box();

    return box_a.min().e[axis] < box_b.min().e[axis];
}

bool box_x_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
	return box_compare(a, b, 0);
}

bool box_y_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
	return box_compare(a, b, 1);
}

bool box_z_compare(const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
	return box_compare(a, b, 2);
}

class bvh_node : public hittable {
	public:
		bvh_node();

		bvh_node(hittable_list& list) : bvh_node(list.objects, 0, list.objects.size()) {}

		bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end);

		virtual bool hit(const ray& r, double tmin, double tmax, hit_record& rec) const;
        aabb bounding_box() const { return box; }

    public:
        shared_ptr<hittable> left;
        shared_ptr<hittable> right;
        aabb box;
};

// return the surface area heuristic of the specific split plane.
// left:     The number of primitives in the left node to be split.
// right:    The number of primitives in the right node to be split.
// lbox:     Bounding box of the left node to be split.
// rbox:     Bounding box of the right node to be split.
// box:      Bounding box of the current node.
float sah(unsigned left, unsigned right, const aabb& lbox, const aabb& rbox , const aabb& box ) {
    return (left * lbox.half_surface_area() + right * rbox.half_surface_area()) / box.half_surface_area();
}

// pick the best split based on the surface area heuristic
// heavily based on bvh implementation in SORT by Jiayin Cao
// https://github.com/JiayinCao/SORT/blob/master/src/accel/bvh_utils.h#L68
float pick_best_split(
	int& axis,
	float& split_pos,
	std::vector<shared_ptr<hittable>>& primitives,
	const aabb& node_aabb,
	const size_t start,
	const size_t end)
{
	static constexpr unsigned BVH_SPLIT_COUNT = 16;

	// centroid bounds
	aabb inner;
	for (auto i = start; i < end; i++) {
		inner = surrounding_box(inner, primitives[i]->centroid);
	}

	auto primitive_num = end - start;
	axis = inner.max_axis_idx();
	auto min_sah = infinity;

	// return the middle as split position (for debugging)
	// split_pos = (inner._min[axis] + inner._max[axis]) / 2;
	// return min_sah;

	// distribute the primitives into bins
	unsigned    bin[BVH_SPLIT_COUNT];
	aabb        bbox[BVH_SPLIT_COUNT];
	aabb        rbox[BVH_SPLIT_COUNT-1];
	memset(bin, 0, sizeof(unsigned) * BVH_SPLIT_COUNT);
	auto split_start = inner._min[axis];
	auto split_delta = (inner._max[axis] - inner._min[axis]) / BVH_SPLIT_COUNT;
	if (split_delta == 0.0f) {
		split_pos = (inner._min[axis] + inner._max[axis]) / 2;
		std::cout << "pick_best_split: fix this! -----------------------------\n";
		return min_sah;
	}
	auto inv_split_delta = 1.0f / split_delta;
	for (auto i = start; i < end ; i++) {
		auto index = (int)((primitives[i]->centroid[axis] - split_start) * inv_split_delta);
	    index = std::min(index, (int)(BVH_SPLIT_COUNT - 1));
	    ++bin[index];
	    bbox[index] = surrounding_box(bbox[index], primitives[i]->bounding_box());
	}

	// determine the best split pos of the 16 splits
	// fill the right boxes
	rbox[BVH_SPLIT_COUNT-2] = surrounding_box(rbox[BVH_SPLIT_COUNT-2], bbox[BVH_SPLIT_COUNT-1]);
	for (int i = BVH_SPLIT_COUNT-3; i >= 0; i--)
	    rbox[i] = surrounding_box(rbox[i+1], bbox[i+1]);

	auto    left = bin[0];
	auto    lbox = bbox[0];
	auto    pos = split_delta + split_start;
	// check the sah value for all 16 split pane positions from begin to end
	for (unsigned i = 0; i < BVH_SPLIT_COUNT - 1; i++) {
	    auto sah_value = sah(left, primitive_num - left, lbox, rbox[i], node_aabb);
	    if (sah_value < min_sah) {
	        min_sah = sah_value;
	        split_pos = pos;
	    }
	    left += bin[i+1];
	    lbox = surrounding_box(lbox, bbox[i+1]);
	    pos += split_delta;
	}

	return min_sah;
}


#ifdef BVH_SPLIT_SAH
bvh_node::bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
	D(num_bvh_nodes++);

	// generate the bounding box for the node
	box = objects[start]->bounding_box();
    for (auto i = start; i < end; i++) {
        box = surrounding_box(box, objects[i]->bounding_box());
    }

	size_t object_span = end - start;

	// make a leaf node
	if (object_span == 1) {
		D(num_bvh_leaf_nodes++);
		left = objects[start];
		right = nullptr;

		box = left->bounding_box();
		return;
	}

	// pick best split plane
    int split_axis;
    float split_pos;
    const auto sah = pick_best_split(split_axis, split_pos, objects, box, start, end);

    if (sah >= object_span){
        std::cout << "sah: fix this! -----------------------------\n";
    }

    // partition the data
    auto compare = [split_pos, split_axis](const shared_ptr<hittable> pri) {
    	return pri->centroid.e[split_axis] < split_pos;
    };
    auto middle = std::partition(objects.begin() + start, objects.begin() + end, compare);
    auto mid = (unsigned)(middle - objects.begin());

    // To avoid degenerated node that has nothing in it.
    // it is possible to pick one with no primitive on one side of the plane,
    // resulting a crash later during ray tracing.
    if (mid == start || mid == end) {
		std::cout << "bvh error: fix this! -----------------------------\n";
		// std::cout << "mid == start!\n";
		// std::cout << "split_axis " << split_axis << "\n";
		// std::cout << box.min() << " -------- " << box.max() << "\n";
		// std::cout << " num children " << object_span << "\n";
		// for (auto i = start; i < end; i++) {
		// 	std::cout << "centroid: " << objects[i]->centroid.e[split_axis] << "\n";
		// 	box = surrounding_box(box, objects[i]->bounding_box());
		// }
		// std::cout << "end -----------------------------------------------\n";
		// auto comparator = (split_axis == 0) ? box_x_compare
		// 				: (split_axis == 1) ? box_y_compare
		// 									: box_z_compare;

		// std::sort(objects.begin() + start, objects.begin() + end, comparator);
		// mid = start + object_span / 2;
    }

	left = make_shared<bvh_node>(objects, start, mid);
	right = make_shared<bvh_node>(objects, mid, end);

	aabb box_left = left->bounding_box();
	aabb box_right = right->bounding_box();
	box = surrounding_box(box_left, box_right);
}
#elif defined BVH_SPLIT_MEDIAN
bvh_node::bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
	D(num_bvh_nodes++);

	size_t object_span = end - start;

	// make a leaf node
	if (object_span == 1) {
		D(num_bvh_leaf_nodes++);
		left = objects[start];
		right = nullptr;

		box = left->bounding_box();
		return;
	}

	int axis = random_int(0, 2);
	auto comparator = (axis == 0) ? box_x_compare
					: (axis == 1) ? box_y_compare
								  : box_z_compare;

	std::sort(objects.begin() + start, objects.begin() + end, comparator);
	auto mid = start + object_span / 2;
	left = make_shared<bvh_node>(objects, start, mid);
	right = make_shared<bvh_node>(objects, mid, end);

	aabb box_left = left->bounding_box();
	aabb box_right = right->bounding_box();
	box = surrounding_box(box_left, box_right);
}
#endif


#ifdef BVH_RECURSIVE_SLOW
bool bvh_node::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {

	#ifdef DEBUG
	num_ray_bvh_aabb_tests++;
	if (right == nullptr) {
		num_ray_bvh_leaf_tests++;
	}
	#endif

	float fmin = box.intersect(r, t_min, t_max);
	if (fmin < 0.0f)
		return false;

	#ifdef DEBUG
	rec.num_bvh_node_intersects++;
	num_ray_bvh_aabb_intersections++;
	if (right == nullptr) {
		num_ray_bvh_leaf_intersections++;
	}
	#endif

	// if their is no right child, this is a leaf node
	if (right == nullptr)
		return left->hit(r, t_min, t_max, rec);

	bool hit_left = left->hit(r, t_min, t_max, rec);
	bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);

	return hit_left || hit_right;
}
#elif defined BVH_RECURSIVE_FAST
bool traverse_node(const bvh_node* node, const ray& r, double t_min, double t_max, hit_record& rec) {

	#ifdef DEBUG
	num_ray_bvh_aabb_tests++;
	if (node->right == nullptr)
		num_ray_bvh_leaf_tests++;
	#endif

	// float fmin = box.intersect(r, t_min, t_max);
	// if (fmin < 0.0f) {
	// 	return false;
	// }

	#ifdef DEBUG
	rec.num_bvh_node_intersects++;
	num_ray_bvh_aabb_intersections++;
	if (node->right == nullptr)
		num_ray_bvh_leaf_intersections++;
	#endif

	// if their is no right child, this is a leaf node
	if (node->right == nullptr)
		return node->left->hit(r, t_min, t_max, rec);

	// bool hit_left = left->hit(r, t_min, t_max, rec);
	// bool hit_right = right->hit(r, t_min, hit_left ? rec.t : t_max, rec);

	// return hit_left || hit_right;

	const auto node_left = (bvh_node*)node->left.get();
	const auto node_right = (bvh_node*)node->right.get();

	const auto fmin0 = node_left->box.intersect(r, t_min, t_max);
    const auto fmin1 = node_right->box.intersect(r, t_min, t_max);

    auto inter = false;
	if (fmin1 > fmin0) {
		if (fmin0 >= 0.0f)
			inter |= traverse_node(node_left, r, t_min, t_max, rec);
		// if (inter && rec.t < fmin0)
		// 	return true;
		if (fmin1 >= 0.0f)
			inter |= traverse_node(node_right, r, t_min, inter ? rec.t : t_max, rec);
	} else {
		if (fmin1 >= 0.0f)
			inter |= traverse_node(node_right, r, t_min, t_max, rec);
		// if (inter && rec.t < fmin1)
		// 	return true;
		if (fmin0 >= 0.0f)
			inter |= traverse_node(node_left, r, t_min, inter ? rec.t : t_max, rec);
	}

	return inter;
}
bool bvh_node::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {

	const auto fmin = box.intersect(r, t_min, t_max);
	if (fmin < 0.0f)
		return false;

	return traverse_node(this, r, t_min, t_max, rec);
}
#elif defined BVH_ITERATIVE
bool bvh_node::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {
	float fmin = box.intersect(r, t_min, t_max);
	if (fmin < 0.0f)
		return false;

	static hittable* bvh_stack[100];
	static hittable* candidate_list[100];
	size_t candidate_count = 0;

	// stack index
	auto si = 0;
	bvh_stack[si++] = left.get();
	bvh_stack[si++] = right.get();

	// create candidate list
	while (si > 0) {
		const auto node = (bvh_node*)(bvh_stack[--si]);

		float fmin = node->box.intersect(r, t_min, t_max);
		if (fmin < 0.0f)
			continue;

		// if child is leaf add to candidate list
		if (node->right == nullptr) {
			candidate_list[candidate_count++] = node->left.get();
			continue;
		}

		bvh_stack[si++] = node->left.get();
		bvh_stack[si++] = node->right.get();
	}

	if (candidate_count == 0)
		return false;

	// iterative candidate tests
	bool any_hit = candidate_list[0]->hit(r, t_min, t_max, rec);
	for (size_t i = 1; i < candidate_count; i++) {
		hit_record this_rec;
		bool this_hit = candidate_list[i]->hit(r, t_min, t_max, this_rec);
		if (this_hit && (!any_hit || this_rec.t < rec.t)) {
			any_hit = true;
			rec = this_rec;
		}
	}

	if (any_hit)
		return true;

	return false;
}
#endif

#endif