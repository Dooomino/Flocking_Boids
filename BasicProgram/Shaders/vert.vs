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

uniform vec3 Eye;

float bc(float t){
	float p1 = 1.2;
	float p2 = 0.3;
	float p3 = 1.1;
	return pow(1-t,2) * p1 + 2 * (1-t) * p2 + pow(t,2) * p3;
} 

void main() 
{
	vec4 pos = vPosition;
	
	vec3 e = normalize(Eye);

	//pos = vec4(pos.x*bc(e.x),pos.y*bc(e.y),pos.z*bc(e.z),1.0);

	gl_Position = projection * modelView * pos;
	// normal = (modelView * vec4(vNormal,1.0)).xyz;
	normal = vNormal;
	position=vPosition;

}