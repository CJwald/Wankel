#include "MeshLoader.h"

#include "PLYLoader.h"
#include "Wankel/Renderer/Mesh.h"

#include <filesystem>
#include <stdexcept>

namespace Wankel {

	std::unique_ptr<Mesh> MeshLoader::Load(const std::string& filepath) {

		std::filesystem::path path(filepath);
		std::string extension = path.extension().string();

		// lowercase safety
		for (char& c : extension)
			c = (char)tolower(c);

		if (extension == ".ply") {

			auto meshData = PLYLoader::Load(filepath);

			return std::make_unique<Mesh>(
				meshData.Vertices.data(),
				meshData.Vertices.size() * sizeof(float),
				meshData.Indices.data(),
				(uint32_t)meshData.Indices.size()
			);
		}

		throw std::runtime_error(
			"Unsupported mesh format: " + extension
		);
	}

}
