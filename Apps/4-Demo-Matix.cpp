// Demo-4-Matrix.cpp: same as Demo-3-Texture.cpp, but with matrix controlled by mouse drap

#include <glad.h>                                         
#include <glfw3.h>                                        
#include "GLXtras.h"  
#include "IO.h"															// ReadTexture

GLuint program = 0;
GLuint textureName = 0, textureUnit = 0;								// OpenGL identifiers
const char *pixFile = "C:/Users/Jules/Code/Assets/Images/Parrots.jpg";  // bmp, jpg, png, tga supported

const char *vertexShader = R"(
	#version 130
	uniform mat4 m;                                                 // from CPU
	in vec2 point, uv;                                              // from GPU
	out vec2 vuv;           
	void main() {
		vec4 v = vec4(point, 0, 1);
		gl_Position = m*v;                                          // transform
		vuv = uv;                                                   // send to pixel shader
	}
)";

const char *pixelShader = R"(
	#version 130
	in vec2 vuv;                                                    // from vertex shader
	uniform sampler2D textureImage;                                 // from CPU
	out vec4 pColor;                                       
	void main() { pColor = texture(textureImage, vuv); }
)";

float totSpin = 0;
vec2 mouseRef, totPosition;

void Display() {
	glClearColor(0, 0, 0, 0);                                       // set background color
	glClear(GL_COLOR_BUFFER_BIT);                                   // clear background
	mat4 m = Translate(vec3(totPosition, 0))*RotateZ(totSpin)*Scale(.7f);
	SetUniform(program, "m", m);                                    // set vertex shader transform
	SetUniform(program, "textureImage", (int) textureUnit);         // set texture map for pixel shader
	glDrawArrays(GL_QUADS, 0, 4);                                   // draw object                     
	glFlush();                                                      // finish scene
}

void MouseButton(float x, float y, bool left, bool down) { if (left && down) mouseRef = vec2(x, y); }

void MouseMove(float x, float y, bool leftDown, bool rightDown) {
	if (leftDown) {
		vec2 mDif(x-mouseRef.x, y-mouseRef.y);
		totPosition += mDif/1000;
		mouseRef = vec2(x, y);
	}
}

void MouseWheel(float spin) { totSpin += spin; }

void Resize(int width, int height) { glViewport(0, 0, width, height); }

int main() {
	GLFWwindow *w = InitGLFW(200, 200, 600, 600, "Scale & Texture");
	RegisterMouseWheel(MouseWheel);
	RegisterMouseButton(MouseButton);
	RegisterMouseMove(MouseMove);
	RegisterResize(Resize);
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	glUseProgram(program);
	GLuint vBuffer = 0;
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	vec2 pts[] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} };                // vertex geometric location
	vec2 uvs[] = { {0, 0}, {0, 1}, {1, 1}, {1, 0} };                // vertex texture location
	int spts = sizeof(pts), suvs = sizeof(uvs);                     // array sizes
	glBufferData(GL_ARRAY_BUFFER, spts+suvs, NULL, GL_STATIC_DRAW); // allocate GPU buffer
	glBufferSubData(GL_ARRAY_BUFFER, 0, spts, pts);                 // store pts
	glBufferSubData(GL_ARRAY_BUFFER, spts, suvs, uvs);              // store uvs after pts in mem
	textureName = ReadTexture(pixFile);								// store texture image
	glActiveTexture(GL_TEXTURE0+textureUnit);                       // make texture active
	glBindTexture(GL_TEXTURE_2D, textureName);
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);        // set feed for pts
	VertexAttribPointer(program, "uv", 2, 0, (void *) spts);        // set feed for uvs
	printf("mouse wheel to rotate, mouse drag to move\n");
	while (!glfwWindowShouldClose(w)) {
		Display();
		glfwSwapBuffers(w);                          
		glfwPollEvents();
	}
}
