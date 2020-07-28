#include "rtweekend.h"

#include "color.h"
#include "hittable_list.h"
#include "sphere.h"
#include "triangle.h"
#include "camera.h"
#include "material.h"
#include "bvh.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // to be able to save png's
#define CHANNEL_NUM 3 // 3 channels (rgb)

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono> // to time the raytracing

#include <SDL2/SDL.h> // so we can show the result as it improves
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;


color ray_color(const ray& r, color& background, const hittable& world, int depth) {
	//std::cout << "shooting ray ------------------------------------- \n";
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0) {
		return color(0, 0, 0);
	}

	if (!world.hit(r, 0.001, infinity, rec)) {
		#ifdef BVH_HEATMAP
		return rec.num_bvh_node_intersects * vec3(1,1,1);
		#endif

		// // background gradient
		// vec3 unit_direction = unit_vector(r.direction());
		// auto t = 0.5*(unit_direction.y() + 1.0);
		// return (1.0-t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0); // blue sky
		// //return (1.0-t) * color(0.2, 0.2, 0.2) + t * color(0.0, 0.0, 0.3); // night sky

		return background;
	}

	ray scattered;
	color attenuation;
	color emitted = rec.mat_ptr->emitted();

	if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
		return emitted;

	#ifdef DEBUG
	#ifdef BVH_HEATMAP
	// return color dependent on amount of bvh node intersects for testing
	return rec.num_bvh_node_intersects * vec3(1,1,1);
	#endif

	// return normal for testing
	//return 0.5 * (rec.normal + vec3(1,1,1));
	#endif

	return emitted + attenuation * ray_color(scattered, background, world, depth - 1);
}


hittable_list load_obj(std::string filename, double scale, point3 pos, shared_ptr<material> m) {
	hittable_list triangles;

	// load obj file
    std::ifstream obj_file;
	obj_file.open(filename);

	// load file into vectors
	std::vector<point3> verts;
	std::vector<std::vector<int>> faces;
	std::string line;

	while (getline(obj_file, line)) {
		std::string type;
		std::istringstream in(line);
		in >> type;
		if (type == "v") {
			double x, y, z;
			in >> x >> y >> z;
			point3 vertex(
				x * scale + pos.x(),
				y * scale + pos.y(), 
				z * scale + pos.z());
			verts.push_back(vertex);
		} else if (type == "f") {

			char delimiter_space = ' ';
			char delimiter_slash = '/';
			std::string vert_idx_slash = "";
			std::vector<int> face;

			// normal triangle face
			for (int i = 0; i < 3; ++i) {
				in >> vert_idx_slash;

				// get only the number before the first slash
				std::string vertex_index = vert_idx_slash.substr(0, vert_idx_slash.find(delimiter_slash));

				face.push_back(std::stoi(vertex_index) - 1);

				std::getline(in, vert_idx_slash, delimiter_space);
			}

			faces.push_back(face);

			// if their is a fourth vertex make a second triangle
			in >> vert_idx_slash;
			if (vert_idx_slash.size() > 1) {

				// get only the number before the first slash
				std::string vertex_index = vert_idx_slash.substr(0, vert_idx_slash.find(delimiter_slash));

				std::vector<int> face2;
				face2.push_back(face[0]);
				face2.push_back(face[2]);
				face2.push_back(std::stoi(vertex_index) - 1);
				faces.push_back(face2);
			}
		}
    }

	obj_file.close();

	// make a triangle for every face
	for (auto &face : faces) {
		point3 v0 = verts[face[0]];
		point3 v1 = verts[face[1]];
		point3 v2 = verts[face[2]];
		triangles.add(make_shared<triangle>( v0, v1, v2, m ));
		num_triangles += 1;
	}

	return triangles;
}


void create_scene_blocks(std::unique_ptr<hittable>& pWorld, camera& cam, size_t& image_width, size_t& image_height) {
	auto aspect_ratio = 9.0 / 16.0;
	image_width = 270;
	image_height = static_cast<size_t>(image_width / aspect_ratio);

	cam.aspect_ratio(aspect_ratio);
    cam.lookfrom(point3(6.92, 4.96, 7.36));
    cam.lookat(point3(0,0,0));

    hittable_list objects;
    objects.add(load_obj("blocks.obj", 1, point3(0,0,0), make_shared<lambertian>(color(1, 1, 1))));

    pWorld = std::make_unique<bvh_node>(objects);
}

