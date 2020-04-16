#include"PageCache.h"

Span* PageCache::NewSpan(size_t numpage){
	//����ֱ�ӷ���
	if (!_spanLists[numpage].Empty()){
		Span* span = _spanLists[numpage].Begin();
		_spanLists[numpage].PopFront();
		return span;
	}

	//��ǰ�ҽ��з���
	for (size_t i = numpage + 1; i < MAX_PAGES; ++i){
		if (!_spanLists[i].Empty()){

			Span* span = _spanLists[i].Begin();
			_spanLists[i].PopFront();

			Span* splitspan = new Span;
			splitspan->_pageid = span->_pageid + span->pagesize - numpage;
			splitspan->pagesize = numpage;
			for (PAGE_ID i = 0; i < numpage; i++){
				_idSpanMap[splitspan->_pageid + i] = splitspan;
			}

			span->pagesize -= numpage;
			_spanLists[span->pagesize].PushFront(span);
			
			return splitspan;
		}
	}

	//��ϵͳ����
	void* ptr = SystemAlloc(MAX_PAGES - 1);

	Span* bigspan = new Span;
	bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigspan->pagesize = MAX_PAGES - 1;

	for (PAGE_ID i = 0; i < bigspan->pagesize; i++){
		_idSpanMap[bigspan->_pageid + i] = bigspan;
	}

	_spanLists[bigspan->pagesize].PushFront(bigspan);
	return NewSpan(numpage);
}
Span* PageCache::GetIdToSpan(PAGE_ID id){
	auto it = _idSpanMap.find(id);
	if (it != _idSpanMap.end()){
		return it->second;
	}
	else{
		return nullptr;
	}
}
void PageCache::ReleaseSpanToPageCache(Span* span){
	//��ǰ�ϲ�
	while (1){
		PAGE_ID prevPageId = span->_pageid - 1;
		auto pit = _idSpanMap.find(prevPageId);
		//ǰ���ҳ������
		if (pit == _idSpanMap.end()){
			break;
		}

		//ǰһ��ҳ����ʹ���У����ܺϲ�
		Span* prevSpan = pit->second;
		if (prevSpan->_usecount != 0){
			break;
		}

		//�ϲ�
		span->_pageid = prevSpan->_pageid;
		span->pagesize += prevSpan->pagesize;
		for (PAGE_ID i = 0; i < prevSpan->pagesize; i++){
			_idSpanMap[prevSpan->_pageid+i] = span;
		}
		_spanLists[prevSpan->pagesize].Erase(prevSpan);
		delete prevSpan;
	}

	//���ϲ�
	while (1){
		PAGE_ID nextPageId = span->_pageid + span->pagesize;
		auto nit = _idSpanMap.find(nextPageId);
		if (nit == _idSpanMap.end()){
			break;
		}
		Span* nextSpan = nit->second;
		if (nextSpan->_usecount != 0){
			break;
		}

		span->pagesize += nextSpan->pagesize;
		for (PAGE_ID i = 0; i < nextSpan->pagesize; i++){
			_idSpanMap[nextSpan->_pageid + i] = span;
		}
		_spanLists[nextSpan->pagesize].Erase(nextSpan);
		delete nextSpan;
	}
	_spanLists[span->pagesize].PushFront(span);
}
