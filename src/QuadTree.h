#pragma once
#include <vector>
#include <cassert>
#include <unordered_map>
#include <any>
#include <array>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <functional>
#include <thread>
#include <future>

#include <omp.h>

#define QTCAPACITY 8
#define THREAD_MAX 12 

float Q_rsqrt(float number)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y = number;
	i = *(long*)&y;                       // evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);               // what the fuck? 
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y));   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

void fastLimit(glm::vec2& v, float lim)
{
	v *= Q_rsqrt(glm::dot(v, v)) * lim;
}

struct BoidParameters
{
	float alignmentRadius;
	float cohesionRadius;
	float separationRadius;

	float alignmentMaxSpeed;
	float cohesionMaxSpeed;
	float separationMaxSpeed;

	float alignmentMaxForce;
	float cohesionMaxForce;
	float separationMaxForce;

	float queryRadius;

	int xMax;
	int xMin;
	int yMax;
	int yMin;

	bool alignmentOn;
	bool cohesionOn;
	bool separationOn;

	glm::vec2 alignment;
	glm::vec2 cohesion;
	glm::vec2 separation;
	int aCount;
	int cCount;
	int sCount;
};

using namespace glm;
class QuadTree;
struct Point
{
	Point() = default;
	Point(float x, float y, int index = 0) : pos(x, y),  radius(0.f), indexGlobal(index), visited(false){}
	glm::vec2 pos;
	glm::vec2 vel;
	glm::vec2 acc;
	glm::vec4 col;

	float radius;
	int indexGlobal;
	int indexCell;
	bool visited;
};

class Rectangle
{
public:
	Rectangle() : x(0), y(0), w(0), h(0)
	{

	}
	Rectangle(float x, float y, float w, float h) : x(x), y(y), w(w), h(h)
	{
		xMax = x + w;
		xMin = x - w;
		yMax = y + h;
		yMin = y - h;
	}

	float x;
	float y;
	float w; 
	float h;

	float xMax;
	float xMin;
	float yMax;
	float yMin;

	bool contains(glm::vec2 p)
	{
		return (p.x > xMin && p.x <= xMax && 
				p.y > yMin && p.y <= yMax); 
	}

	bool intersects(Rectangle range)
	{
		return !(range.xMin > xMax ||
				 range.xMax < xMin ||
				 range.yMin > yMax ||
				 range.yMax < yMin);
	}
};

class QuadTree
{
public:
	QuadTree(Rectangle boundary, int n) : boundary(boundary), capacity(n), divided(false), level(0), nrChildren(0),
		parent(nullptr)
	{}

	bool insert(glm::vec2& position,
		glm::vec2& velocity,
		glm::vec2& acceleration, 
		glm::vec4& color,
		int index,
		bool visited)
	{
		if (!divided && nextFree < capacity) // only insert to leafs!
		{
			divided = false;
			arrayPos[nextFree] = position;
			arrayVel[nextFree] = velocity;
			arrayAcc[nextFree] = acceleration;
			arrayCol[nextFree] = color;
			arrayIdx[nextFree] = index;
			arrayVis[nextFree] = visited;
			nextFree++;

			return true;
		}
		else // if capacity over-reached - need new split!
		{
			if (!divided)
			{
				float w = boundary.w / 2.0f;
				float h = boundary.h / 2.0f;

				float xp = boundary.x + w;
				float xm = boundary.x - w;
				float yp = boundary.y + h;
				float ym = boundary.y - h;

				// bounds for each quadrant 
				bounds[0] = { xp, ym, w, h }; // north east
				bounds[1] = { xm, ym, w, h }; // north west
				bounds[2] = { xp, yp, w, h }; // south east
				bounds[3] = { xm, yp, w, h }; // south west

				newTrees = reinterpret_cast<QuadTree*>(new char[4 * sizeof(QuadTree)]);

				while (nextFree) // just divided and has points -> pass down
				{
					nextFree--;
					for (int i = 0; i < 4; i++)
					{
						if (bounds[i].contains(arrayPos[nextFree]))
						{
							if (!quadrants[i]) 
							{
								quadrants[i] = newTrees + i;
								new (quadrants[i]) QuadTree(bounds[i], capacity); // run constructor			
								quadrants[i]->parent = this;
								quadrants[i]->level = this->level + 1;
								quadrants[i]->divided = false;
								nrChildren++;
							}
							if (!quadrants[i]->insert(arrayPos[nextFree], 
								arrayVel[nextFree], 
								arrayAcc[nextFree], 
								arrayCol[nextFree], 
								arrayIdx[nextFree],
								arrayVis[nextFree]))
								nextFree++;
						}
					}
				}
				divided = true;
			}

			for (int i = 0; i < 4; i++)
			{
				if (bounds[i].contains(position))
				{
					if (!quadrants[i])
					{
						//quadrants[i] = new QuadTree(bounds[i], capacity);
						quadrants[i] = newTrees + i;
						new (quadrants[i]) QuadTree(bounds[i], capacity); // run constructor			
						quadrants[i]->parent = this;
						quadrants[i]->divided = false;
						quadrants[i]->level = this->level + 1;
						nrChildren++;
					}
					return quadrants[i]->insert(position, velocity, acceleration, color, index, visited);
				}
			}
		}
		// something went wrong
		return false;
	}

