#version 330 core

in vec3 normal;
in vec4 position;
in vec2 texcoords;

uniform vec3 Eye;
uniform sampler2D Tex;


void main() {
	vec3 N = normalize(normal);

	float ams = 0.2;
	float sps = 0.6;

	vec4 base = vec4(1.0, 0.1, 0.0, 1.0);
	vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
	vec3 lightpos = vec3(3.0, 3.0, 3.0);

	vec4 amb = ams*lightColor;

    vec3 L = normalize(lightpos-position.xyz);
	float diff = max(dot(N,L),0.0);
	vec4 diffuse = diff*lightColor;

	vec3 V = normalize(Eye - position.xyz);
	vec3 H = reflect(-L,N);
	float spec = pow(max(dot(V,H),0.0),128);
	vec4 specular = sps*spec*lightColor;

	// diffuse = dot(N,L);
	vec4 tex = texture(Tex,texcoords);

	gl_FragColor = (tex + diffuse)*base + specular;
	gl_FragColor = tex;

	gl_FragColor.a = base.a;

}