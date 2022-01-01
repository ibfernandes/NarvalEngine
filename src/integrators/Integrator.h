
#include "primitives/Ray.h"
#include "core/Scene.h"

namespace narvalengine {
	struct PointPathDebugInfo {
		Ray incoming;
		glm::vec3 Tr;
		glm::vec3 bsdf;
		glm::vec3 Li;
	};

	class Integrator {
	public:
		/**
		 * Traces a path starting at {@code incoming.origin} towards {@code incoming.direction}
		 * and calculates the radiance Li accumulated throughout the path.
		 * 
		 * @param incoming ray in World Coordinate System (WCS) containing the initial point and direction. Usually defined as a point in the camera's plane and pointing in a direction close to the camera's forward vector.
		 * @param scene to compute the path against.
		 * @return radiance Li.
		 */
		virtual glm::vec3 Li(Ray incoming, Scene* scene) = 0;
		virtual Integrator *clone() = 0;
	};
}