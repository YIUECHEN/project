#pragma once
#include"Common.h"


class PageCache{
public:
	Span* _NewSpan(size_t numpage);
	Span* NewSpan(size_t numpage);
	
	//��ϵͳ����numpageҳ�ڴ�ҵ���������
	void SystemAllocPage(size_t numpage);

	Span* GetIdToSpan(PAGE_ID id);
	void ReleaseSpanToPageCache(Span* span);//��centralcache�ͷŻ���

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

