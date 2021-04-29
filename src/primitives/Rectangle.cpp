#include "Rectangle.h"

namespace narvalengine {
	Rectangle::Rectangle() {
	}

	glm::vec3 Rectangle::getVertex(int n) {
		return glm::vec3(*vertexData[n], *(vertexData[n] + 1), *(vertexData[n] + 2));
	}

	glm::vec3 Rectangle::getSize() {
		glm::vec3 v0 = getVertex(0);
		glm::vec3 v1 = getVertex(1);

		for (int i = 0; i < 3; i++)
			if (v0[i] == v1[i])
				v0[i] = v1[i] = 0;

		return glm::abs(v0) + glm::abs(v1);
	}

	glm::vec3 Rectangle::getCenter() {

		//Assumes that v[1] is the Rectangle's max
		return getVertex(1) - getSize() / 2.0f;
	}

	//TODO: check same hemisphere (rectangle one sided)
	//TODO: I could strictly define the plane on a xy axis since the ray must be in OCS
	bool Rectangle::containsPoint(glm::vec3 point) {
		glm::vec3 planeVertex1 = glm::vec3(*vertexData[0], *(vertexData[0] + 1), *(vertexData[0] + 2));
		glm::vec3 planeVertex2 = glm::vec3(*vertexData[1], *(vertexData[1] + 1), *(vertexData[1] + 2));

		glm::vec3 min = glm::min(planeVertex1, planeVertex2);
		glm::vec3 max = glm::max(planeVertex1, planeVertex2);
		glm::vec3 size = getSize();

		if (size.x > 0 && (point.x < min.x || point.x > max.x))
			return false;
		if (size.y > 0 && (point.y < min.y || point.y > max.y))
			return false;
		if (size.z > 0 && (point.z < min.z || point.z >  max.z))
			return false;

		return true;
	}

	/*
		Checks if Ray r intesercts this primitive. If true, stores its values on hit.
		Ray must be in OCS.
	*/
	bool Rectangle::intersect(Ray r, RayIntersection &hit) {
		//glm::vec3 planeVertex = getCenter();
		glm::vec3 planeVertex = getVertex(1);

		float denom = glm::dot(normal, r.direction);
		if (glm::abs(denom) < EPSILON)
			return false;

		float num = glm::dot(normal, planeVertex - r.origin);
		float t = num / denom;

		if (t < 0)
			return false;

		glm::vec3 pointOnPlaneAlongRay = r.getPointAt(t);
		if (containsPoint(pointOnPlaneAlongRay)) {

			hit.tNear = t;
			hit.tFar = t;
			hit.normal = normal;
			hit.hitPoint = pointOnPlaneAlongRay;
			hit.primitive = this;
			hit.uv = samplePointOnTexture(pointOnPlaneAlongRay);
			return true;
		}

		return false;
	}

	//TODO abs of width of both vertices
	glm::vec3 Rectangle::samplePointOnSurface(RayIntersection interaction, glm::mat4 transformToWCS) {
		glm::vec3 e(random(), random(), random());
		glm::vec3 v0 = getVertex(0);
		glm::vec3 size = getSize();
		glm::vec3 pointOnSurface = glm::vec3(v0.x + size.x * e[0], v0.y + size.y * e[1], v0.z + size.z * e[2]);

		pointOnSurface = transformToWCS * glm::vec4(pointOnSurface, 1.0f);

		//TODO barycentric or something more elegant than this 
		return pointOnSurface;
	}

	//TODO not the most eficient/elegant solution (?)
	glm::vec4 Rectangle::barycentricCoordinates(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
		glm::vec3 uvwT1;
		glm::vec3 uvwT2;

		glm::vec3 v0 = b - a, v1 = c - a, v2 = p - a;
		float d00 = glm::dot(v0, v0);
		float d01 = glm::dot(v0, v1);
		float d11 = glm::dot(v1, v1);
		float d20 = glm::dot(v2, v0);
		float d21 = glm::dot(v2, v1);
		float denom = d00 * d11 - d01 * d01;
		uvwT1[1] = (d11 * d20 - d01 * d21) / denom;
		uvwT1[2] = (d00 * d21 - d01 * d20) / denom;
		uvwT1[0] = 1.0f - uvwT1[1] - uvwT1[2];

		v0 = b - d, v1 = c - d, v2 = p - d;
		d00 = glm::dot(v0, v0);
		d01 = glm::dot(v0, v1);
		d11 = glm::dot(v1, v1);
		d20 = glm::dot(v2, v0);
		d21 = glm::dot(v2, v1);
		denom = d00 * d11 - d01 * d01;
		uvwT2[1] = (d11 * d20 - d01 * d21) / denom;
		uvwT2[2] = (d00 * d21 - d01 * d20) / denom;
		uvwT2[0] = 1.0f - uvwT2[1] - uvwT2[2];

		if (uvwT1.x < 0 || uvwT1.y < 0 || uvwT1.z < 0 || uvwT1.x > 1 || uvwT1.y > 1 || uvwT1.z > 1)
			return glm::vec4(uvwT2, 1);
		else
			return glm::vec4(uvwT1, 0);
	}

	//Given a hit point on surface, convert it to texture coordinates
	glm::vec2 Rectangle::samplePointOnTexture(glm::vec3 pointOnSurface) {
		if(vertexLayout == nullptr || !vertexLayout->contains(VertexAttrib::TexCoord0))
			return glm::vec3(0);
		else {
			glm::vec2 uv[4];
			int stride = vertexLayout->stride / sizeof(float);
			int texOffset = vertexLayout->offset[VertexAttrib::TexCoord0] / sizeof(float);
			int vertexOffset = vertexLayout->offset[VertexAttrib::Position] / sizeof(float);
			float *texPointer = vertexData[0] + texOffset;

			uv[0] = *((glm::vec2*)texPointer);
			uv[1] = *((glm::vec2*)(texPointer + stride * 1));
			uv[2] = *((glm::vec2*)(texPointer + stride * 2));
			uv[3] = *((glm::vec2*)(texPointer + stride * 3));

			glm::vec3 vertices[4];
			vertices[0] = *((glm::vec3*)vertexData[0]);
			vertices[1] = *((glm::vec3*)(vertexData[0] + stride * 1));
			vertices[2] = *((glm::vec3*)(vertexData[0] + stride * 2));
			vertices[3] = *((glm::vec3*)(vertexData[0] + stride * 3));

			glm::vec4 baryCoord = barycentricCoordinates(pointOnSurface, vertices[0], vertices[1], vertices[2], vertices[3]);
			glm::vec3 lambda = glm::vec3(baryCoord);

			glm::vec2 res;
			//abc
			if (baryCoord.w == 0)
				// w * A + u * B + v * C
				res = glm::vec2(lambda.x * uv[0] + lambda.y * uv[1] + lambda.z * uv[2]);
			else //dbc
				// w * D + u * B + v * C
				res = glm::vec2(lambda.x * uv[3] + lambda.y * uv[1] + lambda.z * uv[2]);
			
			return res;
		}
	}

	float Rectangle::pdf(RayIntersection interaction, glm::mat4 transformToWCS) {
		glm::vec3 sizeWCS = glm::vec3(transformToWCS[0][0], transformToWCS[1][1], transformToWCS[2][2]) * getSize();
		//TODO confirm this math here

		float area = 1;

		for (int i = 0; i < 3; i++)
			if (sizeWCS[i] != 0)
				area = area * sizeWCS[i];

		// 1 / area
		return 1.0f / area;
	}
}