void create_scene_street(std::unique_ptr<hittable>& pWorld, camera& cam, size_t& image_width, size_t& image_height) {
	auto aspect_ratio = 9.0 / 16.0;
	image_width = 540;
	image_height = static_cast<size_t>(image_width / aspect_ratio);

	cam.aspect_ratio(aspect_ratio);
    cam.lookfrom(point3(-0.631397, 2.43137, 9.73438));
    cam.lookat(point3(0,2.43137,0));
    cam.vfov(64);

    hittable_list objects;
    objects.add(load_obj("street.obj", 1, point3(0,0,0), make_shared<lambertian>(color(1, 1, 0.7))));

	// // add light
 //    auto light_white = make_shared<diffuse_light>(color(1, 1, 1));
 //    auto light_yellow = make_shared<diffuse_light>(color(0.4, 0.4, 0.2));
	// objects.add(make_shared<sphere>( point3(7,1,-6), 5, light_yellow ));
	// objects.add(make_shared<sphere>( point3(7,1,0), 5, light_yellow ));
	// objects.add(make_shared<sphere>( point3(-0.6,2.4,15), 5, light_yellow )); // behind camera
	// objects.add(make_shared<sphere>( point3(-5,1,1), 2, light_yellow )); // in the shop
	// objects.add(make_shared<sphere>( point3(-1,4,0), 1, light_white ));

	// // metal ball
	// auto material_metal = make_shared<metal>(color(1, 1, 1), 0.2);
	// objects.add(make_shared<sphere>( point3(0,0.5,2), 0.5, material_metal ));

    pWorld = std::make_unique<bvh_node>(objects);
}

void create_scene_room(std::unique_ptr<hittable>& pWorld, camera& cam, size_t& image_width, size_t& image_height) {
	auto aspect_ratio = 3.0 / 2.0;
	image_width = 600;
	image_height = static_cast<size_t>(image_width / aspect_ratio);

	cam.aspect_ratio(aspect_ratio);
    cam.lookfrom(point3(2.68677, 4.47732, 4.0692));
    cam.lookat(point3(-8.38018, 3.53226, -4.39985));
    cam.vfov(50);

    hittable_list objects;

    auto white = make_shared<lambertian>(color(1, 1, 1));
    auto yellow = make_shared<lambertian>(color(1, 1, 0.2));
    auto orange = make_shared<lambertian>(color(1, 0.5, 0.1));
    auto grey = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    auto mirror = make_shared<metal>(color(1, 1, 1), 0.0);
    auto green = make_shared<lambertian>(color(0.2, 1, 0.2));
    auto yellow_light = make_shared<diffuse_light>(color(1, 1, 0.5));
    objects.add(load_obj("room_obj_files/room.obj", 1, point3(0,0,0), white));
    objects.add(load_obj("room_obj_files/window_mirror_frame_lamp.obj", 1, point3(0,0,0), yellow));
    objects.add(load_obj("room_obj_files/wardrobe_pot.obj", 1, point3(0,0,0), orange));
    objects.add(load_obj("room_obj_files/plant.obj", 1, point3(0,0,0), green));
    objects.add(load_obj("room_obj_files/mirror.obj", 1, point3(0,0,0), mirror));
    objects.add(load_obj("room_obj_files/lamp_stand.obj", 1, point3(0,0,0), grey));
    objects.add(load_obj("room_obj_files/lamp_light.obj", 1, point3(0,0,0), yellow_light));


    pWorld = std::make_unique<bvh_node>(objects);
}


void create_scene_balls(std::unique_ptr<hittable>& pWorld, camera& cam, size_t& image_width, size_t& image_height) {
	image_width = 600;
	image_height = 400;

	cam.lookfrom(point3(-4, 2, 4));
	cam.lookat(point3(0, 1, -1));
	cam.vfov(80);

	hittable_list world;

    auto ground_material = make_shared<lambertian>(color(1, 1, 0.2));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto light = make_shared<diffuse_light>(color(1, 1, 1));
    world.add(make_shared<sphere>(point3(-5, 8, -5), 3.0, light));
    world.add(make_shared<sphere>(point3(-5, 8, +5), 3.0, light));
    world.add(make_shared<sphere>(point3(+5, 8, -5), 3.0, light));
    world.add(make_shared<sphere>(point3(+5, 8, +5), 3.0, light));

    auto material1 = make_shared<lambertian>(color(1, 0.2, 0.2));
    world.add(make_shared<sphere>(point3(0, 1, -2.5), 1.0, material1));

    auto material2 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(0, 1, 2.5), 1.0, material3));

    auto material4 = make_shared<metal>(color(0.2, 0.5, 1), 0.4);
    world.add(make_shared<sphere>(point3(0, 2, -6), 2.0, material4));

    auto material5 = make_shared<lambertian>(color(1, 1, 1));
    world.add(load_obj("bunny.obj", 13, point3(-3,-0.4,0), material5));

    //return std::make_unique<hittable_list>(world);
    pWorld = std::make_unique<bvh_node>(world);
}