	void neighbourCheck(Rectangle& range, BoidParameters& boidParams, glm::vec2& position, glm::vec2& velocity, glm::vec2& acceleration, int index)
	{
		for (int i = 0; i < 4; i++)
			if (quadrants[i] && range.intersects(boundary))
				quadrants[i]->neighbourCheck(range, boidParams, position, velocity, acceleration, index);

		glm::vec2 diff;
		float distance;
		
		for (int i = 0; i < nextFree; i++)
		{
			if (index != arrayIdx[i])
			{
				diff = position - arrayPos[i];

				distance = diff.x * diff.x + diff.y * diff.y;

				if (distance < boidParams.alignmentRadius)
				{
					boidParams.alignment += arrayVel[i];
					boidParams.aCount++;
				}

				if (distance < boidParams.cohesionRadius)
				{
					boidParams.cohesion += arrayPos[i];
					boidParams.cCount++;
				}

				if (distance < boidParams.separationRadius)
				{
					diff /= (distance * distance);
					boidParams.separation += diff;
					boidParams.sCount++;
				}
			}
		}
	}

	void updateBoids(BoidParameters& bp)
	{
		for (int i = 0; i < nextFree; i++)
		{
			bp.alignment *= 0.f;
			bp.cohesion *= 0.f;
			bp.separation *= 0.f;
			bp.aCount = 0;
			bp.cCount = 0;
			bp.sCount = 0;

			int currIdx = arrayIdx[i];
			glm::vec2& currPos = arrayPos[i];
			glm::vec2& currVel = arrayVel[i];
			glm::vec2& currAcc = arrayAcc[i];
			glm::vec2* otherPos = nullptr;

			if (parent)
			{
				qVisited = false;

				arrayCol[i].x -= 0.001f;
				arrayCol[i].y -= 0.005f;
				arrayCol[i].z += 0.05f;

				arrayCol[i].x = std::max(arrayCol[i].x, 0.2f);
				arrayCol[i].y = std::max(arrayCol[i].y, 0.2f);
				arrayCol[i].z = std::min(arrayCol[i].z, 0.2f);

				Rectangle searchArea(currPos.x, currPos.y, bp.queryRadius, bp.queryRadius);
				parent->neighbourCheck(searchArea, bp, currPos, currVel, currAcc, currIdx);

				if (bp.cCount > 3)
					arrayCol[i] = glm::vec4(1, 0.7, 0, 1);

			}
			arrayVis[i] = true;

			if (bp.aCount > 0 && bp.alignmentOn)
			{
				bp.alignment /= bp.aCount;
				fastLimit(bp.alignment, bp.alignmentMaxSpeed);
				bp.alignment -= currVel;
				fastLimit(bp.alignment, bp.alignmentMaxForce);
			}


			if (bp.sCount > 0 && bp.separationOn)
			{
				bp.separation /= bp.sCount;
				fastLimit(bp.separation, bp.separationMaxSpeed);
				bp.separation -= currVel;
				fastLimit(bp.separation, bp.separationMaxForce);
			}

			if (bp.cCount > 0 && bp.cohesionOn)
			{
				bp.cohesion /= bp.cCount;
				bp.cohesion -= currPos;
				fastLimit(bp.cohesion, bp.cohesionMaxSpeed);
				bp.cohesion -= currVel;
				fastLimit(bp.cohesion, bp.cohesionMaxForce);
			}

			currPos += currVel * 0.1f;

			if (!bp.alignmentOn || isnan(bp.alignment.x) || isnan(bp.alignment.y))
				bp.alignment = glm::vec2(0);
			if (!bp.cohesionOn || isnan(bp.cohesion.x) || isnan(bp.cohesion.y))
				bp.cohesion = glm::vec2(0);
			if (!bp.separationOn || isnan(bp.separation.x) || isnan(bp.separation.y))
				bp.separation = glm::vec2(0);

			currAcc += bp.alignment + bp.cohesion + bp.separation;

			// mirror
			currPos.x = (currPos.x > bp.xMax) ? bp.xMin : ((currPos.x < bp.xMin) ? bp.xMax : currPos.x);
			currPos.y = (currPos.y > bp.yMax) ? bp.yMin : ((currPos.y < bp.yMin) ? bp.yMax : currPos.y);
		}
		qVisited = true;
	}

