#version 330 core
out vec4 FragColor;

uniform float color;

in vec3 fragPos;

void main()
{
	FragColor = vec4(fragPos.y*0.25, 0.5, 0.5, 1.0);
}