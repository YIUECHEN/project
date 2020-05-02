#include"CentralCache.h"
#include"PageCache.h"

//从page cache获取一个span
Span* CentralCache::GetOneSpan(size_t size){
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	Span* it = spanlist.Begin();
	while (it != spanlist.End()){
		if (!it->_freeList.Empty()){
			return it;
		}
		else{
			it = it->_next;
		}
	}

	//page cache获取一个span
	size_t numpage = SizeClass::NumMovePage(size);//求出需要申请的页数
	Span* span = PageCache::GetPageCacheInstance().NewSpan(numpage);

	//把申请来的span切成对应大小挂到span的freelist中
	char* start = (char*)(span->_pageid << 12);
	char* end = start + (span->pagesize << 12);
	while (start < end){
		char* obj = start;
		start += size;
		span->_freeList.Push(obj);
	}
	span->objsize = size;
	spanlist.PushBack(span);//讲切好的span挂到centralcache中的相应位置
	return span;
}

size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t size){
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();

	Span* span = GetOneSpan(size);
	FreeList& freelist = span->_freeList;
	size_t actualNum = freelist.PopRange(start, end, num);
	span->_usecount += actualNum;

	spanlist.Unlock();


	return actualNum;
	
}


//将一定数量的对象释放到span跨度
void CentralCache::ReleaseListToSpans(void* start, size_t size){
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();
	while (start){
		void* next = NextObj(start);
		PAGE_ID id = (PAGE_ID)start >> PAGE_SHIFT;
		Span* span = PageCache::GetPageCacheInstance().GetIdToSpan(id);
		span->_freeList.Push(start);
		span->_usecount--;

		//表示当前span切出去的对象都还回来了，可以将span还给pagecache，进行合并，减少内存碎片。
		if (span->_usecount == 0){
			size_t index = SizeClass::ListIndex(span->objsize);
			_spanLists->Erase(span);
			span->_freeList.Clear();

			PageCache::GetPageCacheInstance().ReleaseSpanToPageCache(span);
		}
		start = next;
	}
	spanlist.Unlock();
}

