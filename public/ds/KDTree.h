//**************** Copyright (C) Parallel Prevision, L.L.C 2011 ******************
//
// Description: DarkTech 'Altereality' Kd Tree class
//
//********************************************************************************

#ifndef KDTREE_H
#define KDTREE_H

#include <stdlib.h>

template <class TYPE>
struct KdNode {
	KdNode <TYPE> *lower, *upper;
	unsigned int index;
	TYPE *point;
};

template <class TYPE>
class KdTree 
{
public:
	KdTree(const unsigned int nComp, const unsigned int capasity)
	{
		top = nullptr;
		nComponents = nComp;
		count = 0;

		curr = mem = (unsigned char *) PPAlloc(capasity * (sizeof(KdNode <TYPE>) + nComp * sizeof(TYPE)));
	}

	~KdTree()
	{
		PPFree(mem);
	}

	unsigned int addUnique(const TYPE *point)
	{
		if (top != nullptr){
			return addUniqueToNode(top, point);
		}
		else 
		{
			top = newNode(point);
			return 0;
		}
	}

	void clear()
	{
		curr = mem;

		top = nullptr;
		count = 0;
	}

	const unsigned int getCount() const { return count; }

private:
	unsigned int addUniqueToNode(KdNode <TYPE> *node, const TYPE *point)
	{
		unsigned int comp = 0;

		while (true){
			if (point[comp] < node->point[comp])
			{
				if (node->lower)
				{
					node = node->lower;
				} 
				else 
				{
					node->lower = newNode(point);
					return node->lower->index;
				}
			} 
			else if (point[comp] > node->point[comp])
			{
				if (node->upper)
				{
					node = node->upper;
				}
				else 
				{
					node->upper = newNode(point);
					return node->upper->index;
				}
			} 
			else if (isEqualToNode(node, point)){
				return node->index;
			} 
			else 
			{
				if (node->upper)
				{
					node = node->upper;
				}
				else
				{
					node->upper = newNode(point);
					return node->upper->index;
				}
			}
			if (++comp == nComponents) comp = 0;
		}
	}

	KdNode <TYPE> *newNode(const TYPE *point)
	{
		KdNode <TYPE> *node = (KdNode <TYPE> *) newMem(sizeof(KdNode <TYPE>));
		node->point = (TYPE *) newMem(nComponents * sizeof(TYPE));

		memcpy(node->point, point, nComponents * sizeof(TYPE));

		node->lower = nullptr;
		node->upper = nullptr;
		node->index = count++;
		return node;
	}

	bool isEqualToNode(const KdNode <TYPE> *node, const TYPE *point)
	{
		unsigned int i = 0;
		do 
		{
			if (node->point[i] != point[i]) return false;
			i++;
		}while (i < nComponents);
		return true;
	}

	void *newMem(const unsigned int size)
	{
		unsigned char *rmem = curr;
		curr += size;
		return rmem;
	}

	KdNode <TYPE> *top;
	unsigned int nComponents;
	unsigned int count;

	unsigned char *mem, *curr;
};

#endif // _KDTREE_H_
