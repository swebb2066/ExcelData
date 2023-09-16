#include <Foundation/Memory.h>

#ifdef _MSC_VER
#include <windows.h>
#include <malloc.h>

namespace {

    const char* HeapStatusString(int heapstatus)
    {
        const char* heapStatusMsg;
        switch (heapstatus)
        {
        default:
            heapStatusMsg = "Unknown status error!";
            break;
        case _HEAPEMPTY:
            heapStatusMsg = "OK - empty heap";
            break;
        case _HEAPEND:
            heapStatusMsg = "OK";
            break;
        case _HEAPBADPTR:
            heapStatusMsg = "ERROR - bad pointer to heap";
            break;
        case _HEAPBADBEGIN:
            heapStatusMsg = "ERROR - bad start of heap";
            break;
        case _HEAPBADNODE:
            heapStatusMsg = "ERROR - bad node in heap";
            break;
        }
        return heapStatusMsg;
    }
}

size_t HeapUsed()
{
   size_t heapUsed = 0;
   if (!HeapValidate((PVOID)_get_heap_handle(), 0, 0))
       throw std::runtime_error("Heap memory is inconsistent");
   else
   {
       HeapLock((PVOID)_get_heap_handle());
       _HEAPINFO hinfo;
       int heapstatus;
       hinfo._pentry = NULL;
       while ((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
       {
           if (hinfo._useflag == _USEDENTRY)
               heapUsed += hinfo._size;
       }
       HeapUnlock((PVOID)_get_heap_handle());
       if (_HEAPEND != heapstatus)
           throw std::runtime_error(HeapStatusString(heapstatus));
   }
   return heapUsed;
}

const char* HeapStatusMessage()
{
    _HEAPINFO hinfo;
    int heapstatus;
    hinfo._pentry = NULL;
    HeapLock((PVOID)_get_heap_handle());
    while ((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
        ;
    HeapUnlock((PVOID)_get_heap_handle());
    return HeapStatusString(heapstatus);
}

HeapChangeLogger::HeapChangeLogger(log4cxx::LoggerPtr log)
    : m_log(log)
    , m_used(0)
{
    if (m_log->isInfoEnabled())
        m_used = HeapUsed();
}

    void
HeapChangeLogger::operator()(const char* stage)
{
    if (m_log->isInfoEnabled())
    {
        size_t newUsed = HeapUsed();
        LOG4CXX_INFO(m_log, "HeapChange: " << int(newUsed - m_used) << " at " <<  stage);
        m_used = newUsed;
    }
}
#else

size_t HeapUsed()
{
    return 0;
}

const char *HeapStatusMessage()
{
    return "Heap status not implemented";
}

HeapChangeLogger::HeapChangeLogger(log4cxx::LoggerPtr log)
    : m_log(log)
    , m_used(0)
{
}

    void
HeapChangeLogger::operator()(const char* stage)
{
}

#endif
