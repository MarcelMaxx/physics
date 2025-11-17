// Display a swimming cubeman

#include "cube.h"
#include "sphere.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include <cmath>

glm::mat4 projectMat;
glm::mat4 viewMat;

GLuint programID;
GLuint vaoCube, vaoSphere;
GLuint bufferCube, bufferSphere;

GLuint projectMatrixID;
GLuint viewMatrixID;
GLuint modelMatrixID;
GLuint lightPosID;
GLuint viewPosID;
GLuint lightAmbientID, lightDiffuseID, lightSpecularID;
GLuint materialAmbientID, materialDiffuseID, materialSpecularID;
GLuint materialShininessID;

Sphere g_sphere(40, 40);
int g_sphereVertCount = 0;

typedef glm::vec4  color4;
typedef glm::vec4  point4;


const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)

point4 points[NumVertices];
color4 colors[NumVertices];
point4 normals[NumVertices];

// Vertices of a unit cube centered at origin
point4 vertices[8] = {
	point4(-0.5, -0.5,  0.5, 1.0),
	point4(-0.5,  0.5,  0.5, 1.0),
	point4(0.5,  0.5,  0.5, 1.0),
	point4(0.5, -0.5,  0.5, 1.0),
	point4(-0.5, -0.5, -0.5, 1.0),
	point4(-0.5,  0.5, -0.5, 1.0),
	point4(0.5,  0.5, -0.5, 1.0),
	point4(0.5, -0.5, -0.5, 1.0)
};

// RGBA colors
color4 vertex_colors[8] = {
	color4(0.0, 0.0, 0.0, 1.0),  // black
	color4(0.0, 1.0, 1.0, 1.0),  // cyan
	color4(1.0, 0.0, 1.0, 1.0),  // magenta
	color4(1.0, 1.0, 0.0, 1.0),  // yellow
	color4(1.0, 0.0, 0.0, 1.0),  // red
	color4(0.0, 1.0, 0.0, 1.0),  // green
	color4(0.0, 0.0, 1.0, 1.0),  // blue
	color4(1.0, 1.0, 1.0, 1.0)   // white
};

int Index = 0;
static inline void quad(int a, int b, int c, int d)
{
	glm::vec3 u = glm::vec3(vertices[b] - vertices[a]);
	glm::vec3 v = glm::vec3(vertices[c] - vertices[b]);
	glm::vec3 n = glm::normalize(glm::cross(u, v));
	point4 normal(n.x, n.y, n.z, 0.0f);

	colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; normals[Index] = normal; Index++;
	colors[Index] = vertex_colors[b]; points[Index] = vertices[b]; normals[Index] = normal; Index++;
	colors[Index] = vertex_colors[c]; points[Index] = vertices[c]; normals[Index] = normal; Index++;
	colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; normals[Index] = normal; Index++;
	colors[Index] = vertex_colors[c]; points[Index] = vertices[c]; normals[Index] = normal; Index++;
	colors[Index] = vertex_colors[d]; points[Index] = vertices[d]; normals[Index] = normal; Index++;
}

static inline void colorcube()
{
	quad(1, 0, 3, 2);
	quad(2, 3, 7, 6);
	quad(3, 0, 4, 7);
	quad(6, 5, 1, 2);
	quad(4, 5, 6, 7);
	quad(5, 4, 0, 1);
}

// ---------- Animation/keyframe state ----------
static int g_camMode = 1;                 // 1: side, 2: OTS, 3: front
static float g_cycleSec = 2.0f;           // one swim cycle duration (seconds)
static int g_prevMS = 0;
static double g_timeSec = 0.0;            // accumulated time (seconds)

// 6 keyframes (+1 wrap row). Degrees.
// This guarantees monotonic increase (no "shortest-arc" reversal).
static const int K = 6; // segments
static const float k_shoulderR[K + 1] = { 360, 315, 270, 225, 180, 90, 0 };
static const float k_shoulderL[K + 1] = { 225, 135, 45, 0, -45, -90, -135 };

// Elbow flex (freestyle style). Rough, but plausible.
// R-frame aligned with k_shoulderR; L-frame aligned with k_shoulderL (phase-shifted).
static const float k_elbowR[K + 1] = { 45, 180, 270, 345, 420, 360, 405 };
static const float k_elbowL[K + 1] = { 30, 0, 15, 45, 180, 270, 390 };
// Flutter kick around hips; knees follow with slightly different phase/amplitude.
static const float k_hipR[K + 1] = { 5, 15, 30, 5, 15, 30, 5 };
static const float k_hipL[K + 1] = { 30, 20, 5, 30, 20, 5, 30 };
static const float k_kneeR[K + 1] = { -20, 0, 0, -20, -0, 0, -20 };
static const float k_kneeL[K + 1] = { 0, 0, -20, 0, 0, -20, 0 };
// Torso bobbing (small). Up axis = +Y.
static const float k_torsoBobY[K + 1] = { 0.04, 0.02, 0.00, -0.02, -0.01, 0.02, 0.04 };

