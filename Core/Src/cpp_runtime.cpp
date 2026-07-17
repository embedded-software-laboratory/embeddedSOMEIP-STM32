// Routes C++ allocation through the FreeRTOS heap instead of the newlib _sbrk heap.
// Gives one thread-safe heap sized by configTOTAL_HEAP_SIZE.
#include <cstddef>

#include "FreeRTOS.h"
#include "task.h"

// Built with -fno-exceptions, so halt on exhaustion instead of throwing bad_alloc.
void *operator new(std::size_t size)
{
  void *p = pvPortMalloc(size ? size : 1u);
  configASSERT(p != nullptr);
  return p;
}

void *operator new[](std::size_t size)
{
  return ::operator new(size);
}

void operator delete(void *p) noexcept
{
  if (p != nullptr)
  {
    vPortFree(p);
  }
}

void operator delete[](void *p) noexcept
{
  ::operator delete(p);
}

// C++14 sized-deallocation forms, the compiler may emit calls to these
void operator delete(void *p, std::size_t) noexcept
{
  ::operator delete(p);
}

void operator delete[](void *p, std::size_t) noexcept
{
  ::operator delete(p);
}

// Called on a pure virtual call, must not return
extern "C" void __cxa_pure_virtual()
{
  configASSERT(0);
  for (;;)
  {
  }
}