	void updateVelocity()
	{
		int i;
		for (i = 0; i < 4; i++)
		{
			if (quadrants[i]) // if has quadtrant i 
			{
				quadrants[i]->updateVelocity();
			}
		}
		for (i = 0; i < nextFree; i++)
		{
			arrayVel[i] += arrayAcc[i];
			arrayAcc[i] *= 0.f;
		}
	}

	void update(BoidParameters& params)
	{
		int i;
		for (i = 0; i < 4; ++i)
		{
			if (quadrants[i]) // if has quadtrant i 
			{
				quadrants[i]->update(params);
			}
		}

		updateBoids(params);

		int nrToRelocate = 0;
		int start = nextFree - 1;
		int index = -1;
		for (i = nextFree - 1; i >= 0; --i)
		{
			if (!boundary.contains(arrayPos[i]))
			{
				std::swap(arrayPos[i], arrayPos[nextFree - 1]);
				std::swap(arrayVel[i], arrayVel[nextFree - 1]);
				std::swap(arrayAcc[i], arrayAcc[nextFree - 1]);
				std::swap(arrayCol[i], arrayCol[nextFree - 1]);
				std::swap(arrayIdx[i], arrayIdx[nextFree - 1]);
				std::swap(arrayVis[i], arrayVis[nextFree - 1]);

				nextFree--;
				nrToRelocate++;
			}
		}

		if (!nrToRelocate)
			return;

		QuadTree* current = this;
		for (i = 0; i < nrToRelocate; ++i)
		{
			index = start - i;
			while (!current->boundary.contains(arrayPos[index]))
			{
				if (current->parent != nullptr)
					current = current->parent;
				else
					break; // if root node, the leave
			}
			if (!current->insert(arrayPos[index], arrayVel[index], arrayAcc[index], arrayCol[index], arrayIdx[index], arrayVis[index]))
			{
				if (isnan(arrayPos[index].x))
				{
					arrayPos[index] = glm::vec2(0);
				}
			}
		}
	}

	bool fastQuery(Rectangle& range, std::function<void(QuadTree&)> func)
	{
		int i;
		for (i = 0; i < 4; i++)
		{
			if (quadrants[i] && range.intersects(boundary))
			{
				quadrants[i]->fastQuery(range, func);
			}
		}
		func(*this);
		return false;
	}