// Linear interp helper for keyframes in [0,1) -> [idx, idx+1]
static inline float kfLerp(const float a[], int kCountPlus1, float t01)
{
	float seg = t01 * K;
	int i = (int)glm::clamp(floor(seg), 0.0f, float(K - 1));
	float u = seg - float(i);
	return a[i] * (1.0f - u) + a[i + 1] * u;
}

// ---------- Drawing helpers ----------
static inline void drawUnit(const glm::mat4& model)
{
	glBindVertexArray(vaoCube);

	glm::mat4 modelMat = model;
	glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMat[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
}

static inline void drawSphereUnit(const glm::mat4& model)
{
	glBindVertexArray(vaoSphere);

	glm::mat4 modelMat = model;
	glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMat[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, g_sphereVertCount);
}

static inline glm::mat4 rotX_deg(const glm::mat4& M, float deg)
{
	return glm::rotate(M, glm::radians(deg), glm::vec3(1, 0, 0));
}

// ---------- Man (hierarchical model) ----------
// All dimensions are in "unit cube" scale space.
void drawMan(double timeSec)
{
	// Cycle t in [0,1)
	float tCycle = fmod(float(timeSec / g_cycleSec), 1.0f);

	// Interpolated attributes (degrees)
	float shoulderR = kfLerp(k_shoulderR, K + 1, tCycle);
	float shoulderL = kfLerp(k_shoulderL, K + 1, tCycle);
	float elbowR = kfLerp(k_elbowR, K + 1, tCycle);
	float elbowL = kfLerp(k_elbowL, K + 1, tCycle);
	float hipR = kfLerp(k_hipR, K + 1, tCycle);
	float hipL = kfLerp(k_hipL, K + 1, tCycle);
	float kneeR = kfLerp(k_kneeR, K + 1, tCycle);
	float kneeL = kfLerp(k_kneeL, K + 1, tCycle);
	float bobY = kfLerp(k_torsoBobY, K + 1, tCycle);

	// Base/prone: rotate torso -90¡ã about X so the man "lies facing down"
	glm::mat4 base = glm::translate(glm::mat4(1.0f), glm::vec3(0, bobY, 0));
	base = rotX_deg(base, -90.0f);

	// Sizes
	const glm::vec3 torsoS(0.7f, 1.0f, 0.3f);
	const glm::vec3 headS(0.3f, 0.3f, 0.3f);
	const glm::vec3 uArmS(0.2f, 0.5f, 0.2f);
	const glm::vec3 fArmS(0.2f, 0.6f, 0.2f);
	const glm::vec3 uLegS(0.2f, 0.8f, 0.2f);
	const glm::vec3 lLegS(0.2f, 0.8f, 0.2f);

	// Torso (centered)
	glm::mat4 torso = glm::scale(base, torsoS);
	drawUnit(torso);

	// Head (above torso along +Y in torso space)
	glm::mat4 head = base;
	head = glm::translate(head, glm::vec3(0, torsoS.y * 0.5f + headS.y * 0.5f, 0));
	head = glm::scale(head, headS);
	drawSphereUnit(head);

	// Shoulder/hip anchors in torso space
	const float shoulderY = torsoS.y * 0.55f;
	const float shoulderX = torsoS.x * 0.67f;
	const float hipY = -torsoS.y * 0.55f;
	const float hipX = torsoS.x * 0.33f;

	// ----- Right arm -----
	{   // Upper arm pivot at shoulder; rotate around X; then drop half length
		glm::mat4 A = base;
		A = glm::translate(A, glm::vec3(+shoulderX, shoulderY, 0));
		A = rotX_deg(A, shoulderR);  // continuous monotonic rotation
		A = glm::translate(A, glm::vec3(0, -uArmS.y * 0.55f, 0));
		A = glm::scale(A, uArmS);
		drawUnit(A);

		// Forearm: from upper-arm end; elbow flex around X; drop half length
		glm::mat4 F = base;
		F = glm::translate(F, glm::vec3(+shoulderX, shoulderY, 0));
		F = rotX_deg(F, shoulderR);
		F = glm::translate(F, glm::vec3(0, -uArmS.y, 0));
		F = rotX_deg(F, elbowR);
		F = glm::translate(F, glm::vec3(0, -fArmS.y * 0.55f, 0));
		F = glm::scale(F, fArmS);
		drawUnit(F);
	}
	// ----- Left arm -----
	{
		glm::mat4 A = base;
		A = glm::translate(A, glm::vec3(-shoulderX, shoulderY, 0));
		A = rotX_deg(A, shoulderL);
		A = glm::translate(A, glm::vec3(0, -uArmS.y * 0.55f, 0));
		A = glm::scale(A, uArmS);
		drawUnit(A);

		glm::mat4 F = base;
		F = glm::translate(F, glm::vec3(-shoulderX, shoulderY, 0));
		F = rotX_deg(F, shoulderL);
		F = glm::translate(F, glm::vec3(0, -uArmS.y, 0));
		F = rotX_deg(F, elbowL);
		F = glm::translate(F, glm::vec3(0, -fArmS.y * 0.55f, 0));
		F = glm::scale(F, fArmS);
		drawUnit(F);
	}

	// ----- Right leg -----
	{
		glm::mat4 U = base;
		U = glm::translate(U, glm::vec3(+hipX, hipY, 0));
		U = rotX_deg(U, hipR);
		U = glm::translate(U, glm::vec3(0, -uLegS.y * 0.5f, 0));
		U = glm::scale(U, uLegS);
		drawUnit(U);

		glm::mat4 L = base;
		L = glm::translate(L, glm::vec3(+hipX, hipY, 0));
		L = rotX_deg(L, hipR);
		L = glm::translate(L, glm::vec3(0, -uLegS.y, 0));
		L = rotX_deg(L, kneeR);
		L = glm::translate(L, glm::vec3(0, -lLegS.y * 0.55f, 0));
		L = glm::scale(L, lLegS);
		drawUnit(L);
	}
	// ----- Left leg -----
	{
		glm::mat4 U = base;
		U = glm::translate(U, glm::vec3(-hipX, hipY, 0));
		U = rotX_deg(U, hipL);
		U = glm::translate(U, glm::vec3(0, -uLegS.y * 0.5f, 0));
		U = glm::scale(U, uLegS);
		drawUnit(U);

		glm::mat4 L = base;
		L = glm::translate(L, glm::vec3(-hipX, hipY, 0));
		L = rotX_deg(L, hipL);
		L = glm::translate(L, glm::vec3(0, -uLegS.y, 0));
		L = rotX_deg(L, kneeL);
		L = glm::translate(L, glm::vec3(0, -lLegS.y * 0.56f, 0));
		L = glm::scale(L, lLegS);
		drawUnit(L);
	}
}

// ---------- OpenGL init ----------
void init()
{
	colorcube();

	// ----- build shader program -----
	programID = InitShader("src/vshader.glsl", "src/fshader.glsl");
	glUseProgram(programID);

	// attribute locations
	GLuint vPosition = glGetAttribLocation(programID, "vPosition");
	GLuint vNormal = glGetAttribLocation(programID, "vNormal");
	GLuint vColor = glGetAttribLocation(programID, "vColor");

	// ----- cube VAO -----
	glGenVertexArrays(1, &vaoCube);
	glBindVertexArray(vaoCube);

	glGenBuffers(1, &bufferCube);
	glBindBuffer(GL_ARRAY_BUFFER, bufferCube);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(points) + sizeof(normals) + sizeof(colors),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(normals), normals);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals),
		sizeof(colors), colors);

	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(points)));

	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(points) + sizeof(normals)));

	// ----- sphere VAO -----
	g_sphereVertCount = static_cast<int>(g_sphere.verts.size());

	glGenVertexArrays(1, &vaoSphere);
	glBindVertexArray(vaoSphere);

	glGenBuffers(1, &bufferSphere);
	glBindBuffer(GL_ARRAY_BUFFER, bufferSphere);

	int vertSize = sizeof(g_sphere.verts[0]) * g_sphere.verts.size();
	int normalSize = sizeof(g_sphere.normals[0]) * g_sphere.normals.size();
	std::vector<glm::vec4> sphereColors(
		g_sphere.verts.size(), glm::vec4(1.0f, 0.8f, 0.6f, 1.0f)); // ·ôÉ«
	int colorSize = sizeof(sphereColors[0]) * sphereColors.size();

	glBufferData(GL_ARRAY_BUFFER,
		vertSize + normalSize + colorSize,
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertSize, g_sphere.verts.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertSize, normalSize, g_sphere.normals.data());
	glBufferSubData(GL_ARRAY_BUFFER, vertSize + normalSize,
		colorSize, sphereColors.data());

	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(vertSize));

	glEnableVertexAttribArray(vColor);
	glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(vertSize + normalSize));

	// ----- uniforms -----
	projectMatrixID = glGetUniformLocation(programID, "mProject");
	viewMatrixID = glGetUniformLocation(programID, "mView");
	modelMatrixID = glGetUniformLocation(programID, "mModel");
	lightPosID = glGetUniformLocation(programID, "lightPos");
	viewPosID = glGetUniformLocation(programID, "viewPos");
	lightAmbientID = glGetUniformLocation(programID, "lightAmbient");
	lightDiffuseID = glGetUniformLocation(programID, "lightDiffuse");
	lightSpecularID = glGetUniformLocation(programID, "lightSpecular");
	materialAmbientID = glGetUniformLocation(programID, "materialAmbient");
	materialDiffuseID = glGetUniformLocation(programID, "materialDiffuse");
	materialSpecularID = glGetUniformLocation(programID, "materialSpecular");
	materialShininessID =
		glGetUniformLocation(programID, "materialShininess");

	// projection matrix
	projectMat = glm::perspective(glm::radians(65.0f),
		1.0f, 0.1f, 100.0f);
	glUniformMatrix4fv(projectMatrixID, 1, GL_FALSE, &projectMat[0][0]);

	// default view (applyCamera Ã¿Ö¡»á¸²¸Ç)
	viewMat = glm::lookAt(glm::vec3(3.0f, 0.6f, 1.2f),
		glm::vec3(0, 0, 0),
		glm::vec3(0, 1, 0));
	glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &viewMat[0][0]);

	// lighting setup
	glm::vec3 lightPos(2.0f, 3.0f, 2.0f);
	glUniform3fv(lightPosID, 1, &lightPos[0]);

	glm::vec3 La(0.2f, 0.2f, 0.2f);
	glm::vec3 Ld(1.0f, 1.0f, 1.0f);
	glm::vec3 Ls(1.0f, 1.0f, 1.0f);
	glUniform3fv(lightAmbientID, 1, &La[0]);
	glUniform3fv(lightDiffuseID, 1, &Ld[0]);
	glUniform3fv(lightSpecularID, 1, &Ls[0]);

	glm::vec3 Ma(0.2f, 0.2f, 0.2f);
	glm::vec3 Md(0.8f, 0.8f, 0.8f);
	glm::vec3 Ms(0.8f, 0.8f, 0.8f);
	glUniform3fv(materialAmbientID, 1, &Ma[0]);
	glUniform3fv(materialDiffuseID, 1, &Md[0]);
	glUniform3fv(materialSpecularID, 1, &Ms[0]);
	glUniform1f(materialShininessID, 32.0f);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);

	g_prevMS = glutGet(GLUT_ELAPSED_TIME);
}

