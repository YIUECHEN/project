#pragma once
#include"ThreadCache.h"

void* ConcurrentMalloc(size_t size){
	if (size < MAX_SIZE){
		if (pThreadCache == nullptr){
			
			pThreadCache = new ThreadCache;
			//cout << std::this_thread::get_id() << "->" << pThreadCache << endl;

		}
		return pThreadCache->Allocte(size);

	}
	else if (size<=(MAX_PAGES-1)<<PAGE_SHIFT){
		size_t aligent_size = SizeClass::_RoundUp(size, 1<<PAGE_SHIFT);
		size_t numpage = aligent_size >> PAGE_SHIFT;
		Span* span = PageCache::GetPageCacheInstance().NewSpan(numpage);
		span->objsize = aligent_size;
		void* ptr = (void*)(span->_pageid << PAGE_SHIFT);
		return ptr;
	}
	else{
		size_t aligent_size = SizeClass::_RoundUp(size, 1 << PAGE_SHIFT);
		size_t numpage = aligent_size >> PAGE_SHIFT;
		return SystemAlloc(numpage);
	}
}

void ConcurrentFree(void* ptr){
	size_t pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetPageCacheInstance().GetIdToSpan(pageid);

	if (span == nullptr){
		SystemFree(ptr);
		return;
	}
	size_t size = span->objsize;

	if (size <= MAX_SIZE){
		pThreadCache->Deallocte(ptr, size);
	}
	else if(size<=((MAX_PAGES-1)<<PAGE_SHIFT)){
		PageCache::GetPageCacheInstance().ReleaseSpanToPageCache(span);
	}
}