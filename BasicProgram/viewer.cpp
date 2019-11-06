#define GLM_FORCE_RADIANS

#include <GL/glew.h>
#define GLFW_DLL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>
#include <stdio.h>
#include "Shaders.h"
#include "tiny_obj_loader.h"
#include <iostream>
#include <ctime>
#include <vector>

#define SPHERE

float eyex, eyey, eyez;	// current user position
float scalef=0.2f;
float modelScale = 10.0f;
double theta, phi;		// user's position  on a sphere centered on the object
double r;				// radius of the sphere
float rot = 0;

GLuint program;

glm::mat4 projection;	// projection matrix

GLuint objVAO;			// vertex object identifier
int triangles;			// number of triangles
GLuint ibuffer;			// index buffer identifier

int numBoids = 1000;

std::vector<glm::vec3> listBoid;
std::vector<glm::vec3> rotationAxis;

float normalizedRandom() {
	return 1.0f / (rand()%10);
}

int randomNeg() {
	int a = rand() % 2;
	return a == 0 ? -1 : 1;
}


void initBoidsPos() {
	listBoid.clear();
	for (int i = 0; i < numBoids; i++) {
		float dx = normalizedRandom()*randomNeg();
		float dy = normalizedRandom()*randomNeg();
		float dz = normalizedRandom()*randomNeg();

		float rx = normalizedRandom() * randomNeg();
		float ry = normalizedRandom() * randomNeg();
		float rz = normalizedRandom() * randomNeg();

		glm::vec3 pos(dx, dy, dz);
		glm::vec3 rotate(rx, ry, rz);

		printf("%f %f %f\n", pos.x, pos.y, pos.z);

		listBoid.push_back(pos);
		rotationAxis.push_back(rotate);
	}

}



void init() {
	GLuint vbuffer;
	GLint vPosition;
	GLint vNormal;
	GLfloat *vertices;
	GLfloat *normals;
	GLuint *indices;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	int nv;
	int nn;
	int ni;
	int i;

	glGenVertexArrays(1, &objVAO);
	glBindVertexArray(objVAO);

	/*  Load the obj file */

	std::string err = tinyobj::LoadObj(shapes, materials, "boids.obj", 0);
	//std::string err = tinyobj::LoadObj(shapes, materials, "sphere.obj", 0);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return;
	}

	/*  Retrieve the vertex coordinate data */

	nv = (int)shapes[0].mesh.positions.size();
	vertices = new GLfloat[nv];
	for (i = 0; i<nv; i++) {
		vertices[i] = shapes[0].mesh.positions[i];
	}

	/*  Retrieve the vertex normals */

	nn = (int)shapes[0].mesh.normals.size();
	normals = new GLfloat[nn];
	for (i = 0; i<nn; i++) {
		normals[i] = shapes[0].mesh.normals[i];
	}

	/*  Retrieve the triangle indices */

	ni = (int)shapes[0].mesh.indices.size();
	triangles = ni / 3;
	indices = new GLuint[ni];
	for (i = 0; i<ni; i++) {
		indices[i] = shapes[0].mesh.indices[i];
	}

	/*
	*  load the vertex coordinate data
	*/
	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, (nv + nn)*sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, nv*sizeof(GLfloat), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, nv*sizeof(GLfloat), nn*sizeof(GLfloat), normals);

	/*
	*  load the vertex indexes
	*/
	glGenBuffers(1, &ibuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni*sizeof(GLuint), indices, GL_STATIC_DRAW);

	/*
	*  link the vertex coordinates to the vPosition
	*  variable in the vertex program.  Do the same
	*  for the normal vectors.
	*/
	glUseProgram(program);
	vPosition = glGetAttribLocation(program, "vPosition");
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);
	vNormal = glGetAttribLocation(program, "vNormal");
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*)((nv/2)*sizeof(vertices)));
	glEnableVertexAttribArray(vNormal);

}

double lastsY = 0;
double speed = 0.01f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	float distY = yoffset;

	scalef += speed * distY * 10;

	lastsY = yoffset;
}