	void removeStale()
	{
		for (int i = 0; i < 4; i++)
		{
			if (quadrants[i])
			{
				quadrants[i]->removeStale();
				if (quadrants[i]->nextFree == 0 && !quadrants[i]->nrChildren)
				{
					quadrants[i]->~QuadTree();
					quadrants[i] = nullptr;
					nrChildren--;
				}

				if (nrChildren == 0)
				{
					free(newTrees);
					divided = false;
					return;
				}

			}
		}
	}

	void merge()
	{
		if (nrChildren == 0)
			return;

		bool parentToleaves = true;
		int totalInserted = 0;
		for (int i = 0; i < 4; i++)
		{
			if (quadrants[i])
			{
				if (quadrants[i]->nrChildren)
					parentToleaves = false;
				quadrants[i]->merge();
				totalInserted += quadrants[i]->nextFree;
			}
		}

		if (parentToleaves && totalInserted > 0 && totalInserted < capacity-(nextFree-1))
		{
			for (int i = 0; i < 4; i++)
			{
				if (quadrants[i])
				{
					int index = quadrants[i]->nextFree;
					if (index > 0)
					{
						while (index)
						{
							index--;
							arrayPos[nextFree] = quadrants[i]->arrayPos[index];
							arrayVel[nextFree] = quadrants[i]->arrayVel[index];
							arrayAcc[nextFree] = quadrants[i]->arrayAcc[index];
							arrayCol[nextFree] = quadrants[i]->arrayCol[index];
							arrayIdx[nextFree] = quadrants[i]->arrayIdx[index];

							nextFree++;
						}
						quadrants[i]->~QuadTree();
						quadrants[i] = nullptr;
						nrChildren--;
					}
				}
				if (nrChildren == 0)
				{
					free(newTrees);
					divided = false;
					return;
				}
			}
		}
	}

	void getAllPositions(std::vector<glm::vec2>& positionVec, std::vector<glm::vec4>& colors, int& index)
	{
		for (int i = 0; i < 4; i++)
		{
			if (quadrants[i]) // if has quadtrant i 
			{
				quadrants[i]->getAllPositions(positionVec, colors, index);
			}
		}

		for (int i = 0; i < nextFree; i++)
		{
			positionVec[index] = arrayPos[i];
			colors[index] = arrayCol[i];

			index++;
		}
		this->qVisited = false;
	}

	// used for debugging, ignore...
	void GetBoundsLineSegments(std::vector<glm::vec2>& lines)
	{
		for (int i = 0; i < 4; i++)
		{
			if (quadrants[i] && quadrants[i]->nrChildren)
				quadrants[i]->GetBoundsLineSegments(lines);
		}

		lines.push_back(glm::vec2(boundary.xMin, boundary.y));
		lines.push_back(glm::vec2(boundary.xMax, boundary.y));
		lines.push_back(glm::vec2(boundary.x, boundary.yMax));
		lines.push_back(glm::vec2(boundary.x, boundary.yMin));
	}

	int level = 0;

//private:
	Rectangle boundary;
	int capacity;
	
	// switching to more data-oriented
	alignas(16) std::array<glm::vec2, QTCAPACITY> arrayPos;
	alignas(16) std::array<glm::vec2, QTCAPACITY> arrayVel;
	alignas(16) std::array<glm::vec2, QTCAPACITY> arrayAcc;
	alignas(16) std::array<glm::vec4, QTCAPACITY> arrayCol;
	alignas(16) std::array<int, QTCAPACITY> arrayIdx;
	alignas(16) std::array<bool, QTCAPACITY> arrayVis;

	int nextFree = 0;

	bool qVisited = false;
	bool divided = false;
	int nrChildren;

	QuadTree* parent;

	std::array<QuadTree*, 4> quadrants{};
	QuadTree* newTrees; // use if alloc all quadrants at once

	std::array<Rectangle, 4> bounds;
};