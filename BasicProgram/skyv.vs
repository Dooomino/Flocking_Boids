#version 330 core
uniform mat4 modelView;
uniform mat4 projection;

uniform samplerCube tex;

in vec4 vPosition;

out vec3 tc;

void main(){
	tc = vec3(-vPosition.x, vPosition.y, -vPosition.z);

	gl_Position = projection * modelView * vPosition;
}