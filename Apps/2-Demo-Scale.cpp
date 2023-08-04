// Demo-Scale.cpp

#include <glad.h>                                         
#include <glfw3.h>                                        
#include "GLXtras.h"  
#include "time.h"												// for animation

GLuint program = 0;												// make global
float scale = .5;												// make global
time_t startTime = clock();										// app start
bool animate = false;											// scale via time, else via mouse wheel

const char *vertexShader = R"(
	#version 130
	uniform float scale = 1;									// from CPU
	in vec2 point;												// from GPU             
	void main() { gl_Position = vec4(scale*point, 0, 1); }		// scale, promote to 4D
)";

const char *pixelShader = R"(
	#version 130
	out vec4 pColor;											// r,g,b,a output
	void main() { pColor = vec4(0, 1, 0, 1); }					// opaque green
)";

void Display() {
	if (animate) {
		float dt = (float)(clock()-startTime)/CLOCKS_PER_SEC;	// elapsed time
		float freq = .5f, ang = 2*3.1415f*dt*freq;				// current angle
		scale = (1+sin(ang))/2;
	}
	SetUniform(program, "scale", scale);						// send scale to pixel shader
	glClearColor(0, 0, 1, 1);									// set background blue
	glClear(GL_COLOR_BUFFER_BIT);								// clear background
	glDrawArrays(GL_QUADS, 0, 4);								// draw object                     
	glFlush();													// finish scene
}

void MouseWheel(float spin) {
	scale *= spin > 0? 1.1f : .9f;								// adjust scale factor
}

void Keyboard(int key, bool press, bool shift, bool control) {
	if (press && key == 'A')
		animate = !animate;
}

int main() {
	GLFWwindow *w = InitGLFW(200, 200, 600, 600, "Scale");
	program = LinkProgramViaCode(&vertexShader, &pixelShader);
	glUseProgram(program);
	GLuint vBuffer = 0;
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	float pts[][2] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} };
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);
	RegisterMouseWheel(MouseWheel);
	RegisterKeyboard(Keyboard);
	printf("Usage: mouse-wheel to change scale or 'A' to animate\n");
	while (!glfwWindowShouldClose(w)) {
		Display();												// modularize
		glfwSwapBuffers(w);                          
		glfwPollEvents();
	}
	glfwDestroyWindow(w);
	glfwTerminate();
}
