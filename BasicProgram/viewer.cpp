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
#include "viewer.h"
#include <FreeImage/FreeImage.h>
#include "texture.h"


float eyex, eyey, eyez;	// current user position
float scalef=0.1f;
float modelScale = 10.0f;
double theta, phi;		// user's position  on a sphere centered on the object
double r;				// radius of the sphere
float rot = 0;
float rotspeed = 0.001;


glm::mat4 projection;	// projection matrix

//Boids
GLuint program;
GLuint objVAO;			// vertex object identifier
int triangles;			// number of triangles
GLuint ibuffer;			// index buffer identifier

//sphere
GLuint sunprogram;
GLuint sphereVAO;		// vertex object identifier
int striangles;			// number of triangles
GLuint sibuffer;		// index buffer identifier

//Skybox
Cube* textureCube; // texture container
GLuint SkyTextureBuffer;  // sky texture buffer
GLuint skyprogram;  // shader program
GLuint skyVAO;		// vertex object identifier
int skytriangles;			// number of triangles
GLuint skyibuffer;		// index buffer identifier

int numBoids = 1000;
int centerSize = 4;

int width = 512, height = 512;

std::vector<glm::vec3> listBoid;
std::vector<glm::vec3> rotationAxis;
std::vector<GLfloat> rotationOffset;

float normalizedRandom() {
	int a = rand();
	return 1.0f / ((a % 10)+1);
}

float sizedRandom(int size) {
	int a = rand()%(size+1);
	return  a*1.0f;
}

int randomNeg() {
	int a = rand() % 2;
	return a == 0 ? -1 : 1;
}

float is0or1() {
	int a = rand() % 2;
	return a == 0 ? 0 : 1.0f;
}

void initBoidsPos() {
	listBoid.clear();
	for (int i = 0; i < numBoids; i++) {
		//float dy = centerSize * normalizedRandom() * randomNeg();
		//float dz = centerSize * normalizedRandom() * randomNeg();
		//glm::vec3 pos(dx, dy, dz);
		//glm::vec3 rotate(rx, ry, rz);

		float dx = centerSize * normalizedRandom() * randomNeg();
		float rx = is0or1();
		float ry = is0or1();
		float rz = is0or1();

		glm::vec3 pos(dx,dx,dx);
		glm::vec3 rotate(rx,0.0f, rz);
		
		if(numBoids<100)
			printf("%f %f %f\n", pos.x, pos.y, pos.z);

		rotationOffset.push_back(sizedRandom(360));
		listBoid.push_back(pos);
		rotationAxis.push_back(rotate);
	}

}

void initSky() {
	GLuint vbuffer;
	GLint vPosition;
	GLint vNormal;

	glGenVertexArrays(1, &skyVAO);
	glBindVertexArray(skyVAO);

	GLfloat vertices[8][4] = {
			{ -1.0, -1.0, -1.0, 1.0 },		//0
			{ -1.0, -1.0, 1.0, 1.0 },		//1
			{ -1.0, 1.0, -1.0, 1.0 },		//2
			{ -1.0, 1.0, 1.0, 1.0 },		//3
			{ 1.0, -1.0, -1.0, 1.0 },		//4
			{ 1.0, -1.0, 1.0, 1.0 },		//5
			{ 1.0, 1.0, -1.0, 1.0 },		//6
			{ 1.0, 1.0, 1.0, 1.0 }			//7
	};

	GLfloat normals[8][3] = {
			{ -1.0, -1.0, -1.0 },			//0
			{ -1.0, -1.0, 1.0 },			//1
			{ -1.0, 1.0, -1.0 },			//2
			{ -1.0, 1.0, 1.0 },				//3
			{ 1.0, -1.0, -1.0 },			//4
			{ 1.0, -1.0, 1.0 },				//5
			{ 1.0, 1.0, -1.0 },				//6
			{ 1.0, 1.0, 1.0 }				//7
	};

	GLuint indexes[36] = { 0, 1, 3, 0, 2, 3,
		0, 4, 5, 0, 1, 5,
		2, 6, 7, 2, 3, 7,
		0, 4, 6, 0, 2, 6,
		1, 5, 7, 1, 3, 7,
		4, 5, 7, 4, 6, 7 };

	skytriangles = 12;

	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(normals), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(normals), normals);

	glGenBuffers(1, &skyibuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);

	glUseProgram(skyprogram);
	vPosition = glGetAttribLocation(skyprogram, "vPosition");
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);
	vNormal = glGetAttribLocation(skyprogram, "vNormal");
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*) sizeof(vertices));
	glEnableVertexAttribArray(vNormal);
}

