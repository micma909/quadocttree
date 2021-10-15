#pragma once

#define NR_CHILDREN 8
#define MIN_BOUNDS 0.1

#include <vector>
#include <queue>
#include <stack>
#include <set>

//#include "Box.h"

#include "List.h"
#include "States.h"
#include "Bounds.h"

class Box;

namespace Octree
{
	enum class Octant : unsigned char
	{
		O1 = 0x01, // 0b00000001
		O2 = 0x02, // 0b00000010
		O3 = 0x04, // 0b00000100
		O4 = 0x08, // 0b00001000
		O5 = 0x10, // 0b00010000
		O6 = 0x20, // 0b00100000
		O7 = 0x40, // 0b01000000
		O8 = 0x80  // 0b10000000
	};
	/*
		utility methods callbacks
	*/

	// calculate bounds of speciefied quadrant in bounding region
	void calculateBounds(BoundingRegion& out, Octant octant, BoundingRegion parentRegion);

	class Node 
	{
	public: 
		Node* parent; 
		Node* children[NR_CHILDREN];

		unsigned char activeOctants;
		bool hasChildren = false;
		bool treeReady = false;
		bool treeBuilt = false;
		
		int level = 0;
		int activeChildren = 0;

		int maxLifespan = 8;
		int currentLifespan = -1;

		std::vector<BoundingRegion*> objects;
		std::queue<BoundingRegion*> pendingQueue;
		std::set<int> instanceIDs;

		BoundingRegion region;

		Node();
		Node(BoundingRegion bounds);
		Node(BoundingRegion bounds, std::vector<BoundingRegion> objectList);
		Node(BoundingRegion* bounds, std::vector<BoundingRegion*> objectList);


		void addToPending(BoundingRegion* bounds);

		void Search(BoundingRegion& bounds, std::vector<int>& ids);
		void ProximitySearch(Node& origin, BoundingRegion& search, std::vector<int>& ids);

		bool RemoveStaleBranches();
		
		void Build();
		void Update();
		void ProcessPending();
		bool Insert(BoundingRegion* obj);
		void Destroy();

		void Draw(Box& box, bool highlightGrid = false);
	};
}
