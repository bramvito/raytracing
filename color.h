#ifndef COLOR_H
#define COLOR_H

#include "vec3.h"

#include <iostream>

color inferno(float t) {
    const color c0 = color(0.0002189403691192265, 0.001651004631001012, -0.01948089843709184);
    const color c1 = color(0.1065134194856116, 0.5639564367884091, 3.932712388889277);
    const color c2 = color(11.60249308247187, -3.972853965665698, -15.9423941062914);
    const color c3 = color(-41.70399613139459, 17.43639888205313, 44.35414519872813);
    const color c4 = color(77.162935699427, -33.40235894210092, -81.80730925738993);
    const color c5 = color(-71.31942824499214, 32.62606426397723, 73.20951985803202);
    const color c6 = color(25.13112622477341, -12.24266895238567, -23.07032500287172);

    return c0+t*(c1+t*(c2+t*(c3+t*(c4+t*(c5+t*c6)))));
}

void write_color_ppm(std::ostream &out, color pixel_color, int samples_per_pixel) {
	auto r = pixel_color.r();
	auto g = pixel_color.g();
	auto b = pixel_color.b();

	// Divide the color total by the number of samples and gamma-correct for gamma=2.0.
	auto scale = 1.0 / samples_per_pixel;
	r = sqrt(scale * r);
	g = sqrt(scale * g);
	b = sqrt(scale * b);

    // Write the translated [0,255] value of each color component.
    out << static_cast<int>(256 * clamp(r, 0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(g, 0, 0.999)) << ' '
        << static_cast<int>(256 * clamp(b, 0, 0.999)) << '\n';
}

void rgb_from_hdr(uint8_t pixel[3], color c, int samples_per_pixel) {
	auto r = c.r();
	auto g = c.g();
	auto b = c.b();

	// Divide the color total by the number of samples and gamma-correct for gamma=2.0.
	auto scale = 1.0 / samples_per_pixel;
	r = sqrt(scale * r);
	g = sqrt(scale * g);
	b = sqrt(scale * b);

    // Write the translated [0,255] value of each color component.
    pixel[0] = static_cast<int>(256 * clamp(r, 0, 0.999));
    pixel[1] = static_cast<int>(256 * clamp(g, 0, 0.999));
    pixel[2] = static_cast<int>(256 * clamp(b, 0, 0.999));
}

#endif
