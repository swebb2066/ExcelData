#ifndef FOUNDATION__MEMORY_H_
#define FOUNDATION__MEMORY_H_

#include <Foundation/Config.h>
#include <Foundation/Logger.h>

/// Count the number of bytes of used memory in the heap
size_t HeapUsed(void);

/// A message that describes the sanity of the heap structure
const char *HeapStatusMessage(void);

class HeapChangeLogger
{
protected: // Attributes
    log4cxx::LoggerPtr m_log;
    size_t m_used;

public: // ...structors
    HeapChangeLogger(log4cxx::LoggerPtr log);

public: // operators
    void operator()(const char* stage);
};

#endif // FOUNDATION__MEMORY_H_
