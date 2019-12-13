#version 330

uniform samplerCube tex;

in vec3 tc;


void main(){
	gl_FragColor = texture(tex,tc);
}