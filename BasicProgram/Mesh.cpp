#include "Mesh.h"

Mesh::Mesh(const char* name) {
	this->name = name;
}

void Mesh::createHitbox(float minx, float minz, float maxx, float maxz) {
	this->top = glm::vec2(maxx, maxz);
	this->bottom = glm::vec2(minx, minz);
}

