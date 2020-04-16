#include"ThreadCache.h"

void* ThreadCache::Allocte(size_t size){
	size_t index = SizeClass::ListIndex(size);
	FreeList& freeList = _freeLists[index];
	if (!freeList.Empty()){
		return freeList.Pop();
	}
	else{
		return FetchFromCentralCache(SizeClass::RoundUp(size));
	}
}

void ThreadCache::Deallocte(void* ptr, size_t size){
	size_t index = SizeClass::ListIndex(size);//***
	FreeList& freeList = _freeLists[index];
	freeList.Push(ptr);

	//链表太长了就是释放回centralcache
	//对象个数满足一定条件
	size_t num = SizeClass::NumMoveSize(size);
	if (freeList.Num() >= num){
		
		ListTooLong(freeList,num,size);
	}

} 

void ThreadCache::ListTooLong(FreeList& freeList,size_t num,size_t size){
	void* start = nullptr, *end = nullptr;
	freeList.PopRange(start, end, num);
	NextObj(end) = nullptr;
	centralCacheInst.ReleaseListToSpans(start, size);
}

//独立测试thread cache
//void* ThreadCache::FetchFromCentralCache(size_t index){
//	//模拟实现取内存对象，测试ThreadCache的逻辑
//	size_t num= 20;//***
//	size_t size = (index+ 1)*8;//一个的大小
//
//	char* start = (char*)malloc(size*num);
//	char* cur = start;
//	for (size_t i = 0; i < num-1; i++){
//		char* next = cur + size;
//		NextObj(cur) = next;
//		cur = next;
//	}
//	NextObj(cur) = nullptr;
//	void* head = NextObj(start);
//	void* tail = cur;
//	_freeLists[index].PushRange(head,tail);
//
//	return start;
//}

void* ThreadCache::FetchFromCentralCache(size_t size){
	size_t num = SizeClass::NumMoveSize(size);//对齐（申请5字节，给8字节）
	void* start = nullptr, *end = nullptr;
	size_t actualNum=centralCacheInst.FetchRangeObj(start, end, num, size);

	if (actualNum == 1){
		return start;
	}
	else{
		size_t index = SizeClass::ListIndex(size);
		FreeList& list = _freeLists[index];
		list.PushRange(NextObj(start), end, actualNum-1);
		return start;
	}
} 
