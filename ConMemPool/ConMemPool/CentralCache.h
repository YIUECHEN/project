#pragma once
#include"Common.h"
#include"PageCache.h"

class CentralCache{
public:

	static CentralCache* GetInstance()
	{
		return &centralCacheInst;
	}

	//从中心缓存获取一定数量的对象给thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);
 
	//将一定数量的对象释放到span跨度
	void ReleaseListToSpans(void* start, size_t size);

	//从page cache获取一个span
	Span* GetOneSpan(size_t size);

private:
	//构造函数默认化，也就是无参无内容
	CentralCache() = default;
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;
	static CentralCache centralCacheInst;//进程共享
private:
	SpanList _spanLists[NFREE_LIST];
};

