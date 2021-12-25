
#include "GarbageCollection.h"
#include "Exception.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"
#include <stdlib.h>

static MemoryPage *MemoryPage_Page();
MemoryPage *MemoryPage_Get(Index Index);
MemoryPage *MemoryPage_GetEmpty();

void Clear() {
  MemoryPage *page = &Manager.GarbageCollection.Pages;

  while (page != NULL) {
    int i;
    for (i = 0; i < page->UsedMemoryLength; i++) {
      free(page->Datas[i].Value);
      page->Datas[i].Value = NULL;
      page->Datas[i].Length = 0;
      page->Datas[i].Policey = MemoryPolicey_None;
    }

    page = page->Next;
  }
}
void *Memory(MemoryPosition Position) {
  MemoryPage *page = MemoryPage_Get(Position.PageIndex);
  return page->Datas[Position.MemoryIndex].Value;
}

MemoryInfo Info(void *Obj) {
  MemoryPage *page = &Manager.GarbageCollection.Pages;
  MemoryInfo ret = {
      0,
  };

  if (Obj == NULL)
    return ret;

  int i;
  for (i = 0; page != NULL; i++) {
    int pl = 0;
    int pr = page->UsedMemoryLength;
    int pc = 0;

    do {
      pc = (pl + pr) / 2;

      if (page->Datas[pc].Value == Obj) {
        ret.IsFounded = true;
        ret.Length = page->Datas[pc].Length;
        ret.Policy = page->Datas[pc].Policey;
        ret.Position.MemoryIndex = pc;
        ret.Position.PageIndex = i;
        ret.Value = Obj;
        return ret;
      } else if (page->Datas[pc].Value < Obj)
        pl = pc + 1;
      else
        pr = pc - 1;

    } while (pl <= pr);
    page = page->Next;
  }
  return ret;
}

static MemoryPage *MemoryPage_Page() {
  MemoryPage *ptr = malloc(sizeof(MemoryPage));
  if (ptr == NULL) {
    Warning("메모리 페이지를 생성할 수 없습니다.");
    return NULL;
  }

  ptr->Next = NULL;
  ptr->UsedMemoryLength = 0;
  int i;
  for (i = 0; i < MemoryMaxLength; i++) {
    ptr->Datas[i].Value = NULL;
    ptr->Datas[i].Length = 0;
  }

  Manager.GarbageCollection.UsedMemoryPageLength++;
  return ptr;
}

MemoryPage *MemoryPage_Get(Index Index) {
  if (Manager.GarbageCollection.UsedMemoryPageLength < Index)
    return NULL;

  MemoryPage *page = &Manager.GarbageCollection.Pages;
  int i = 0;
  while (i < Index) {
    page = page->Next;
    i++;
  }
  return page;
}
MemoryPage *MemoryPage_GetEmpty() {
  MemoryPage *page = &Manager.GarbageCollection.Pages;
  int i = 0;
  while (page != NULL) {
    if (page->UsedMemoryLength < MemoryMaxLength)
      return page;
    page = page->Next;
  }

  page = &Manager.GarbageCollection.Pages;
  while (page->Next != NULL)
    page = page->Next;

  page->Next = MemoryPage_Page();
  return page->Next;
}

void GC_Append(void *ptr, Length Length) {
  MemoryPage *page = MemoryPage_GetEmpty();

  page->Datas[page->UsedMemoryLength].Value = ptr;
  page->Datas[page->UsedMemoryLength].Length = Length;
  page->Datas[page->UsedMemoryLength].Policey = MemoryPolicey_None;
  page->UsedMemoryLength++;
  Manager.GarbageCollection.UsedMemoryLength++;
}

void GC_Remove(void *ptr) {
  MemoryInfo info = Info(ptr);
  if (!info.IsFounded)
    return;

  MemoryPage *page = MemoryPage_Get(info.Position.PageIndex);
  page->Datas[info.Position.MemoryIndex].Value = NULL;
  page->Datas[info.Position.MemoryIndex].Length = 0;

  page->UsedMemoryLength--;
  Manager.GarbageCollection.UsedMemoryLength--;

  int i;
  for (i = info.Position.MemoryIndex; i < page->UsedMemoryLength; i++) {
    MemorySwap(&page->Datas[i], &page->Datas[i + 1], sizeof(page->Datas[i]));
  }
}

bool GC_CreateCheck(void *Obj1, void *Obj2) {
  MemoryInfo obj1_info = Manager.GarbageCollection.Method.Info(Obj1);
  MemoryInfo obj2_info = Manager.GarbageCollection.Method.Info(Obj2);

  if (!obj1_info.IsFounded || !obj2_info.IsFounded) {
    if (obj1_info.IsFounded)
      Warning("GC에서 생성된 메모리가 아닙니다. --> %p", obj1_info.Value);
    else
      Warning("GC에서 생성된 메모리가 아닙니다. --> %p", obj2_info.Value);
    return true;
  }
  return false;
}

bool Policey(void *Obj, MemoryPolicey Policey) {
  MemoryInfo info = Manager.GarbageCollection.Method.Info(Obj);
  return info.Policy & Policey;
}

void Policey_Append(void *Obj, MemoryPolicey Policey) {
  MemoryInfo info = Manager.GarbageCollection.Method.Info(Obj);
  MemoryPage *page = MemoryPage_Get(info.Position.PageIndex);
  page->Datas[info.Position.MemoryIndex].Policey |= Policey;
}

void Policey_Remove(void *Obj, MemoryPolicey Policey) {
  MemoryInfo info = Manager.GarbageCollection.Method.Info(Obj);
  MemoryPage *page = MemoryPage_Get(info.Position.PageIndex);
  page->Datas[info.Position.MemoryIndex].Policey &= ~Policey;
}

bool GC_IndexOfExceptionCheck(void *Obj, Length Length) {
  MemoryInfo info = Manager.GarbageCollection.Method.Info(Obj);
  if (info.Length < Length)
    return true;
  return false;
}
void *MemoryConstCreate(Length Length) {
  void *ptr = Manager.GarbageCollection.Method.MemoryCreate(Length);
  if (ptr == NULL)
    return NULL;
  Manager.GarbageCollection.Method.PolicyAppend(ptr, MemoryPolicey_Const);
  return ptr;
}