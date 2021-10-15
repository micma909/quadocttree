#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "arcball_camera.h"
#include "Octree.h"
#include "Box.h"
#include "Instance.h"

static int frameNr = 0;

static ArcballCamera arcballCamera({ 80,80,90 }, { 0,40,0 }, { 0,1,0 });
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorCallback(GLFWwindow* window, double x, double y);
void scrollCallback(GLFWwindow* window, double x, double y);

class OctreeTest
{
public:
	float RandomFloat(float min, float max)
	{
		assert(max > min);
		float random = ((float)rand()) / (float)RAND_MAX;
		float range = max - min;
		return (random * range) + min;
	}

	void Setup(GLFWwindow* window, int w_width, int w_height)
	{

		Shader debugshader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
		Shader gridShader("QuadTree/shaders/grid.vert", "QuadTree/shaders/grid.frag");
		Shader boxShader("QuadTree/shaders/box.vert", "QuadTree/shaders/box.frag");

		program = debugshader.createProgram();
		gridProgram = gridShader.createProgram();
		boxProgram = boxShader.createProgram();

		arcballCamera.setWH(w_width, w_height);

		Projection = glm::perspective(glm::radians(45.0f), (float)w_width / (float)w_height, 0.01f, 1000.0f);

		Model = glm::mat4(1.0f);
		Model = glm::translate(Model, glm::vec3(0, 0, 0));

		View = arcballCamera.transform();

		mvp = Projection * View * Model;
		mvpId = glGetUniformLocation(program, "MVP");
		gridProjId = glGetUniformLocation(gridProgram, "proj");
		gridViewId = glGetUniformLocation(gridProgram, "view");

		boxModelId = glGetUniformLocation(boxProgram, "model");
		boxViewId = glGetUniformLocation(boxProgram, "view");
		boxProjId = glGetUniformLocation(boxProgram, "projection");

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		this->window = window;

		box.init();
		nrHunters = 4; 
		nrBoids = 2000;

		for (int i = 0; i < nrHunters; i++)
		{
			accelerations.push_back(glm::vec3(0.f));
			positions.push_back(glm::vec3(RandomFloat(-39, 39), RandomFloat(1, 79), RandomFloat(-39, 39)));
			velocities.push_back(glm::vec3(RandomFloat(-0.1f, 0.1f), RandomFloat(-0.1f, 0.1f), RandomFloat(-0.1f, 0.1f)));
			colors.push_back(glm::vec4(1,1,1,1));
			behaviors.push_back(Behavior::Hunter);
		}

		for (int i = 0; i < nrBoids; i++)
		{
			accelerations.push_back(glm::vec3(0.f));
			positions.push_back(glm::vec3(RandomFloat(-39, 39), RandomFloat(1, 79), RandomFloat(-39, 39)));
			velocities.push_back(glm::vec3(RandomFloat(-0.1f, 0.1f), RandomFloat(-0.1f, 0.1f), RandomFloat(-0.1f, 0.1f)));
			colors.push_back(glm::vec4(1,1,1,0.5f));
			behaviors.push_back(Behavior::Boid);
		}

		glGenBuffers(1, &pointBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &colorbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);

		octree = new Octree::Node(BoundingRegion(50, glm::vec3(0, 50, 0)));
		octree->region.debugColor = glm::vec4(0.2, 0.2, 0.2, 1);

		flockingCenter = octree->region.calculateCenter();

		for (int i = 0; i < positions.size(); i++)
		{
			glm::vec3 pSize = glm::vec3(0.05f);
			instances.push_back(Instance(&positions[i], pSize));
			boundingRegions.push_back(BoundingRegion(-pSize, +pSize));
		}

		for (int i = 0; i < positions.size(); i++)
		{
			boundingRegions[i].instance = &instances[i];
			boundingRegions[i].instance->id = i;
			octree->addToPending(&boundingRegions[i]);
		}
		octree->Update();

		glfwSetScrollCallback(window, scrollCallback);
		glfwSetCursorPosCallback(window, cursorCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
	}

	void checkBounds(int i)
	{
		glm:vec3 nextStep = velocities[i];
		glm::vec3 nextPos = *instances[i].position + nextStep;

		BoundingRegion tmp = boundingRegions[i];
		tmp.transform();

		if (!octree->region.containsPoint(nextPos))
		{

			//octree->region.reflectOnBounds(nextPos, velocities[i]);
			octree->region.mirrorOnBounds(nextPos);
			*instances[i].position = nextPos;

			boundingRegions[i].outOfBounds = true;
			boundingRegions[i].octreeNode = nullptr;
		}
	}

	void data_flocking_oct()
	{
		glm::vec3 alignment(0.f);
		glm::vec3 cohesion(0.f);
		glm::vec3 separation(0.f);
		glm::vec3 avoidance(0.f);

		int alignmentCount = 0;
		int cohesionCount = 0;
		int separationCount = 0;

		for (int i = 0; i < instances.size(); i++)
		{
			std::vector<int> foundIDs;
			glm::vec3 pos = *instances[i].position;

			if (behaviors[i] == Behavior::Boid)
			{
				BoundingRegion searchRegion(pos - glm::vec3(5), pos + glm::vec3(5));

				octree->ProximitySearch(*boundingRegions[i].octreeNode, searchRegion, foundIDs);
				//octree->Search(searchRegion, foundIDs);

				colors[i].x -= 0.01f;
				colors[i].y -= 0.001f;
				colors[i].z -= 0.0002f;
				//colors[i].w -= 0.005f;
				colors[i].w = std::max(0.9f, colors[i].w);
				
				for (int j = 0; j < foundIDs.size(); j++)
				{
					int idx = foundIDs[j];

					float d = glm::distance(pos, positions[idx]);

					if (d < 0.001f)
						continue;
					
					float alignmentRadius = 6.f;
					float separationRadius = 5.f;
					float cohesionRadius = 10.f;


					if (behaviors[idx] == Behavior::Boid)
					{
						if (d < alignmentRadius)
						{
							alignment += velocities[idx];
							alignmentCount++;
						}

						if (d < cohesionRadius)
						{
							cohesion += positions[idx];
							cohesionCount++;
						}

						if (d < separationRadius)
						{
							glm::vec3 diff = pos - positions[idx];
							diff /= std::pow(d, 2);
							separation += diff;
							separationCount++;
						}
					}
					else if (behaviors[idx] == Behavior::Hunter)
					{
						//float k = RandomFloat(0.0f, 6.f);
						colors[i] = glm::vec4(1, 1, 1, 1);

						glm::vec3 diff = positions[i] - positions[idx];
						diff /= d;
						
						avoidance = 6.0f * diff;
						alignment *= 0;
						cohesion *= 0;
						alignmentCount = 0;
						cohesionCount = 0;
					}
				}
			}
			else if (behaviors[i] == Behavior::Hunter)
			{
				colors[i] = glm::vec4(0.8, 0.8, 0.8, 1);

				float hunterRadius = 1500;
				BoundingRegion searchRegion(pos - glm::vec3(hunterRadius), pos + glm::vec3(hunterRadius));

				octree->ProximitySearch(*boundingRegions[i].octreeNode, searchRegion, foundIDs);
				//octree->Search(searchRegion, foundIDs);

				for (int j = 0; j < foundIDs.size(); j++)
				{
					int idx = foundIDs[j];
					float d = glm::distance(pos, positions[idx]);

					if (d < hunterRadius && behaviors[idx] != Behavior::Hunter)
					{
						cohesion += positions[idx];
						cohesionCount++;
					}
				}
			}
			
			float alignmentMaxSpeed = 2.0f;
			float alignmentMaxForce = 2.0f;

			if (alignmentCount > 0)
			{
				alignment /= alignmentCount;
				alignment = glm::normalize(alignment) * alignmentMaxSpeed;
				alignment -= velocities[i];
				if (glm::length(alignment) > alignmentMaxForce)
				{
					alignment = glm::normalize(alignment) * alignmentMaxForce;
				}
			}

			float cohesionMaxSpeed = 1.0f;
			float cohesionMaxForce = 0.7f;

			if (cohesionCount > 0)
			{
				cohesion /= cohesionCount;
				cohesion -= positions[i];

				if (glm::length(cohesion) > 0)
					cohesion = glm::normalize(cohesion) * cohesionMaxSpeed;
				cohesion -= velocities[i];

				if (glm::length(cohesion) > cohesionMaxForce)
					cohesion = glm::normalize(cohesion) * cohesionMaxForce;
			}

			float separationMaxSpeed = 1.0f;
			float separationMaxForce = 1.0f;

			if (separationCount > 0)
			{
				separation /= separationCount;
				limit(separation, separationMaxSpeed);
				separation -= velocities[i];
				limit(separation, separationMaxForce);
			}

			accelerations[i] += alignment * 0.01f;
			accelerations[i] += cohesion * 0.01f;
			accelerations[i] += separation * 0.01f;
			accelerations[i] += avoidance * 0.01f;

			if (behaviors[i] == Behavior::Hunter && cohesionCount > 40)
			{
				colors[i] = glm::vec4(1, 0.6, 0, 1);
				cohesionCount = std::min(cohesionCount, 40);
				accelerations[i] *= cohesionCount;
				accelerations[i] -= (pos - flockingCenter) * 0.0001f;
			}
			else
			{
				accelerations[i] -= (pos - flockingCenter) * 0.0002f;
			}

			if (frameNr % 1000 == 0)
				flockingCenter = glm::vec3(RandomFloat(-30, 30), RandomFloat(30, 70), RandomFloat(-30, 30));
		
			velocities[i] *= 0.99f;

			alignment *= 0.0f;
			cohesion *= 0.0f;
			separation *= 0.0f;
			avoidance *= 0.0f;
			alignmentCount = 0;
			cohesionCount = 0;
			separationCount = 0;
			foundIDs.clear();
		}
	}

	void Draw()
	{
		// buttons
		int timeState = glfwGetKey(window, GLFW_KEY_SPACE);
		if (timeState == GLFW_RELEASE && oldTimeState == GLFW_PRESS)
		{
			pauseTime = !pauseTime;
		}
		oldTimeState = timeState;

		int highlightState = glfwGetKey(window, GLFW_KEY_H);
		if (highlightState == GLFW_RELEASE && oldHighlightState == GLFW_PRESS)
		{
			highlightGrid = !highlightGrid;
		}
		oldHighlightState = highlightState;

		// debug boxdraw
		box.positions.clear();
		box.sizes.clear();
		box.colors.clear();

		// Update boids 
		if (!pauseTime)
		{

			// run boid algorithm
			data_flocking_oct();

			for (int i = 0; i < positions.size(); i++)
			{				
				checkBounds(i);
				
				//positions[i] += velocities[i];
				positions[i] += velocities[i];
				velocities[i] += accelerations[i];
			
				accelerations[i] *= 0.f;
			
				// states intended for when some objects stationary, currently all in motion
				states::activate(&boundingRegions[i].instance->state, INSTANCE_MOVED);
			}
		}

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * positions.size(), &positions[0].x, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * colors.size(), &colors[0].x, GL_DYNAMIC_DRAW);

		octree->Update();
	
		octree->Draw(box, highlightGrid);

		View = arcballCamera.transform();
		mvp = Projection * View * Model;

		glUseProgram(program);
		glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);
		
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
		
