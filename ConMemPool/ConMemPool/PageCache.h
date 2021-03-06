#pragma once
#include"Common.h"


class PageCache{
public:
	Span* _NewSpan(size_t numpage);
	Span* NewSpan(size_t numpage);
	
	//向系统申请numpage页内存挂到自由链表
	void SystemAllocPage(size_t numpage);

	Span* GetIdToSpan(PAGE_ID id);
	void ReleaseSpanToPageCache(Span* span);//从centralcache释放回来

	static PageCache& GetPageCacheInstance()
	{
		return inst;
	}

private:
	SpanList _spanLists[MAX_PAGES];

private:
	PageCache() = default;
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;

	static PageCache inst;

	std::mutex _mtx;
	std::unordered_map<PAGE_ID, Span*>  _idSpanMap;
};

