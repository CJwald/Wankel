#pragma once

#include <memory>
#include <string>

namespace Wankel {

	class Mesh;

	class MeshLoader {
	public:
		static std::unique_ptr<Mesh> Load(const std::string& filepath);
	};

}