double lastX = 0, lastY = 0;
double mouseX = 0, mouseY = 0;

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos;
	mouseY = ypos;

	float distX = abs(lastX - mouseX);
	float distY = abs(lastY - mouseY);

	if (lastX < mouseX) {
		phi -= speed * distX;
	}
	else {
		phi += speed * distX;
	}

	if (lastY < mouseY) {
		theta += speed * distY;
	}
	else {
		theta -= speed * distY;
	}

	lastX = mouseX;
	lastY = mouseY;

	//printf("eye: %f %f %f\n", eyex, eyey, eyez);
	//printf("theta: %f phi: %f\n", theta,phi);
}


static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && (action == GLFW_REPEAT)) {
		if (lastX <= mouseX) {
			phi -= speed;
		}
		else {
			phi += speed;
		}

		if (lastY <= mouseY) {
			theta -= speed;
		}
		else {
			theta += speed;
		}
	}


}
float speedk = 0.1f;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		printf("Key: %s\n", glfwGetKeyName(key, 0));
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);

		if (key == GLFW_KEY_R)
			initBoidsPos();
	}

}

void framebufferSizeCallback(GLFWwindow *window, int w, int h) {

	// Prevent a divide by zero, when window is too short

	if (h == 0)
		h = 1;

	float ratio = 1.0f * w / h;

	glfwMakeContextCurrent(window);

	glViewport(0, 0, w, h);

	projection = glm::perspective(0.7f, ratio, 1.0f, 100.0f);

}

void display(void) {
	glm::mat4 view;
	int viewLoc;
	int projLoc;
	int eyeLoc;

	eyex = (float)(r * sin(theta) * cos(phi));
	eyey = (float)(r * sin(theta) * sin(phi));
	eyez = (float)(r * cos(theta));

	glm::vec3 eyepos(eyex, eyey, eyez);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	view = glm::lookAt(glm::vec3(eyex, eyey, eyez),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	viewLoc = glGetUniformLocation(program, "modelView");
	projLoc = glGetUniformLocation(program, "projection");
	eyeLoc = glGetUniformLocation(program, "Eye");



	for (int i = 0; i < numBoids; i++) {
		glm::mat4 scale(1.0f);
		glm::mat4 transform(1.0f);

		view = glm::lookAt(glm::vec3(eyex, eyey, eyez),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));

		scale = glm::scale(scale, glm::vec3(scalef));

		view *= scale;

		transform = glm::rotate(transform, rot, rotationAxis[i]);
		transform = glm::translate(transform, modelScale * listBoid[i]);
		transform = glm::rotate(transform, rot, rotationAxis[i]);

		//transform = glm::translate(transform, -1.0f*listBoid[i]);

		view *= transform;

		glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
		glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));

		glBindVertexArray(objVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
		glDrawElements(GL_TRIANGLES, 3*triangles, GL_UNSIGNED_INT, NULL);
	}
	rot += 0.01;
	if (rot > 360.0f) {
		rot = 0;
	}
}

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char **argv) {
	int fs;
	int vs;
	GLFWwindow *window;

	srand(time(NULL));
	// start by setting error callback in case something goes wrong

	glfwSetErrorCallback(error_callback);

	// initialize glfw

	if (!glfwInit()) {
		fprintf(stderr, "can't initialize GLFW\n");
	}

	// create the window used by our application

	window = glfwCreateWindow(512, 512, "GLFW", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// establish framebuffer size change and input callbacks

	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	glfwSetKeyCallback(window, key_callback);

	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glfwSetCursorPosCallback(window, cursor_position_callback);

	glfwSetScrollCallback(window, scroll_callback);
	// now initialize glew our extension handler

	glfwMakeContextCurrent(window);

	GLenum error = glewInit();
	if (error != GLEW_OK) {
		printf("Error starting GLEW: %s\n", glewGetErrorString(error));
		exit(0);
	}
	//printf("%f\n", 1.0f/(rand()%10));
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glViewport(0, 0, 512, 512);

	projection = glm::perspective(0.7f, 1.0f, 1.0f, 100.0f);

	vs = buildShader(GL_VERTEX_SHADER, (char*)"vert.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"frag.fs");
	program = buildProgram(vs, fs, 0);
	dumpProgram(program, (char*)"shader program");
	init();

	eyex = 0.0;
	eyez = 0.0;
	eyey = 10.0;

	theta = 1.5;
	phi = 1.5;
	r = 10.0;

	glfwSwapInterval(1);


	//create init position
	initBoidsPos();

	// GLFW main loop, display model, swapbuffer and check for input

	while (!glfwWindowShouldClose(window)) {
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

}
