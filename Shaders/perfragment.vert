#version 120

attribute vec3 v_position;
attribute vec3 v_normal;
attribute vec2 v_texCoord;

uniform int active_lights_n; // Number of active lights (< MG_MAX_LIGHT)

uniform mat4 modelToCameraMatrix;
uniform mat4 cameraToClipMatrix;
uniform mat4 modelToWorldMatrix;
uniform mat4 modelToClipMatrix;

//Dijo en clase no normalizar
varying vec3 f_position;
varying vec3 f_viewDirection;
varying vec3 f_normal;
varying vec2 f_texCoord;

void main() {
	//posicion con un uno por que es un punto
	vec4 ojo = modelToCameraMatrix * vec4(v_position,1);
	f_position = ojo.xyz;
	//la direccion tambien con un 1 por ser un punto
	vec4 zeros = vec4(0,0,0,1);
	zeros -= ojo;
	f_viewDirection = zeros.xyz;

	//normal con 0 por ser vector
	vec4 norm = vec4(v_normal,0);
	norm = modelToCameraMatrix * norm;
	f_normal = norm.xyz;

	//la textura
	f_texCoord = v_texCoord;

	gl_Position = modelToClipMatrix * vec4(v_position, 1.0);
}
