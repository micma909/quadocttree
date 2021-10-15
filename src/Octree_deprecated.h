#pragma once
#include <vector>
#include <glm/glm.hpp>

using namespace glm;

enum OctreeIndex
{
	BottomLeftFront = 0, // = 000,
	BottomRightFront = 2, //= 010, 
	BottomRightBack = 3, //= 011,
	BottomLeftBack = 1, //= 001,
	TopLeftFront = 4, //= 100,
	TopRightFront = 6, //= 110,
	TopRightBack = 7, //= 111,
	TopLeftBack = 5, //= 101,
};

template<typename T> 
class Octree
{
public:

	Octree(vec3 position, float size, int depth)
	{
		node = new OctreeNode<T>(position, size);
	}

private:
	
	template<typename T>
	class OctreeNode
	{
	private:
		vec3 position;
		float size;

		OctreeNode<T> subNodes[];
		std::vector<T> value;
	public: 
		OctreeNode(vec3 pos, float size)
		{
			this->position = pos;
			this->size = size;
		}

		void Subdivide()
		{
			subNodes = new OctreeNode<T>[8];

			for (int i = 0; subNodes.size(); ++i)
			{
				vec3 newPos = this->position;

				if ((i & 4) == 4)
				{
					newPos.y += size * 0.25f;
				}
				else
				{
					newPos.y -= size * 0.25f;
				}

				if ((i & 2) == 2)
				{
					newPos.x += size * 0.25f;
				}
				else
				{
					newPos.x -= size * 0.25f;
				}

				if ((i & 1) == 1)
				{
					newPos.z += size * 0.25f;
				}
				else
				{
					newPos.z -= size * 0.25f;
				}


				subNodes[i] = new OctreeNode<T>(newPos, size * 0.5f);
			}
		}

		bool IsLeaf()
		{
			return subnodes == nullptr;
		}

	};

	OctreeNode<T> node;
	int depth;


	int GetIndexOfPosition(vec3 lokokupPos, vec3 nodePos)
	{
		int index = 0;

		index |= lokokupPos.y > nodePos.y ? 4 : 0;
		index |= lokokupPos.x > nodePos.x ? 2 : 0;
		index |= lokokupPos.z > nodePos.z ? 1 : 0;

		return index;
	}
public:
	
	OctreeNode<T> GetRood()
	{
		return node;
	}


};
