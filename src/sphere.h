#pragma once

//#include <cmath>
#include <iostream>
#include <vector>
#include "glm/glm.hpp"
//#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/constants.hpp"

using namespace std;

const float PI = glm::pi<float>();

extern void makeSphere(vector<glm::vec4> *verts, int nLongi, int nLati);

class Sphere {
public:
	vector<glm::vec4> verts;
	vector<glm::vec4> normals;

	Sphere() :Sphere(20, 20) {  }
	Sphere(int n) :Sphere(n, n) {  }
	Sphere(int nLongi, int nLati) { 
		makeUV(nLongi, nLati);
		computeNormals();
	};
	~Sphere() {
		verts.clear();
		vector<glm::vec4>().swap(verts);
		normals.clear();
		vector<glm::vec4>().swap(normals);
	}
private:
	const float PI = glm::pi<float>();
	void makeUV(int nLongi, int nLati);
	void computeNormals();
} ;


