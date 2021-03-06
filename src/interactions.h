#pragma once

#include "intersections.h"

#define epsilon  1e-3f

#define SCHLICK 1
#define HEMISPHERE 0

// CHECKITOUT
/**
 * Computes a cosine-weighted random direction in a hemisphere.
 * Used for diffuse lighting.
 */
__host__ __device__
glm::vec3 calculateRandomDirectionInHemisphere(
        glm::vec3 normal, thrust::default_random_engine &rng) {
    thrust::uniform_real_distribution<float> u01(0, 1);

#if HEMISPHERE
	float s = u01(rng);
	float t = u01(rng);

	float u = TWO_PI*s;
	float v = sqrt(1-t);

	return glm::vec3(v*cos(u), sqrt(t), v * sin(u));
#else
    float up = sqrt(u01(rng)); // cos(theta)
    float over = sqrt(1 - up * up); // sin(theta)
    float around = u01(rng) * TWO_PI;

    // Find a direction that is not the normal based off of whether or not the
    // normal's components are all equal to sqrt(1/3) or whether or not at
    // least one component is less than sqrt(1/3). Learned this trick from
    // Peter Kutz.

    glm::vec3 directionNotNormal;
    if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(1, 0, 0);
    } else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(0, 1, 0);
    } else {
        directionNotNormal = glm::vec3(0, 0, 1);
    }

    // Use not-normal direction to generate two perpendicular directions
    glm::vec3 perpendicularDirection1 =
        glm::normalize(glm::cross(normal, directionNotNormal));
    glm::vec3 perpendicularDirection2 =
        glm::normalize(glm::cross(normal, perpendicularDirection1));

    return up * normal
        + cos(around) * over * perpendicularDirection1
        + sin(around) * over * perpendicularDirection2;
#endif
}

/**
 * Scatter a ray with some probabilities according to the material properties.
 * For example, a diffuse surface scatters in a cosine-weighted hemisphere.
 * A perfect specular surface scatters in the reflected ray direction.
 * In order to apply multiple effects to one surface, probabilistically choose
 * between them.
 * 
 * The visual effect you want is to straight-up add the diffuse and specular
 * components. You can do this in a few ways. This logic also applies to
 * combining other types of materias (such as refractive).
 * 
 * - Always take an even (50/50) split between a each effect (a diffuse bounce
 *   and a specular bounce), but divide the resulting color of either branch
 *   by its probability (0.5), to counteract the chance (0.5) of the branch
 *   being taken.
 *   - This way is inefficient, but serves as a good starting point - it
 *     converges slowly, especially for pure-diffuse or pure-specular.
 * - Pick the split based on the intensity of each material color, and divide
 *   branch result by that branch's probability (whatever probability you use).
 *
 * This method applies its changes to the Ray parameter `ray` in place.
 * It also modifies the color `color` of the ray in place.
 *
 * You may need to change the parameter list for your purposes!
 */
__host__ __device__
void scatterRay(
PathSegment & pathSegment,
float t,
glm::vec3 normal,
const Material &m,
thrust::default_random_engine &rng) {
	// A basic implementation of pure-diffuse shading will just call the
	// calculateRandomDirectionInHemisphere defined above.

	thrust::uniform_real_distribution<float> u01(0, 1);
	glm::vec3 color;
	glm::vec3 direction;


	pathSegment.color *= m.color;
	pathSegment.ray.origin = getPointOnRay(pathSegment.ray, t) + epsilon * normal;


	if (m.hasReflective > 0.0f && m.hasRefractive > 0.0f)
	{
#if SCHLICK
		float R0 = powf(((1.0f - m.indexOfRefraction) / (1.0f + m.indexOfRefraction)), 2);

		float specular = R0 + (1 - R0)*powf((1 - glm::dot(normal, pathSegment.ray.direction)),5);
		float diffuse = 1.0f - specular;
		direction = specular * glm::reflect(pathSegment.ray.direction, normal)
			+ diffuse * calculateRandomDirectionInHemisphere(normal, rng);
#else
		float diffuse = m.hasRefractive / (m.hasReflective + m.hasRefractive);
		float specular = 1 - diffuse;
		direction = specular * glm::reflect(pathSegment.ray.direction, normal)
			+ diffuse * calculateRandomDirectionInHemisphere(normal, rng);
#endif
	}
	else if (m.hasReflective > 0.0f)
	{
		//reflect
		direction = glm::reflect(pathSegment.ray.direction, normal);
	}
	else
	{
		//diffuse
		direction = calculateRandomDirectionInHemisphere(normal, rng);
	}

	pathSegment.ray.direction = direction;
}