		glPointSize(1);
		glDrawArrays(GL_POINTS, 0, positions.size());

		std::vector<glm::vec3> hunters;

		for (int i = 0; i < nrHunters; i++)
		{
			hunters.push_back(positions[i]);
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * hunters.size(), &hunters[0].x, GL_DYNAMIC_DRAW);
	
		
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		
		glPointSize(10);
		glDrawArrays(GL_POINTS, 0, hunters.size());
		hunters.clear();

		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(0);

		glm::mat4 boxModelMat = glm::mat4(1.0f);

		glUseProgram(boxProgram);
		glUniformMatrix4fv(boxModelId, 1, GL_FALSE, &boxModelMat[0][0]);
		glUniformMatrix4fv(boxViewId, 1, GL_FALSE, &View[0][0]);
		glUniformMatrix4fv(boxProjId, 1, GL_FALSE, &Projection[0][0]);

		box.render();

		glUseProgram(gridProgram);
		glUniformMatrix4fv(gridProjId, 1, GL_FALSE, &Projection[0][0]);
		glUniformMatrix4fv(gridViewId, 1, GL_FALSE, &View[0][0]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);


		if (glfwGetKey(this->window, GLFW_KEY_R)) 
		{
			Shader debugshader("QuadTree/shaders/debug.vert", "QuadTree/shaders/debug.frag");
			Shader gridShader("QuadTree/shaders/grid.vert", "QuadTree/shaders/grid.frag");
			Shader boxShader("QuadTree/shaders/box.vert", "QuadTree/shaders/box.frag");

			program = debugshader.createProgram();
			gridProgram = gridShader.createProgram();
			boxProgram = boxShader.createProgram();
		}

