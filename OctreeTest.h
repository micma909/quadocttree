#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "QuadTree/Geometries.h"

class OctreeTest
{
public:
	void Setup()
	{
		glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float)w_width / (float)w_height, 0.1f, 100.0f);

		glm::mat4 View = glm::lookAt(
			glm::vec3(4, 3, 3), // position
			glm::vec3(0, 0, 0), // lookat
			glm::vec3(0, 1, 0)  // up
		);

		glm::mat4 Model = glm::mat4(1.0f);

		glm::mat4 mvp = Projection * View * Model;
		mvpId = glGetUniformLocation(program, "MVP");

		glGenBuffers(0, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(gCubeVertexBuffer), gCubeVertexBuffer, GL_STATIC_DRAW);

		glGenBuffers(1, &colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(gCubeColorBuffer), gCubeColorBuffer, GL_STATIC_DRAW);
	}

	void Draw()
	{
		glUseProgram(program);
		glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);


		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glDrawArrays(GL_TRIANGLES, 0, 12 * 3);
	}


private:
	int w_width;
	int w_height;

	glm::mat4 mvp;
	GLuint mvpId;
	GLuint program;

	GLuint vertexbuffer;
	GLuint colorbuffer;

};