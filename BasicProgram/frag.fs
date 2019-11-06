#version 330 core

in vec3 normal;
in vec4 position;

uniform vec3 Eye;


void main() {
	vec4 base = vec4(0.8, 0.8, 0.8, 1.0);
	vec4 lightColor = vec4(1.0, 0.5, .0, 1.0);
	float diffuse;
    vec3 L = normalize(vec3(3.0, 0.0, 3.0)-position.xyz);
	//vec3 L = normalize(vec3(0.0, 1.0, 0.0));
	vec3 N = normalize(normal);

	//vec3 H = normalize(L + vec3(0.0, 0.0, 1.0));
	vec3 H = normalize(L - Eye);
	vec3 V = -reflect(H,N);

	float specular;

	diffuse = max(dot(N,L),0.0);
	// diffuse = dot(N,L);

	if(diffuse < 0.0) {
		diffuse = 0.0;
		specular = 0.0;
	} else {
		specular = pow(max(0.0, dot(N,V)),100.0);
	}

	gl_FragColor = min(0.2*base + 0.8*diffuse*base + 0.8*lightColor*specular, vec4(1.0));
	gl_FragColor.a = base.a;

}