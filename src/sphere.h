#pragma once
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

static const float PI = glm::pi<float>();

class Sphere {
public:
    std::vector<glm::vec4> verts;
    std::vector<glm::vec4> normals;
    std::vector<glm::vec2> texCoords;

    int nLongitude = 0;
    int nLatitude = 0;

    Sphere(int nLongi, int nLati) { makeUV(nLongi, nLati); computeNormals(); }
    ~Sphere() {
        std::vector<glm::vec4>().swap(verts);
        std::vector<glm::vec4>().swap(normals);
        std::vector<glm::vec2>().swap(texCoords);
    }

private:
    void makeUV(int nLongi, int nLati);
    void computeNormals();
};
