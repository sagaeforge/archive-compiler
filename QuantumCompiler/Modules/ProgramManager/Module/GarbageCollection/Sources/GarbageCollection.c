
#include "GarbageCollection.h"
#include "Private_GarbageCollection.h"
#include "ProgramManager.h"

void GarbageCollectionModule_Initialized() {
  // clang-format off
  Application.GarbageCollection.Compare       = MemoryCompare;
  Application.GarbageCollection.ConstCreate   = MemoryConstCreate;
  Application.GarbageCollection.Copy          = MemoryCopy;
  Application.GarbageCollection.Create        = MemoryCreate;
  Application.GarbageCollection.Info          = Info;
  Application.GarbageCollection.Length        = MemoryLength;
  Application.GarbageCollection.Move          = MemoryMove;
  Application.GarbageCollection.Policy        = Policy;
  Application.GarbageCollection.PolicyAppend  = Policy_Append;
  Application.GarbageCollection.PolicyRemove  = Policy_Remove;
  Application.GarbageCollection.Remove        = MemoryRemove;
  Application.GarbageCollection.Set           = MemorySet;
  Application.GarbageCollection.Swap          = MemorySwap;
  
  Application.Member.GarbageCollection_UsedMemoryLength = 0;
  Application.Member.GarbageCollection_UsedMemoryPageLength = 1;
  MemorySet(&Application.Member.GarbageCollection_Pages.Datas, 
            0,
            1,
            sizeof(Application.Member.GarbageCollection_Pages.Datas[0]) * MemoryMaxLength);
  // clang-format on
  MemoryInfo info;
  info.IsFound = false;
  info.Memory.Length = sizeof(Application.Member.GarbageCollection_Pages);
  info.Memory.Policy = MemoryPolicy_None;
  info.Memory.Position = &Application.Member.GarbageCollection_Pages;
  info.Position.MemoryIndex = 0;
  info.Position.PageIndex = 0;

  Application.Member.GarbageCollection_Pages.Info = info;
  Application.Member.GarbageCollection_Pages.Next = NULL;
  Application.Member.GarbageCollection_Pages.UsedMemoryLength = 0;
}