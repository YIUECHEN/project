#pragma once
#include"Common.h"
#include"CentralCache.h"
class ThreadCache{
public:
	void* Allocte(size_t size);//被申请
	void Deallocte(void* ptr,size_t size);//释放换回来
	void* FetchFromCentralCache(size_t index);//从centralcache取内存
	void ListTooLong(FreeList& freeList, size_t num, size_t size); //自由链表中对象过长释放回centralcache

private:

	FreeList _freeLists[NFREE_LIST];

};

_declspec(thread) static ThreadCache* pThreadCache = nullptr;//_declspec (thread)表示属于每个线程独享的