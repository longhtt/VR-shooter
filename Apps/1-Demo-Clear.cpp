// Demo-ClearScreen.cpp

#include <glad.h>														// OpenGL routines
#include <glfw3.h>														// negotiate with OS
#include "GLXtras.h"													// OpenGL convenience routines

const char *vertexShader = R"(
	#version 130														// minimum version (Apple may need 330)
	in vec2 point;														// 2D input expected 
	void main() { gl_Position = vec4(point, 0, 1); }					// promote 2D point to 4D, set built-in output
)";

const char *pixelShader = R"(
	#version 130														// minimum version (Apple may need 330)
	out vec4 pColor;													// 4D color output expected
	void main() { pColor = vec4(0, 1, 0, 1); }							// set (r,g,b,a) output to opaque green
)";

int main() {	 														// execution starts here
	GLFWwindow *w = InitGLFW(150, 50, 300, 300, "Clear");				// create 300x300 titled window
	GLuint program = LinkProgramViaCode(&vertexShader, &pixelShader);	// build shader program
	glUseProgram(program);												// activate shader program
	GLuint vBuffer = 0;													// OpenGL GPU memory ID
	glGenBuffers(1, &vBuffer);											// set ID
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);								// make allocated GPU memory active
	vec2 pts[] = { {-1,-1}, {-1,1}, {1,1}, {1,-1} };					// corner points of default OpenGL window
	glBufferData(GL_ARRAY_BUFFER, sizeof(pts), pts, GL_STATIC_DRAW);	// store the corner points in GPU memory
	VertexAttribPointer(program, "point", 2, 0, (void *) 0);			// set GPU memory transfer to vertex shader
	while (!glfwWindowShouldClose(w)) {									// event loop, check for user kill window
		glDrawArrays(GL_QUADS, 0, 4);									// send 4 vertices from GPU to vertex shader
		glFlush();														// complete graphics operations                                      
		glfwSwapBuffers(w);												// exchange render buffer with display buffer
		glfwPollEvents();												// check for user input
	}
}
