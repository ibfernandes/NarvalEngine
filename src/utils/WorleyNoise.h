#pragma once
#include <math.h>
#include <glm/glm.hpp>

namespace wnoise
{
	//Source: https://www.shadertoy.com/view/3ls3RX
	class WorleyNoise
	{	
	public:

		float rescale(float x, glm::vec2 range){
			float a = range.x, b = range.y;
			return (x - a) / (b - a);
		}

		float rescale(float x, glm::vec2 r1, glm::vec2 r2)
		{
			float a = r1.x, b = r1.y;
			float c = r2.x, d = r2.y;
			return c + (d - c) * ((x - a) / (b - a));
		}
		// simple LCG
		#define LCG(k) k = (65 * k) % 1021
		#define lr(k) float(k)/1021

		// permutation polynomial

		int permp(int i1, int i2)
		{
			int t = (i1 + i2) & 255;

			return ((112 * t + 153) * t + 151) & 255;
		}

		// return the two closest distances for 3D Worley noise
		// type controls the type of metric used

		glm::vec2 worley(int type, glm::vec3 p)
		{
			glm::vec2 dl = glm::vec2(20.0);
			glm::ivec3 iv = glm::ivec3(floor(p));
			glm::vec3 fv = fract(p);

			int j = 0; // initialization for Knuth's "algorithm L"
			glm::ivec3 di = glm::ivec3(1), ki = -di;
			glm::ivec4 fi = glm::ivec4(0, 1, 2, 3);

			// instead of writing a triply nested loop (!!)
			// generate the indices for the neighbors in Gray order (Knuth's "algorithm L")
			// see section 7.2.1.1 of TAOCP, Volume 4A or https://doi.org/10.1145/360336.360343

			for (int k = 0; k < 27; k++) // loop through all neighbors
			{
				// seeding
				int s = permp(permp(permp(0, iv.z + ki.z), iv.y + ki.y), iv.x + ki.x); LCG(s);

				for (int m = 0; m < 2; m++) // two points per cell
				{
					// generate feature points within the cell
					LCG(s); float sz = lr(s);
					LCG(s); float sy = lr(s);
					LCG(s); float sx = lr(s);

					glm::vec3 tp = glm::vec3(ki) + glm::vec3(sx, sy, sz) - fv;
					float c = 0.0;
					if (type == 1) c = glm::dot(tp, tp); // Euclidean metric
					if (type == 2) c = glm::abs(tp.x) + glm::abs(tp.y) + glm::abs(tp.z); // Manhattan metric
					if (type == 3) c = glm::max(glm::abs(tp.x), glm::max(glm::abs(tp.y), glm::abs(tp.z))); // Chebyshev metric

					float m1 = glm::min(c, dl.x); // ranked distances
					dl = glm::vec2(glm::min(m1, dl.y), glm::max(m1, glm::min(glm::max(c, dl.x), dl.y)));
				}

				// updating steps for Knuth's "algorithm L"
				j = fi[0];
				if (j > 2)
					j = 2;
				fi[0] = 0; ki[2 - j] += di[j];
				if ((ki[2 - j] & 1) == 1) {
					di[j] = -di[j];
					fi[j] = fi[j + 1]; fi[j + 1] = j + 1;
				}
			}

			if (type == 1) dl = sqrt(dl); // don't forget to root at the end for Euclidean distance

			return dl;
		}
	};
}