void create_scene_bunny(std::unique_ptr<hittable>& pWorld, camera& cam, size_t& image_width, size_t& image_height) {
	image_width = 200;
	image_height = 200;

	cam.aspect_ratio(1);
    cam.lookfrom(point3(0, 1, 4));
    cam.lookat(point3(0,1,0));
    cam.vfov(40);

	hittable_list objects;

	// load bunny
	auto material_bunny = make_shared<lambertian>(color(.4, .2, .8));
	objects.add(load_obj("bunny.obj", 7, point3(-0.2,-0.3,0), material_bunny));

	auto material_red = make_shared<metal>(color(.8, .2, .2), 0.5);
	auto material_green = make_shared<metal>(color(.2, .8, .2), 0.05);
	objects.add(make_shared<sphere>(point3(0.6,0.3,0), 0.3, material_red));
	objects.add(make_shared<sphere>(point3(0.6,0.9,0), 0.3, material_green));

	// auto material_glass = make_shared<dielectric>(1.5);
	// objects.add(make_shared<sphere>(point3(0.2,0.3,0.6), 0.3, material_glass));

    // add light
    objects.add(make_shared<sphere>( point3(0,2,0), 0.3, make_shared<diffuse_light>(color(1, 1, 1)) ));

	// load ground and walls
	auto box_material = make_shared<lambertian>(color(1, 1, 1));
	objects.add(load_obj("box_one_face_open.obj", 1, point3(0,0,0), box_material));

    pWorld = std::make_unique<bvh_node>(objects);
}

void create_scene_blob(std::unique_ptr<hittable>& pWorld, camera& cam, size_t& image_width, size_t& image_height) {
	auto aspect_ratio = 9.0 / 16.0;
	image_width = 540;
	image_height = static_cast<size_t>(image_width / aspect_ratio);

	cam.aspect_ratio(aspect_ratio);
    cam.lookfrom(point3(3,3,3));
    cam.lookat(point3(0,0,0));

	hittable_list objects;

	// load blob
	auto material_blob = make_shared<lambertian>(color(1, 1, 1));
	objects.add(load_obj("distorted_blob.obj", 0.7, point3(0,0,0), material_blob));

	auto red_light = make_shared<diffuse_light>(color(2, .2, .2));
	auto blue_light = make_shared<diffuse_light>(color(.2, .2, 2));
	objects.add(make_shared<sphere>(point3(-150,0,0), 100, blue_light));
	objects.add(make_shared<sphere>(point3(150,0,0), 100, red_light));

    pWorld = std::make_unique<bvh_node>(objects);
}


