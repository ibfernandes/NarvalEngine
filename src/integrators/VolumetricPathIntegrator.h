#pragma once
#include <glm/glm.hpp>
#include "integrators/Integrator.h"
#include "lights/Light.h"
#include "primitives/Primitive.h"
#include "utils/Math.h"
#include "core/BSDF.h"
#include "materials/Medium.h"
#define NE_DEBUG_SAMPLE_EVERY_N 10000

namespace narvalengine {

	class VolumetricPathIntegrator : public Integrator {
	private:
		bool shouldDebug() {
		#ifdef NE_DEBUG_MODE
			return !(debugCount%NE_DEBUG_SAMPLE_EVERY_N);
		#else
			return false;
		#endif
		}
	public:
		/**
		 * After {@code NE_DEBUG_SAMPLE_EVERY_N} calls to Li(), we store a full path for debugging purposes.
		 */
		static uint32_t debugCount;
		std::vector<PointPathDebugInfo> path;
		std::vector<std::vector<PointPathDebugInfo>> visibilityPath;


		bool intersectTr(Ray ray, RayIntersection &intersection, glm::vec3* Tr, Scene* s);
		
		/**
		 * Calculats the visibility ray between {@code intersecPoint} and {@code lightPoint}.
		 *	Returns (0,0,0) if an opaque object is hit, (1,1,1) if nothing is hit or the calculated transmittance if a medium is hit.
		 *  
		 * @param intersecPoint in World Coordinate System (WCS).
		 * @param lightPoint in World Coordinate System (WCS).
		 * @param scene.
		 * @return the transmittance between {@code intersecPoint} and {@code lightPoint}.
		 */
		glm::vec3 visibilityTr(glm::vec3 intersecPoint, glm::vec3 lightPoint, Scene* scene);
		
		/**
		 * Estimates direct lightning. Given the point at {@code intersection}, we trace a path directly to {@code light}.
		 * 
		 * @param incoming ray in World Coordinate System (WCS) pointing towards {@code intersection}.
		 * @param intersecOnASurface point where this {@code incoming} ray hit an object in this {@code scene}.
		 * @param light to which sample from.
		 * @param lightIm InstancedModel of this {@code light}.
		 * @param scene.
		 * @return Radiance arriving at {@code intersection}.
		 */
		glm::vec3 estimateDirect(Ray incoming, RayIntersection intersecOnASurface, Light* light, InstancedModel* lightIm, Scene* scene);
		
		/**
		 * Randomly chooses one light from this {@code scene} and samples it.
		 * 
		 * @param incoming ray in World Coordinate System (WCS) pointing towards {@code intersection}.
		 * @param intersection point where this {@code incoming} ray hit an object in this {@code scene}.
		 * @param scene.
		 * @return Radiance arriving at {@code intersection}.
		 */
		glm::vec3 uniformSampleOneLight(Ray incoming, RayIntersection intersection, Scene* scene);
		glm::vec3 Li(Ray incoming, Scene* scene);
		Integrator* clone();
	};
}