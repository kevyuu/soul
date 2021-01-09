#pragma once
#include "../data.h"

namespace Blender {
	#include "DNA_light_types.h"
}

namespace BlenKyu {

	enum class LightType : uint8 {
		SUNLIGHT,
		COUNT
	};

	class Light {
	public:
		Light::Light(void* blenderPtr) {
			_blenderLight = (Blender::Light*) blenderPtr;
		}
		KyuRen::SunLight sunLight() {
			KyuRen::SunLight sunLight;
			sunLight.color = Soul::Vec3f(_blenderLight->r, _blenderLight->r, _blenderLight->b);
			sunLight.energy = _blenderLight->energy;
			return sunLight;
		}

		BlenKyu::LightType type() {
			switch (_blenderLight->type) {
			case LA_LOCAL:
				SOUL_LOG_INFO("Create local light");
				break;
			case LA_SUN:
				return LightType::SUNLIGHT;
				break;
			case LA_SPOT:
				SOUL_LOG_INFO("Create spot light");
				break;
			case LA_AREA:
				SOUL_LOG_INFO("Create area light");
				break;
			default:
				SOUL_LOG_INFO("Light type unknown");
				break;
			}
			return LightType::COUNT;
		}

	private:
		Blender::Light* _blenderLight;
	};

}
