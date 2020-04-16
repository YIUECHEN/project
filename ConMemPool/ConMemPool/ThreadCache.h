#pragma once
#include"Common.h"
#include"CentralCache.h"
class ThreadCache{
public:
	void* Allocte(size_t size);//������
	void Deallocte(void* ptr,size_t size);//�ͷŻ�����
	void* FetchFromCentralCache(size_t index);//��centralcacheȡ�ڴ�
	void ListTooLong(FreeList& freeList, size_t num, size_t size); //���������ж�������ͷŻ�centralcache

private:

	FreeList _freeLists[NFREE_LIST];

};

_declspec(thread) static ThreadCache* pThreadCache = nullptr;//_declspec (thread)��ʾ����ÿ���̶߳����