		frameNr++;
	}

private:
	int nrHunters;
	int nrBoids;

	int w_width;
	int w_height;

	int oldTimeState;
	int oldHighlightState;
	bool pauseTime = true;
	bool highlightGrid = false;

	glm::mat4 mvp;
	GLuint mvpId;
	GLuint gridProjId;
	GLuint gridViewId;

	GLuint boxProjId;
	GLuint boxViewId;
	GLuint boxModelId;

	GLuint program;
	GLuint gridProgram;
	GLuint boxProgram;

	GLuint colorbuffer;
	GLuint pointBuffer;

	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Model;

	Box box;
	// pointer to root node in octree
	Octree::Node* octree;

	std::vector<BoundingRegion> boundingRegions;
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> velocities;
	std::vector<glm::vec3> accelerations;
	std::vector<glm::vec4> colors;

	std::vector<Instance> instances;

	enum Behavior
	{
		Boid = 0,
		Leader = 1,
		Hunter = 2,
		Undefined = 3
	};

	std::vector<Behavior> behaviors;

	glm::vec3 flockingCenter;

	GLFWwindow* window;
};


void scrollCallback(GLFWwindow* window, double x, double y)
{
	//arcball.scrollCallback(window, x, y);
	arcballCamera.scrollCallback(window, x, y);
}

void cursorCallback(GLFWwindow* window, double x, double y)
{
	//arcball.cursorCallback(window, x, y);
	arcballCamera.cursorCallback(window, x, y);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	//arcball.mouseButtonCallback(window, button, action, mods);
	arcballCamera.mouseButtonCallback(window, button, action, mods);
}