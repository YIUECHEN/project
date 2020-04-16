#include"CentralCache.h"
#include"PageCache.h"

//��page cache��ȡһ��span
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

	//page cache��ȡһ��span
	size_t numpage = SizeClass::NumMovePage(size);//�����Ҫ�����ҳ��
	Span* span = pageCacheInst.NewSpan(numpage); 

	//����������span�гɶ�Ӧ��С�ҵ�span��freelist��
	char* start = (char*)(span->_pageid << 12);
	char* end = start + (span->pagesize << 12);
	while (start < end){
		char* obj = start;
		start += size;
		span->_freeList.Push(obj);
	}
	span->objsize = size;
	spanlist.PushBack(span);//���кõ�span�ҵ�centralcache�е���Ӧλ��
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


//��һ�������Ķ����ͷŵ�span���
void CentralCache::ReleaseListToSpans(void* start, size_t size){
	size_t index = SizeClass::ListIndex(size);
	SpanList& spanlist = _spanLists[index];
	spanlist.Lock();
	while (start){
		void* next = NextObj(start);
		PAGE_ID id = (PAGE_ID)start >> PAGE_SHIFT;
		Span* span = pageCacheInst.GetIdToSpan(id);
		span->_freeList.Push(start);
		span->_usecount--;

		//��ʾ��ǰspan�г�ȥ�Ķ��󶼻������ˣ����Խ�span����pagecache�����кϲ��������ڴ���Ƭ��
		if (span->_usecount == 0){
			size_t index = SizeClass::ListIndex(span->objsize);
			_spanLists->Erase(span);
			span->_freeList.Clear();

			pageCacheInst.ReleaseSpanToPageCache(span);
		}
		start = next;
	}
	spanlist.Unlock();
}