void createCubemap(const char* url, GLuint& Buffer) {
	/*
		load texture
	*/
	textureCube = loadCube(url);
	glGenTextures(1, &Buffer);
	glBindTexture(GL_TEXTURE_CUBE_MAP, Buffer);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, textureCube->width, textureCube->height,
			0, GL_RGB, GL_UNSIGNED_BYTE, textureCube->data[i]);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


}

void initSphere() {
	GLuint vbuffer;
	GLint vPosition;
	GLint vNormal;
	GLfloat* vertices;
	GLfloat* normals;
	GLuint* indices;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	int nv;
	int nn;
	int ni;
	int i;

	glGenVertexArrays(1, &sphereVAO);
	glBindVertexArray(sphereVAO);

	/*  Load the obj file */

	std::string err = tinyobj::LoadObj(shapes, materials, "sphere.obj", 0);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return;
	}

	/*  Retrieve the vertex coordinate data */

	nv = (int)shapes[0].mesh.positions.size();
	vertices = new GLfloat[nv];
	for (i = 0; i < nv; i++) {
		vertices[i] = shapes[0].mesh.positions[i] / 2.0f;
	}

	/*  Retrieve the vertex normals */

	nn = (int)shapes[0].mesh.normals.size();
	normals = new GLfloat[nn];
	for (i = 0; i < nn; i++) {
		normals[i] = shapes[0].mesh.normals[i];
	}

	/*  Retrieve the triangle indices */

	ni = (int)shapes[0].mesh.indices.size();
	striangles = ni / 3;
	indices = new GLuint[ni];
	for (i = 0; i < ni; i++) {
		indices[i] = shapes[0].mesh.indices[i];
	}

	/*
	*  load the vertex coordinate data
	*/
	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, (nv + nn) * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, nv * sizeof(GLfloat), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, nv * sizeof(GLfloat), nn * sizeof(GLfloat), normals);

	/*
	*  load the vertex indexes
	*/
	glGenBuffers(1, &sibuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(GLuint), indices, GL_STATIC_DRAW);

	/*
	*  link the vertex coordinates to the vPosition
	*  variable in the vertex program.  Do the same
	*  for the normal vectors.
	*/
	glUseProgram(sunprogram);
	vPosition = glGetAttribLocation(sunprogram, "vPosition");
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vPosition);
	vNormal = glGetAttribLocation(sunprogram, "vNormal");
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*)((nv/2) * sizeof(vertices)));
	glEnableVertexAttribArray(vNormal);

}

void initBoids() {
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
		//printf("Key: %s\n", glfwGetKeyName(key, 0));
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GLFW_TRUE);

		if (action == GLFW_REPEAT) {
			if (key == GLFW_KEY_EQUAL){
				rotspeed += 0.0001;
				printf("set rotaion speed %f\n", rotspeed);
			}
			else if (key == GLFW_KEY_MINUS){
				rotspeed -= 0.0001;
				printf("set rotaion speed %f\n",rotspeed);
			}
		}else if (action == GLFW_PRESS){
			if (key == GLFW_KEY_R)
				initBoidsPos();
			if (key == GLFW_KEY_T)
				createCubemap("D:\\Desktop\\Flocking_Boids\\BasicProgram\\Cubemap", SkyTextureBuffer);
			if (key == GLFW_KEY_EQUAL){
				rotspeed += 0.001;
				printf("set rotaion speed %f\n",rotspeed);
			}else if (key == GLFW_KEY_MINUS){
				rotspeed -= 0.001;
				printf("set rotaion speed %f\n",rotspeed);
			}		
		}
	}
}

void framebufferSizeCallback(GLFWwindow *window, int w, int h) {

	// Prevent a divide by zero, when window is too short

	if (h == 0)
		h = 1;

	float ratio = 1.0f * w / h;

	glfwMakeContextCurrent(window);

	width = w;
	height = h;

	glViewport(0, 0, w, h);

	projection = glm::perspective(0.7f, ratio, 1.0f, 1000.0f);

}

