#pragma once

#include<iostream>
#include<assert.h>
#include<thread>
#include<mutex>
#include<map>

#ifdef _WIN32
#include <windows.h>
#endif//WIIN32

//using namespace std;
using std::endl;
using std::cout;

const size_t MAX_SIZE = 64 * 1024;//64k
const size_t NFREE_LIST = MAX_SIZE / 8;//***
const size_t MAX_PAGES = 129;
const size_t PAGE_SHIFT = 12; // 4k为页移位

inline void*& NextObj(void* obj){
	return *(void**)obj;
}

class FreeList{
public:

	void Push(void* obj){//头插，释放回来
		NextObj(obj)=_freelist;
		_freelist = obj;	
		++_num;
	}

	void PushRange(void* head,void* tail,size_t num){
		NextObj(tail) = _freelist;
		_freelist = head;
		_num += num;
	}


	void* Pop(){//头删，被取走
		void* obj = _freelist;
		_freelist = NextObj(obj);
		--_num;
		return obj;
	}

	size_t PopRange(void*& start, void*& end, size_t num){
		size_t actualNum = 0;
		void* prev = nullptr;
		void* cur = _freelist;
		for (; actualNum < num&&cur != nullptr; actualNum++){
			prev = cur;
			cur = NextObj(cur);
		}

		start = _freelist;
		end = prev;
		_freelist = cur;
		_num -= actualNum;
		return actualNum;
	}


	bool Empty(){
		return _freelist == nullptr;
	
	}

	size_t Num(){
		return _num;
	}

	void Clear(){
		 _freelist = nullptr;
		 _num = 0;
	}
private:
	void* _freelist=nullptr;
	size_t _num = 0;

};

class SizeClass{
public:
	static size_t ListIndex(size_t size){
		assert(size <= MAX_SIZE);

		//每个区间上有多少个链
		static int group_array[4] = { 16, 56, 56, 112 };

		if (size <= 128){
			return _ListIndex(size, 3);
		}
		else if (size <= 1024){
			return _ListIndex(size - 128, 4) + group_array[0];
		}

		else if (size <= 8192){
			return _ListIndex(size - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (size <= 65536){
			return _ListIndex(size - 8192, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		return -1;
	}

	static size_t _ListIndex(size_t size, size_t align_shift){
		/*if (size % 8 == 0){
			return size / 8 - 1;
		} 
		else{
			return size / 8;
		}*/
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	
	static inline size_t RoundUp(size_t size)
	{
		assert(size <= MAX_SIZE);

		if (size <= 128){
			return _RoundUp(size, 8);
		}
		else if (size <= 1024){
			return _RoundUp(size, 16);
		}
		else if (size <= 8192){
			return _RoundUp(size, 128);
		}
		else if (size <= 65536){
			return _RoundUp(size, 1024);
		}

		return -1;
	}

	//控制有[1%,10%]左右的内碎片
	// [1,128] 8byte对齐 freelist[0,16)
	// [129,1024] 16byte对齐 freelist[16,72)
	// [1025,8*1024] 128byte对齐 freelist[72,128)
	// [8*1024+1,64*1024] 1024byte对齐 freelist[128,184)
	//向上对齐
	static size_t _RoundUp(size_t size, size_t align){
		/*if (size % 8 != 0){
			return (size / 8 + 1)*8;
			}
			else{
			return size;
			}*/

		//[9,16]+7=[16,23]->16 8 4 2 1  &后，低位变为0
		return (size + align - 1)&(~(align - 1));
	}

	static size_t NumMoveSize(size_t size)
	{
		if (size == 0)
			return 0;
		int num = MAX_SIZE / size;
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}


	static size_t NumMovePage(size_t size){
		size_t num = NumMoveSize(size);
		size_t npage = num*size;
		npage >>= 12;
		if (npage == 0){
			return 1;
		}
		return npage;
	}

};

//span跨度 管理页为单位的内存对象，本质是方便做合并，解决内存碎片

//针对window
#ifdef _WIN32
typedef unsigned int PAGE_ID;
#else
typedef unsigned long long PAGE_ID
#endif

struct Span{
	PAGE_ID _pageid=0;//页号(有2^52个页号)
	PAGE_ID pagesize = 0;//页的数量

	FreeList _freeList;//对象的自由自由链表
	int _usecount=0;//内存块对象使用计数
	size_t objsize=0;//自由链表对象大小
	Span* _next;
	Span* _prev;
};

class SpanList{
public:
	SpanList(){
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}
	
	Span* Begin(){
		return _head->_next;
	}

	Span* End(){
		return _head;
	}

	void PushFront(Span* newspan){
		Insert(_head->_next, newspan);
	}

	void PopFront(){
		Erase(_head->_next);
	}

	void PushBack(Span* newspan){
		Insert(_head, newspan);
	}

	void PopBack(){
		Erase(_head->_prev);
	}


	void Insert(Span* pos,Span* newspan){
		Span*  prev = pos->_prev;
		prev->_next = newspan;
		newspan->_next = pos;
		pos->_prev = newspan;
		newspan->_prev = prev;
	}

	void Erase(Span* pos){
		assert (pos != _head);
		Span *prev = pos->_prev;
		Span *next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}

	bool Empty(){
		return Begin() == End();
	}

	void Lock(){
		_mtx.lock();
	}
	void Unlock(){
		_mtx.unlock();
	}

private:
	Span* _head;
	std::mutex _mtx;
};
inline static void* SystemAlloc(size_t numpage){
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, numpage*(1 << PAGE_SHIFT),
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	// brk mmap等
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}


inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
#endif
}