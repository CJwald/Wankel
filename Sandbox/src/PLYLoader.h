#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

struct PLYMeshData
{
    std::vector<float> Vertices;   // x y z r g b a
    std::vector<uint32_t> Indices;
};

class PLYLoader
{
public:
    static PLYMeshData Load(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "[PLY] Failed to open file: " << path << std::endl;
            return {};
        }

        std::string line;
        uint32_t vertexCount = 0;
        uint32_t faceCount = 0;

        auto trim = [](std::string& s)
        {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };

        // ================= HEADER =================
        std::cout << "[PLY] Parsing header...\n";

        while (std::getline(file, line))
        {
            trim(line);

            if (line.find("element vertex") != std::string::npos)
            {
                std::stringstream ss(line);
                std::string tmp;
                ss >> tmp >> tmp >> vertexCount;
                std::cout << "[PLY] Vertex count: " << vertexCount << "\n";
            }
            else if (line.find("element face") != std::string::npos)
            {
                std::stringstream ss(line);
                std::string tmp;
                ss >> tmp >> tmp >> faceCount;
                std::cout << "[PLY] Face count: " << faceCount << "\n";
            }
            else if (line == "end_header")
            {
                std::cout << "[PLY] End header found\n";
                break;
            }
        }

        PLYMeshData data;

        // ================= VERTICES =================
        std::cout << "[PLY] Reading vertices...\n";

        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            std::getline(file, line);
            std::stringstream ss(line);
			float x, y, z;
			
			int r, g, b, a;
			
			// read position
			ss >> x >> y >> z;
			
			// Blender -> Engine conversion
			float ex = -y;
			float ey = z;
			float ez = -x;
			
			// read color (IMPORTANT: uchar = int)
			ss >> r >> g >> b >> a;
			
			// normalize
			float rf = r / 255.0f;
			float gf = g / 255.0f;
			float bf = b / 255.0f;
			float af = a / 255.0f;
			
			// push vertex
			data.Vertices.push_back(ex);
			data.Vertices.push_back(ey);
			data.Vertices.push_back(ez);
			
			data.Vertices.push_back(rf);
			data.Vertices.push_back(gf);
			data.Vertices.push_back(bf);
			data.Vertices.push_back(af);
        }

        std::cout << "[PLY] Vertices loaded: " << data.Vertices.size() / 7 << "\n";

        // ================= FACES =================
        std::cout << "[PLY] Reading faces...\n";

        uint32_t facesRead = 0;

        while (std::getline(file, line))
        {
            trim(line);

            if (line.empty())
                continue;

            std::stringstream ss(line);

            int n = 0;
            if (!(ss >> n))
                continue;

            if (n < 3)
                continue;

            std::vector<uint32_t> idx(n);

            for (int i = 0; i < n; ++i)
            {
                if (!(ss >> idx[i]))
                {
                    std::cout << "[PLY] Face parse failed: " << line << "\n";
                    goto skip_face;
                }
            }

            // triangulate (fan method)
            for (int i = 1; i + 1 < n; ++i)
            {
                data.Indices.push_back(idx[0]);
                data.Indices.push_back(idx[i]);
                data.Indices.push_back(idx[i + 1]);
            }

            facesRead++;

        skip_face:
            continue;
        }

        std::cout << "[PLY] Faces processed: " << facesRead << "\n";
        std::cout << "[PLY] Final index count: " << data.Indices.size() << "\n";

        return data;
    }
};
