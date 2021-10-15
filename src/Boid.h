#pragma once
#include <random>
#include <glm/glm.hpp>
#include <cmath>

#include "Util.h"
// Written OOP to compare performance, not used

class Boid
{
public: 
	Boid(int w_width, int w_height) : position(RandomFloat(0, w_width), RandomFloat(0, w_height))
		, velocity(RandomFloat(-1.f, 1.f), RandomFloat(-1.f, 1.f))
		, acceleration(0.f, 0.f)
		, radius(10.0f)
		, maxForce(0.2f)
		, maxSpeed(4.2f)
	{}
	
	glm::vec2 align(std::vector<Boid>* boids)
	{
		float perceptionRadius = 50.f;
		glm::vec2 steering(0.f,0.f);
		int count = 0;
		for (auto other : *boids)
		{
			float d = distance(this->position, other.position);

			if (&other != this && d < perceptionRadius)
			{
				steering += other.velocity;
				count++;
			}
		}

		if (count)
		{
			steering /= count;
			steering = glm::normalize(steering) * maxSpeed;
			steering -= this->velocity;
			float length = glm::length(steering);
			if (length > maxForce)
			{
				steering = glm::normalize(steering)* maxForce;
			}
			length = glm::length(steering);
			int a; 
			a = 1;
		}
		return steering;
	}

	glm::vec2 cohesion(std::vector<Boid>* boids)
	{
		float perceptionRadius = 100;
		glm::vec2 steering(0.f, 0.f);
		int count = 0;
		for (auto other : *boids) // own function - boid avg
		{
			float d = distance(this->position, other.position);

			if (&other != this && d < perceptionRadius)
			{
				steering += other.position;
				count++;
			}
		}

		if (count > 0)
		{
			steering /= count;
			steering -= this->position;
			if(glm::length(steering) > 0)
				steering = glm::normalize(steering) * maxSpeed;
			steering -= this->velocity;
			if (glm::length(steering) > maxForce)
				steering = glm::normalize(steering) * maxForce;
		}
		return steering;
	}

	glm::vec2 separation(std::vector<Boid>* boids)
	{
		float perceptionRadius = 50;
		glm::vec2 steering(0.f, 0.f);
		int count = 0;
		for (auto other : *boids) // own function - boid avg
		{
			float d = distance(this->position, other.position);

			if (&other != this && d < perceptionRadius && d > 0)
			{
				glm::vec2 diff = this->position - other.position;
				diff /= d;

				steering += diff;
				count++;
			}
		}

		if (count > 0)
		{
			steering /= count;
			if (glm::length(steering) > 0)
				steering = glm::normalize(steering) * maxSpeed;
			steering -= this->velocity;
			if (glm::length(steering) > maxForce)
				steering = glm::normalize(steering) * maxForce;
		}
		return steering;
	}

	void flock(std::vector<Boid>* boids)
	{
		glm::vec2 alignment  = this->align(boids);
		glm::vec2 cohesion   = this->cohesion(boids);
		glm::vec2 separation = this->separation(boids);

		this->acceleration += separation*1.5f;
		this->acceleration += alignment;
		this->acceleration += cohesion;
		
	}

	void update()
	{
		position += velocity;
		velocity += acceleration;

		if(glm::length(velocity) > maxSpeed)
			velocity = glm::normalize(velocity) * maxSpeed;

		this->acceleration *= 0.0f;
	}
	
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec2 acceleration;

	float radius;
	float maxForce;
	float maxSpeed;

private:
};