// ---------- Camera control ----------
static inline void applyCamera()
{
    glm::vec3 eye;
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    if (g_camMode == 1) {
        // Side view
        eye = glm::vec3(5.0f, 0.0f, 0.0f);
    }
    else if (g_camMode == 2) {
        // Over-the-shoulder: slightly above & right behind
        eye = glm::vec3(0.5f, 1.5f, 3.0f);
        center = glm::vec3(0.0f, 0.1f, 0.0f);
    }
    else {
        // Front view
        eye = glm::vec3(0.0f, 0.5f, -3.2f);
    }

    viewMat = glm::lookAt(eye, center, up);
    glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &viewMat[0][0]);
    glUniform3fv(viewPosID, 1, &eye[0]);
}

// ---------- Display ----------
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	applyCamera();
	drawMan(g_timeSec);
	glutSwapBuffers();
}

// ---------- Idle (time-based) ----------
void idle()
{
	int curr = glutGet(GLUT_ELAPSED_TIME);
	int dtms = curr - g_prevMS;
	if (dtms <= 0) return;
	g_prevMS = curr;
	g_timeSec += dtms * 0.001; // seconds
	glutPostRedisplay();
}

// ---------- Keyboard ----------
void keyboard(unsigned char key, int, int)
{
	switch (key) {
	case '1': g_camMode = 1; glutPostRedisplay(); break;
	case '2': g_camMode = 2; glutPostRedisplay(); break;
	case '3': g_camMode = 3; glutPostRedisplay(); break;
	case 033: // ESC
	case 'q': case 'Q':
		exit(EXIT_SUCCESS);
		break;
	default: break;
	}
}

void resize(int w, int h)
{
	float ratio = (h > 0) ? (float)w / (float)h : 1.0f;
	glViewport(0, 0, w, h);
	projectMat = glm::perspective(glm::radians(65.0f), ratio, 0.1f, 100.0f);
	glUseProgram(programID);
	glUniformMatrix4fv(projectMatrixID, 1, GL_FALSE, &projectMat[0][0]);
	glutPostRedisplay();
}

// ---------- Main ----------
int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutInitContextVersion(3, 2);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow("Cubeman Swim");

	glewInit();
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(resize);
	glutIdleFunc(idle);

	glutMainLoop();
	return 0;
}
