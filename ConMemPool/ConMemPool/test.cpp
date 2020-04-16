#include"ThreadCache.h"
#include<vector>
//void UnitThreadCache(){
//	ThreadCache tc;
//	vector<void*> v;
//
//	for (int i = 0; i < 21; i++){//链表一开始是空的，第一次先从central获取，十九次从链表获取，链表空了以后，再从central获取一次（白盒测试）
//		v.push_back(tc.Allocte(7));
//	}
//
//	for (int i = 0; i < 21; i++){
//		printf("[%d]--->%p\n",i,v[i]);
//	}
//
//	for (auto ptr:v){
//		tc.Deallocte(ptr, 7);//释放申请出来的内存
//	
//	}
//}

void UnitTestSizeClass(){
	cout << SizeClass::RoundUp(1) << endl;
	cout << SizeClass::RoundUp(127) << endl;
	cout<<endl;

	cout << SizeClass::RoundUp(129) << endl;
	cout << SizeClass::RoundUp(1023) << endl;
	cout << endl;

	cout << SizeClass::RoundUp(1025) << endl;
	cout << SizeClass::RoundUp(8*1024-1) << endl;
	cout << endl;

}

void UnitTestSystemAlloc()
{
	void* ptr = SystemAlloc(MAX_PAGES - 1);
	PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	void* ptrshift = (void*)(id << PAGE_SHIFT);

	char* obj1 = (char*)ptr;
	char* obj2 = (char*)ptr + 8;
	char* obj3 = (char*)ptr + 16;
	PAGE_ID id1 = (PAGE_ID)obj1 >> PAGE_SHIFT;
	PAGE_ID id2 = (PAGE_ID)obj2 >> PAGE_SHIFT;
	PAGE_ID id3 = (PAGE_ID)obj3 >> PAGE_SHIFT;
}

void UnitThreadCache()
{
	ThreadCache tc;
	std::vector<void*> v;
	size_t size =5;
	size_t num = SizeClass::NumMoveSize(size);
	for (size_t i = 0; i < num; ++i)
	{
		v.push_back(tc.Allocte(size));
	}

	v.push_back(tc.Allocte(size));

	for (size_t i = 0; i < v.size(); ++i)
	{
		printf("[%d]->%p\n", i, v[i]);
	}

	for (auto ptr : v)
	{
		tc.Deallocte(ptr, 7);
	}

	v.clear();

	v.push_back(tc.Allocte(size));//从头申请
}


#include "ConcurrentMalloc.h"

void func1(size_t n)
{
	std::vector<void*> v;
	size_t size = 7;
	for (size_t i = 0; i < SizeClass::NumMoveSize(size) + 1; ++i)
	{
		v.push_back(ConcurrentMalloc(size));
	}

	for (size_t i = 0; i < v.size(); ++i)
	{
		//printf("[%d]->%p\n", i, v[i]);
	}

	for (auto ptr : v)
	{
		ConcurrentFree(ptr);
	}

	v.clear();
}

void func2(size_t n)
{
	std::vector<void*> v;
	size_t size = 7;
	for (size_t i = 0; i < SizeClass::NumMoveSize(size) + 1; ++i)
	{
		v.push_back(ConcurrentMalloc(size));
	}

	for (size_t i = 0; i < v.size(); ++i)
	{
		//	printf("[%d]->%p\n", i, v[i]);
	}

	for (auto ptr : v)
	{
		ConcurrentFree(ptr);
	}

	v.clear();
}


int main(){
	//UnitThreadCache();
	//UnitTestSizeClass();
    //UnitTestSystemAlloc();


	void* ptr1 = ConcurrentMalloc(1 << PAGE_SHIFT);
	void* ptr2 = ConcurrentMalloc(65 << PAGE_SHIFT);
	void* ptr3 = ConcurrentMalloc(129 << PAGE_SHIFT);

	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);
	ConcurrentFree(ptr3);

	system("pause");
	return 0;
}