void display(void) {
	glm::mat4 view;
	int viewLoc;
	int projLoc;
	int eyeLoc;
	//pre-defines
	eyex = (float)(r * sin(theta) * cos(phi));
	eyey = (float)(r * sin(theta) * sin(phi));
	eyez = (float)(r * cos(theta));

	glm::vec3 eyepos(eyex, eyey, eyez);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//sky 
	glEnable(GL_DEPTH_TEST);
	glUseProgram(skyprogram);

	glm::mat4 viewsky(1.0f);
	viewsky = glm::lookAt(
		eyepos,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	glm::mat4 skySize = glm::scale(glm::mat4(1.0f), glm::vec3(100 * width / height));

	viewLoc = glGetUniformLocation(skyprogram, "modelView");
	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(viewsky * skySize));
	projLoc = glGetUniformLocation(skyprogram, "projection");
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, SkyTextureBuffer);
	GLuint skyboxTextureId = glGetUniformLocation(skyprogram, "tex");
	glUniform1i(skyboxTextureId, 0);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glFrontFace(GL_CCW);
	glDisable(GL_CULL_FACE);


	glBindVertexArray(skyVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyibuffer);
	//glDrawElements(GL_TRIANGLES, 3 * skytriangles, GL_UNSIGNED_INT, NULL);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	//sun 
	glUseProgram(sunprogram);
	view = glm::lookAt(eyepos,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	viewLoc = glGetUniformLocation(sunprogram, "modelView");
	projLoc = glGetUniformLocation(sunprogram, "projection");
	eyeLoc = glGetUniformLocation(sunprogram, "Eye");

	view = glm::scale(view, glm::vec3(glm::max(scalef * centerSize,0.0f)));
	glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
	glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));

	glBindVertexArray(sphereVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sibuffer);
	glDrawElements(GL_TRIANGLES, 3 * striangles, GL_UNSIGNED_INT, NULL);
	
	//Boids
	glUseProgram(program);

	viewLoc = glGetUniformLocation(program, "modelView");
	projLoc = glGetUniformLocation(program, "projection");
	eyeLoc = glGetUniformLocation(program, "Eye");

	for (int i = 0; i < numBoids; i++) {
		glm::mat4 scale(1.0f);
		glm::mat4 transform(1.0f);

		view = glm::lookAt(eyepos,
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f));

		scale = glm::scale(scale, glm::vec3(glm::max(scalef,0.0f)));

		view *= scale;

		transform = glm::rotate(transform, rot * rotationOffset[i], rotationAxis[i]);
		transform = glm::translate(transform, modelScale * listBoid[i]);
		//transform = glm::translate(transform, modelScale * glm::vec3(0.0,1.0,0.0));
		transform = glm::rotate(transform,90.0f, glm::vec3(0.0,0.0,-1.0));

		view *= transform;

		glUniformMatrix4fv(viewLoc, 1, 0, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, 0, glm::value_ptr(projection));
		glUniform3fv(eyeLoc, 1, glm::value_ptr(eyepos));

		glBindVertexArray(objVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
		glDrawElements(GL_TRIANGLES, 3*triangles, GL_UNSIGNED_INT, NULL);
	}

	

	//updates
	rot += rotspeed;
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

	if (!window){
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

	projection = glm::perspective(0.7f, 1.0f, 1.0f, 1000.0f);

	vs = buildShader(GL_VERTEX_SHADER, (char*)"vert.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"frag.fs");
	program = buildProgram(vs, fs, 0);
	dumpProgram(program, (char*)"shader program");


	vs = buildShader(GL_VERTEX_SHADER, (char*)"Svert.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"Sfrag.fs");
	sunprogram = buildProgram(vs, fs, 0);
	dumpProgram(sunprogram, (char*)"sun program");

	vs = buildShader(GL_VERTEX_SHADER, (char*)"skyv.vs");
	fs = buildShader(GL_FRAGMENT_SHADER, (char*)"skyf.fs");
	skyprogram = buildProgram(vs, fs, 0);
	dumpProgram(skyprogram, (char*)"sky program");

	initBoids();
	initSphere();
	initSky();

	eyex = 0.0;
	eyez = 0.0;
	eyey = 10.0;

	theta = 1.5;
	phi = 1.5;
	r = 10.0;

	glfwSwapInterval(1);


	//create init position
	initBoidsPos();

	//create texture
	createCubemap("D:\\Desktop\\Flocking_Boids\\BasicProgram\\Cubemap",SkyTextureBuffer);

	// GLFW main loop, display model, swapbuffer and check for input

	while (!glfwWindowShouldClose(window)) {
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

}