int main() {
	const int samples_per_pixel = 16*32;
	const int max_depth = 10;

	// default values
    color background(0,0,0);
    size_t image_width = 300;
    size_t image_height = 200;
    camera cam(
    	point3(1,1,1),                    // look from
    	point3(0,0,0),                    // lookat
    	vec3(0,1,0),                      // vup 
    	50,                               // vfov
    	(float)image_width / image_height // aspect_ratio
    );
    std::unique_ptr<hittable> pWorld;
	create_scene_balls(pWorld, cam, image_width, image_height);

	// SDL stuff
    SDL_Init(SDL_INIT_VIDEO); // initialize SDL
    gWindow = SDL_CreateWindow("rtweekend", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, image_width, image_height, SDL_WINDOW_SHOWN);
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_PRESENTVSYNC); // create renderer
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255); // initialize renderer color
    SDL_SetRenderDrawBlendMode(gRenderer, SDL_BLENDMODE_BLEND); // allow colors with alpha transparancy values
	
    // start timer
    using Time = std::chrono::high_resolution_clock; 
    using fsec = std::chrono::duration<float>; 
    auto time_start = Time::now();

    // initialize pixel hdr array to (0, 0, 0) vectors
    vec3* pixels_hdr = new vec3[image_width * image_height];
    for (size_t i = 0; i < image_width * image_height; i++) {
		pixels_hdr[i] = color(0, 0, 0);
	}
	uint8_t* pixels_rgb = new uint8_t[image_width * image_height * CHANNEL_NUM]; // 3 channels (rgb)

	// SDL preview texture
    SDL_Texture *preview_texture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, image_width, image_height);
    
    for (int s = 0; s < samples_per_pixel; ++s) {
    	std::cerr << "\rSample: " << s << ' ' << std::flush;

    	// OpenMP
    	#ifndef DEBUG
    	#pragma omp parallel for schedule(dynamic, 1)
    	#endif
		for (int j = (int)image_height-1; j >= 0; --j) {
			for (size_t i = 0; i < image_width; ++i) {
				num_primary_rays++;
				auto u = (i + random_double()) / (image_width-1);
				auto v = (j + random_double()) / (image_height-1);
				ray r = cam.get_ray(u, v);

				int idx = (image_height - 1 - j) * image_width + i;
				pixels_hdr[idx] += ray_color(r, background, *pWorld, max_depth);
			}
		}

		#ifdef BVH_HEATMAP
		float max = 0;
		for (size_t i = 0; i < image_width * image_height; i++) {
			color* p = pixels_hdr + i;
			if (p->r() > max)
				max = p->r();
		}

		std::cout << "max number bvh node hits: " << max << "\n";

		for (size_t i = 0; i < image_width * image_height; i++) {
			color* p = pixels_hdr + i;
			*p = inferno(p->x() / max);
			//p = vec3(1,1,1) - p / max; // black and white
		}
		#endif

		// update the pixel_rgb array
		// and show in the SDL preview image

		// clear the screen
		SDL_RenderClear(gRenderer);

		// lock preview_texture
		void *pixels;
		int pitch;
		SDL_LockTexture(preview_texture, NULL, &pixels, &pitch);

		// convert pixels from hdr to rgb
		for (size_t i = 0; i < image_width * image_height; i++) {
			uint8_t* pixel = pixels_rgb + (i * CHANNEL_NUM);
			rgb_from_hdr(pixel, pixels_hdr[i], s + 1);

			// edit the preview texture with the new color values
			Uint8 *base = ((uint8_t*)pixels) + (4 * i);
			base[0] = pixel[2];
			base[1] = pixel[1];
			base[2] = pixel[0];
			base[3] = 255;
		}

		SDL_UnlockTexture(preview_texture); // unlock texture
		SDL_RenderCopy(gRenderer, preview_texture, NULL, NULL); // copy the preview_texture to the rendering context
		SDL_RenderPresent(gRenderer); // draw the screen
	}


	auto time_end = Time::now(); 
    fsec fs = time_end - time_start;

	// save pixel array to output file
	int img_saved = stbi_write_png(
		"output_images/output.png",
		image_width,
		image_height,
		CHANNEL_NUM,
		pixels_rgb,
		image_width * CHANNEL_NUM
	);

    if (img_saved)
    	std::cerr << "\rImage saved with " << samples_per_pixel << " samples per pixel\n";
    else
    	std::cerr << "\rError while saving image\n";

    std::cout << "Info:\n";
    std::cout << "Render time                                 :" << fs.count() << " (sec)\n";
    std::cout << "Total number of triangles                   :" << num_triangles << "\n";
    std::cout << "Total number of primary rays                :" << num_primary_rays << "\n";
    std::cout << "Total number of ray-triangles tests         :" << num_ray_triangle_tests << "\n";
    std::cout << "Total number of ray-triangles intersections :" << num_ray_triangle_intersections << "\n";
    std::cout << "Total number of BVH ray-aabb tests          :" << num_ray_bvh_aabb_tests << "\n";
    std::cout << "Total number of BVH ray-aabb intersections  :" << num_ray_bvh_aabb_intersections << "\n";
    std::cout << "Total number of BVH ray-leaf tests          :" << num_ray_bvh_leaf_tests << "\n";
    std::cout << "Total number of BVH ray-leaf intersections  :" << num_ray_bvh_leaf_intersections << "\n";
    std::cout << "Total number of BVH nodes                   :" << num_bvh_nodes << "\n";
    std::cout << "Total number of BVH leaf nodes              :" << num_bvh_leaf_nodes << "\n";
    std::cout << "Samples per pixel                           :" << samples_per_pixel << "\n";
    std::cout << "Maxium ray depth                            :" << max_depth << "\n";
    std::cout << "Image Dimensions                            :" << image_width << "x" << image_height << "\n";


    // main loop
	while (1) {
		
		// event handling
		SDL_Event e;
		if ( SDL_PollEvent(&e) ) {
			if (e.type == SDL_QUIT)
				break;
			else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_ESCAPE)
				break;
		} 
		
		// clear the screen
		SDL_RenderClear(gRenderer);

		// copy the preview_texture to the rendering context
		SDL_RenderCopy(gRenderer, preview_texture, NULL, NULL);
		// draw the screen
		SDL_RenderPresent(gRenderer);
		
	}

	delete [] pixels_hdr;
	delete [] pixels_rgb;

    // destroy window
    SDL_DestroyRenderer(gRenderer);

    // destroy window
    SDL_DestroyWindow(gWindow);

    // quit SDL subsystems
    SDL_Quit();

    return 0; 
}