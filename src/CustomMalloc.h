#include <stddef.h>
//#include "UnitTests.h"

void* CustomMalloc(size_t size);

int IncrementRefCount(void* ptr);

void CustomFree(void* ptr);
