#pragma once 

#include <glm/glm.hpp>

#include "Instance.h"

// forward declaration
namespace Octree {
	class Node;
}

enum class BoundTypes
{
	AABB = 0x00,  
	SPHERE = 0x01
};

class BoundingRegion
{
public:
	BoundTypes type;
	
	Instance* instance;

	// pointer for quick access to current octree node
	Octree::Node* octreeNode;

	glm::vec4 debugColor;

	// sphere values
	glm::vec3 center;
	float radius;
	glm::vec3 ogCenter;
	float ogRadius;

	bool outOfBounds = false;

	// bounding box values
	glm::vec3 min;
	glm::vec3 max;
	glm::vec3 ogMin;
	glm::vec3 ogMax;

	// pointers to instance data
	glm::vec3* instancePosition;
	glm::vec3* instanceHalfSize;

	BoundingRegion(BoundTypes type = BoundTypes::AABB) : type(type) {}
	BoundingRegion(glm::vec3 center, float radius) : type(BoundTypes::SPHERE)
		, center(center)
		, radius(radius)
		, ogCenter(center)
		, ogRadius(radius)
	{}
	BoundingRegion(glm::vec3 min, glm::vec3 max) : type(BoundTypes::AABB)
		, min(min)
		, max(max)
		, ogMin(min)
		, ogMax(max)
	{}

	BoundingRegion(float uniformSize, glm::vec3 offset) : type(BoundTypes::AABB)
		, min(offset - glm::vec3(uniformSize))
		, max(offset + glm::vec3(uniformSize))
		, ogMin(min)
		, ogMax(max)
	{}

	glm::vec3 calculateCenter()
	{
		return (type == BoundTypes::AABB) ? (this->min + this->max) * 0.5f : this->center;
	}
	glm::vec3 calculateDimensions()
	{ 
		return (type == BoundTypes::AABB) ? (this->max - this->min) : glm::vec3(2.0f * this->radius);
	}

	// transform for instance
	void transform() {
		if (instance) {
			if (type == BoundTypes::AABB) 
			{
				min = ogMin * instance->size + *instance->position;
				max = ogMax * instance->size + *instance->position;
			}
			else {
				center = ogCenter * instance->size + *instance->position;
				radius = ogRadius * instance->size.x;
			}
		}
	}

	// determine if point inside 
	bool containsPoint(glm::vec3 pt)
	{
		if (type == BoundTypes::AABB)
		{
			return (pt.x >= min.x) && (pt.x <= max.x) &&
				   (pt.y >= min.y) && (pt.y <= max.y) &&
				   (pt.z >= min.z) && (pt.z <= max.z);
		}
		else
		{
			float distSq = 0.0f;
			for (int i = 0; i < 3; i++)
			{
				distSq += (this->center[i] - pt[i]) * (this->center[i] - pt[i]);
			}
		}
	}

	void mirrorOnBounds(glm::vec3& pt)
	{
		if (type == BoundTypes::AABB)
		{
			if (pt.x < min.x)
				pt.x = max.x;
			if (pt.x > max.x)
				pt.x = min.x;

			if (pt.y < min.y)
				pt.y = max.y;
			if (pt.y > max.y)
				pt.y = min.y;

			if (pt.z < min.z)
				pt.z = max.z;
			if (pt.z > max.z)
				pt.z = min.z;
		}
	}

	void reflectOnBounds(glm::vec3& pt, glm::vec3& vel)
	{
		if (type == BoundTypes::AABB)
		{
			if (pt.x < min.x)
				vel = glm::reflect(vel, glm::vec3(1, 0, 0));
			if (pt.x > max.x)
				vel = glm::reflect(vel, glm::vec3(-1, 0, 0));

			if (pt.y < min.y)
				vel = glm::reflect(vel, glm::vec3(0, 1, 0));
			if (pt.y > max.y)
				vel = glm::reflect(vel, glm::vec3(0, -1, 0));

			if (pt.z < min.z)
				vel = glm::reflect(vel, glm::vec3(0, 0, 1));
			if (pt.z > max.z)
				vel = glm::reflect(vel, glm::vec3(0, 0, -1));
		}
	}

	// determine if region completely inside 
	bool containsRegion(BoundingRegion br)
	{
		if (br.type == BoundTypes::AABB)
		{
			// if br is box, just needs to contain min max
			bool containsMin = containsPoint(br.min);
			bool containsMax = containsPoint(br.max);

			return (containsMin && containsMax);
		}
		else if (this->type == BoundTypes::SPHERE && br.type == BoundTypes::SPHERE)
		{
			// if both sphreres, combination of distance from centers and br.radius is less than radius
			return glm::length(this->center - br.center) + br.radius < this->radius;
		}
		else
		{
			// if 'this' box and br sphere 
			if (!containsPoint(br.center))
			{
				// center outside of the box
				return false;
			}

			// center inside the box
			/*
				for each axis z y z 
				- id distance to each side is smaller than the radius, return false
			*/
			for (int i = 0; i < 3; i++)
			{
				if (abs(max[i] - br.center[i]) < br.radius ||
					abs(br.center[i] - min[i]) < br.radius)
				{
					return false;
				}
			}
		}
		return true;
	}

	// determine if region intersects (partial containment)
	bool intersectsWith(BoundingRegion br)
	{
		// has to overlap on all axes - for true

		if (type == BoundTypes::AABB && br.type == BoundTypes::AABB)
		{
			// both are boxes
			glm::vec3 rad = calculateDimensions() / 2.0f; // 'radius' of this box
			glm::vec3 radBr = br.calculateDimensions() / 2.0f; // 'radius' of this br

			glm::vec3 center = calculateCenter();	// center of this box
			glm::vec3 centerBr = br.calculateCenter();	// center of br

			glm::vec3 dist = abs(center - centerBr);

			for (int i = 0; i < 3; i++)
			{
				if (dist[i] > rad[i] + radBr[i])
				{
					// no overlap on this axis
					return false;
				}
			}
			// failed to prove wrong on each axis
			return true;
		}
		else if (type == BoundTypes::SPHERE && br.type == BoundTypes::SPHERE)
		{
			// both spheres
			return glm::length(center - br.center) < (radius + br.radius);
		}
		else if (type == BoundTypes::SPHERE)
		{
			// this sphere, br box
			// determine if sphere aboce top, below bottom etc.
			// find distSquared to closest plane
			
			float distSquared = 0.0f;
			for (int i = 0; i < 3; i++)
			{
				if (center[i] < br.min[i])
				{
					// beyond min
					distSquared += (br.min[i] - center[i]) * (br.min[i] - center[i]);
				}
				else if (center[i] > br.max[i])
				{
					distSquared += (center[i] - br.max[i]) * (center[i] - br.max[i]);
				}
			}

			return distSquared < (radius * radius);
		}
		else
		{
			// this is a box, br is a sphere
			// call algorithm for br (defined in preceding else if block)
			return br.intersectsWith(*this);
		}

	}

};