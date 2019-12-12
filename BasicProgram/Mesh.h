#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

class Mesh {
public:
	GLuint vao;			// vertex object identifier
	GLuint ibuffer;		// number of triangles
	int triangles;		// index buffer identifierSSS
	const char* name;
	glm::vec3 size = glm::vec3(0.0f);

	GLfloat* vertices;
	GLfloat* normals;
	GLuint* indices;
	int nv;
	int nn;
	int ni;

	glm::vec2 top;
	glm::vec2 bottom;

	Mesh(const char* name);
	void createHitbox(float minx, float minz, float maxx, float maxz);
};
