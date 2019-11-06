/*
 *  Simple vertex shader for lab 4
 */

#version 330 core

uniform mat4 modelView;
uniform mat4 projection;
in vec4 vPosition;
in vec3 vNormal;
out vec3 normal;
out vec4 position;

void main() {

	gl_Position = projection * modelView * vPosition;
	// normal = (modelView * vec4(vNormal,1.0)).xyz;
	normal = vNormal;
	position=vPosition;

}