#version 120

uniform mat4 modelToClipMatrix;
attribute vec3 v_position;

void main() {
	gl_Position = modelToClipMatrix * vec4(v_position, 1.0);
}
