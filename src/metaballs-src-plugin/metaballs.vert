#version 330

layout(location = 0) in vec2 iPosition;

void main() {
    gl_Position = vec4(iPosition, 0, 1);
}
