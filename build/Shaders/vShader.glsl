#version 430 core
layout(location = 0) in vec4 vPosition;
out vec3 vertexPos;
void main(){
gl_Position = vPosition;
vertexPos = vec3(vPosition.x,vPosition.y,1.0);
}
