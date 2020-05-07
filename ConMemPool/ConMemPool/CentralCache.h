#pragma once
#include"Common.h"
#include"PageCache.h"

class CentralCache{
public:

	static CentralCache* GetInstance()
	{
		return &centralCacheInst;
	}

	//�����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);
 
	//��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t size);

	//��page cache��ȡһ��span
	Span* GetOneSpan(size_t size);

private:
	//���캯��Ĭ�ϻ���Ҳ�����޲�������
	CentralCache() = default;
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;
	static CentralCache centralCacheInst;//���̹���
private:
	SpanList _spanLists[NFREE_LIST];
};

