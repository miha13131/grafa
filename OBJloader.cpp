#include "OBJloader.hpp"
bool loadOBJ(
    const std::string& path,
    std::vector<glm::vec3>& out_vertices,
    std::vector<glm::vec2>& out_uvs,
    std::vector<glm::vec3>& out_normals
) {
    std::cout << "Loading OBJ file: " << path << std::endl;

    out_vertices.clear();
    out_uvs.clear();
    out_normals.clear();

    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Impossible to open the file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);
        }
        else if (prefix == "vt") {
            glm::vec2 uv;
            iss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "f") {
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            char slash;
            for (int i = 0; i < 3; i++) {
                if (!(iss >> vertexIndex[i] >> slash >> uvIndex[i] >> slash >> normalIndex[i])) {
                    std::cerr << "Invalid face format in OBJ file: " << path << std::endl;
                    file.close();
                    return false;
                }
                vertexIndices.push_back(vertexIndex[i] - 1);
                uvIndices.push_back(uvIndex[i] - 1);
                normalIndices.push_back(normalIndex[i] - 1);
            }
        }
    }
    file.close(); // Explicitní zavření souboru

    // Převod indexů na výstupní vektory
    for (size_t i = 0; i < vertexIndices.size(); i++) {
        unsigned int vertexIndex = vertexIndices[i];
        unsigned int uvIndex = uvIndices[i];
        unsigned int normalIndex = normalIndices[i];

        if (vertexIndex >= temp_vertices.size() ||
            uvIndex >= temp_uvs.size() ||
            normalIndex >= temp_normals.size()) {
            std::cerr << "Invalid index in OBJ file: " << path << std::endl;
            return false;
        }

        out_vertices.push_back(temp_vertices[vertexIndex]);
        out_uvs.push_back(temp_uvs[uvIndex]);
        out_normals.push_back(temp_normals[normalIndex]);
    }

    std::cout << "OBJ loaded successfully: " << out_vertices.size() << " vertices" << std::endl;
    return true;
}