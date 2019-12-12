#version 330 core

in vec3 normal;
in vec4 position;

uniform vec3 Eye;
uniform int marked;

void main() {
	vec3 N = normalize(normal);

	float ams = 0.2;
	float sps = 0.6;

	vec4 base = vec4(1.0, 1.0, 1.0, 1.0);
	vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);

	vec3 lightpos = Eye;

	vec4 amb = ams*lightColor;

    vec3 L = normalize(lightpos-position.xyz);
	float diff = max(dot(N,L),0.0);
	vec4 diffuse = diff*lightColor;
	

	vec3 V = normalize(Eye - position.xyz);
	vec3 H = reflect(-L,N);
	float spec = pow(max(dot(V,H),0.0),128);
	vec4 specular = sps*spec*lightColor;

	// diffuse = dot(N,L);
	gl_FragColor = ((amb + diffuse)*base + specular);
	if (marked == 1){
		gl_FragColor = ((amb + diffuse)*vec4(1.0, 1.0, 0.0, 1.0) + specular);
	}

	gl_FragColor.a = base.a;

}