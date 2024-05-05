/*
 *  Copyright (c) 2014, Oculus VR, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant 
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/// \file DS_MemoryPool.h
/// https://github.com/facebookarchive/RakNet/blob/master/Source/DS_MemoryPool.h


#ifndef __MEMORY_POOL_MODERN_H
#define __MEMORY_POOL_MODERN_H

#ifndef __APPLE__
// Use stdlib and not malloc for compatibility
#include <stdlib.h>
#endif
#include "RakAssert.h"
#include "Export.h"

// DS_MEMORY_POOL_MAX_FREE_PAGES must be > 1
#define DS_MEMORY_POOL_MAX_FREE_PAGES 4

//#define _DISABLE_MEMORY_POOL

/// Very fast memory pool for allocating and deallocating structures that don't have constructors or destructors.
/// Contains a list of pages, each of which has an array of the user structures
template <class MemoryBlockType>
class RAK_DLL_EXPORT MemoryPoolModern
{
public:
	struct Page;
	struct MemoryWithPage
	{
		MemoryBlockType userMemory;
		Page *parentPage;
	};
	struct Page
	{
		MemoryWithPage** availableStack;
		int availableStackSize;
		MemoryWithPage* block;
		Page *next, *prev;
	};

	MemoryPoolModern();
	~MemoryPoolModern();
	void SetPageSize(int size); // Defaults to 16384 bytes
	MemoryBlockType *Allocate();
	void Release(MemoryBlockType *m);
	void Clear();

	int GetAvailablePagesSize(void) const {return availablePagesSize;}
	int GetUnavailablePagesSize(void) const {return unavailablePagesSize;}
	int GetMemoryPoolPageSize(void) const {return memoryPoolPageSize;}
protected:
	int BlocksPerPage(void) const;
	void AllocateFirst(void);
	bool InitPage(Page *page, Page *prev);

	// availablePages contains pages which have room to give the user new blocks.  We return these blocks from the head of the list
	// unavailablePages are pages which are totally full, and from which we do not return new blocks.
	// Pages move from the head of unavailablePages to the tail of availablePages, and from the head of availablePages to the tail of unavailablePages
	Page *availablePages, *unavailablePages;
	int availablePagesSize, unavailablePagesSize;
	int memoryPoolPageSize;
};

template<class MemoryBlockType>
MemoryPoolModern<MemoryBlockType>::MemoryPoolModern()
{
#ifndef _DISABLE_MEMORY_POOL
	//AllocateFirst();
	availablePagesSize=0;
	unavailablePagesSize=0;
	memoryPoolPageSize=16384;
#endif
}

template<class MemoryBlockType>
MemoryPoolModern<MemoryBlockType>::~MemoryPoolModern()
{
#ifndef _DISABLE_MEMORY_POOL
	Clear();
#endif
}

template<class MemoryBlockType>
void MemoryPoolModern<MemoryBlockType>::SetPageSize(int size)
{
	memoryPoolPageSize=size;
}

template<class MemoryBlockType>
MemoryBlockType* MemoryPoolModern<MemoryBlockType>::Allocate()
{
#ifdef _DISABLE_MEMORY_POOL
	return (MemoryBlockType*) malloc(sizeof(MemoryBlockType));
#else

	if (availablePagesSize>0)
	{
		MemoryBlockType *retVal;
		Page *curPage;
		curPage=availablePages;
		retVal = (MemoryBlockType*) curPage->availableStack[--(curPage->availableStackSize)];
		if (curPage->availableStackSize==0)
		{
			--availablePagesSize;
			availablePages=curPage->next;
			RakAssert(availablePagesSize==0 || availablePages->availableStackSize>0);
			curPage->next->prev=curPage->prev;
			curPage->prev->next=curPage->next;

			if (unavailablePagesSize++==0)
			{
				unavailablePages=curPage;
				curPage->next=curPage;
				curPage->prev=curPage;	
			}
			else
			{
				curPage->next=unavailablePages;
				curPage->prev=unavailablePages->prev;
				unavailablePages->prev->next=curPage;
				unavailablePages->prev=curPage;
			}			
		}

		RakAssert(availablePagesSize==0 || availablePages->availableStackSize>0);
		return retVal;
	}

	availablePages = (Page*)malloc(sizeof(Page));
	if (availablePages==0)
		return 0;
	availablePagesSize=1;
	if (InitPage(availablePages, availablePages)==false)
		return 0;
	// If this assert hits, we couldn't allocate even 1 block per page. Increase the page size
	RakAssert(availablePages->availableStackSize>1);

	return (MemoryBlockType *) availablePages->availableStack[--availablePages->availableStackSize];
#endif
}

template<class MemoryBlockType>
void MemoryPoolModern<MemoryBlockType>::Release(MemoryBlockType *m)
{
#ifdef _DISABLE_MEMORY_POOL
	free(m);
	return;
#else
	// Find the page this block is in and return it.
	Page *curPage;
	MemoryWithPage *memoryWithPage = (MemoryWithPage*)m;
	curPage=memoryWithPage->parentPage;

	if (curPage->availableStackSize==0)
	{
		// The page is in the unavailable list so move it to the available list
		curPage->availableStack[curPage->availableStackSize++]=memoryWithPage;
		unavailablePagesSize--;

		// As this page is no longer totally empty, move it to the end of available pages
		curPage->next->prev=curPage->prev;
		curPage->prev->next=curPage->next;
		
		if (unavailablePagesSize>0 && curPage==unavailablePages)
			unavailablePages=unavailablePages->next;
		
		if (availablePagesSize++==0)
		{
			availablePages=curPage;
			curPage->next=curPage;
			curPage->prev=curPage;
		}
		else
		{
			curPage->next=availablePages;
			curPage->prev=availablePages->prev;
			availablePages->prev->next=curPage;
			availablePages->prev=curPage;	
		}
	}
	else
	{
		curPage->availableStack[curPage->availableStackSize++]=memoryWithPage;

		if (curPage->availableStackSize==BlocksPerPage() &&
			availablePagesSize>=DS_MEMORY_POOL_MAX_FREE_PAGES)
		{
			// After a certain point, just deallocate empty pages rather than keep them around
			if (curPage==availablePages)
			{
				availablePages=curPage->next;
				RakAssert(availablePages->availableStackSize>0);
			}
			curPage->prev->next=curPage->next;
			curPage->next->prev=curPage->prev;
			availablePagesSize--;
			free(curPage->availableStack);
			free(curPage->block);
			free(curPage);
		}
	}
#endif
}

template<class MemoryBlockType>
void MemoryPoolModern<MemoryBlockType>::Clear()
{
#ifdef _DISABLE_MEMORY_POOL
	return;
#else
	Page *cur, *freed;

	if (availablePagesSize>0)
	{
		cur = availablePages;
#ifdef _MSC_VER
#pragma warning(disable:4127)   // conditional expression is constant
#endif
		while (true) 
		// do
		{
			free(cur->availableStack);
			free(cur->block);
			freed=cur;
			cur=cur->next;
			if (cur==availablePages)
			{
				free(freed);
				break;
			}
			free(freed);
		}// while(cur!=availablePages);
	}
	
	if (unavailablePagesSize>0)
	{
		cur = unavailablePages;
		while (1)
		//do 
		{
			free(cur->availableStack);
			free(cur->block);
			freed=cur;
			cur=cur->next;
			if (cur==unavailablePages)
			{
				free(freed);
				break;
			}
			free(freed);
		} // while(cur!=unavailablePages);
	}

	availablePagesSize=0;
	unavailablePagesSize=0;
#endif
}

template<class MemoryBlockType>
int MemoryPoolModern<MemoryBlockType>::BlocksPerPage(void) const
{
	return memoryPoolPageSize / sizeof(MemoryWithPage);
}

template<class MemoryBlockType>
bool MemoryPoolModern<MemoryBlockType>::InitPage(Page *page, Page *prev)
{
	int i=0;
	const int bpp = BlocksPerPage();
	page->block=(MemoryWithPage*) malloc(memoryPoolPageSize);
	if (page->block==0)
		return false;
	page->availableStack=(MemoryWithPage**)malloc(sizeof(MemoryWithPage*)*bpp);
	if (page->availableStack==0)
	{
		free(page->block );
		return false;
	}
	MemoryWithPage *curBlock = page->block;
	MemoryWithPage **curStack = page->availableStack;
	while (i < bpp)
	{
		curBlock->parentPage=page;
		curStack[i]=curBlock++;
		i++;
	}
	page->availableStackSize=bpp;
	page->next=availablePages;
	page->prev=prev;
	return true;
}

#endif
