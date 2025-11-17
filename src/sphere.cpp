#include "sphere.h"
#include <map>

void Sphere::makeUV(int nLongi, int nLati)
{
    int nverts = nLongi * nLati;
    float radius = 1;
    std::vector<glm::vec4> vertList;
    texCoords.clear();

    nLongitude = nLongi;
    nLatitude = nLati;

    // make vertex list
    for (int v = 0; v < nLati + 1; v++)
    {
        for (int u = 0; u < nLongi; u++)
        {
            float theta = 2 * PI * u / nLongi; // longitude
            float phi = PI * v / nLati;        // latitude
            float x = glm::sin(phi) * glm::cos(theta) * radius;
            float y = glm::sin(phi) * glm::sin(theta) * radius;
            float z = glm::cos(phi) * radius;
            vertList.push_back(glm::vec4(x, y, z, 1));
        }
    }

    // make triangles
    for (int v = 0; v < nLati; v++)
    {
        for (int u = 0; u < nLongi; u++)
        {
            int u2 = (u + 1) % nLongi;
            int v2 = v + 1;

            // triangle (u, v), (u, v2), (u2, v2)
            verts.push_back(vertList[u + v * nLongi]);      // v0
            verts.push_back(vertList[u + v2 * nLongi]);     // v2
            verts.push_back(vertList[u2 + v2 * nLongi]);    // v3
            {
                float s0 = float(u) / nLongi; float t0 = 1.0f - float(v) / nLati;
                float s1 = float(u) / nLongi; float t1 = 1.0f - float(v2) / nLati;
                float s2 = float(u2) / nLongi; float t2 = 1.0f - float(v2) / nLati;
                texCoords.push_back(glm::vec2(s0, t0));
                texCoords.push_back(glm::vec2(s1, t1));
                texCoords.push_back(glm::vec2(s2, t2));
            }

            // triangle (u, v), (u2, v2), (u2, v)
            verts.push_back(vertList[u + v * nLongi]);      // v0
            verts.push_back(vertList[u2 + v2 * nLongi]);    // v3
            verts.push_back(vertList[u2 + v * nLongi]);     // v1
            {
                float s0 = float(u) / nLongi; float t0 = 1.0f - float(v) / nLati;
                float s1 = float(u2) / nLongi; float t1 = 1.0f - float(v2) / nLati;
                float s2 = float(u2) / nLongi; float t2 = 1.0f - float(v) / nLati;
                texCoords.push_back(glm::vec2(s0, t0));
                texCoords.push_back(glm::vec2(s1, t1));
                texCoords.push_back(glm::vec2(s2, t2));
            }
        }
    }
}

void Sphere::computeNormals()
{
    normals.resize(verts.size());
    for (size_t i = 0; i < verts.size(); i++)
    {
        glm::vec3 n = glm::normalize(glm::vec3(verts[i]));
        normals[i] = glm::vec4(n, 0.0f);
    }
}
