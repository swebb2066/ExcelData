#ifndef _LOGGER_HEADER_
#define _LOGGER_HEADER_

#include <Foundation/Config.h>

#ifdef _MSC_VER
#include <tchar.h>
#pragma warning (disable :  4275) // non dll-interface class 'std::_Container_base_aux' used as base for dll-interface class 'std::_Container_base_aux_alloc_real<_Alloc>'
#pragma warning (disable :  4251) // 'std::_Container_base_aux_alloc_real<_Alloc>::_Alaux' : class 'std::allocator<_Ty>' needs to have dll-interface to be used by clients of class 'std::_Container_base_aux_alloc_real<_Alloc>'
#else
#define _T(str) str
#endif

// Remove path names from executables
#include <log4cxx/log4cxx.h>
#if defined(LOG4CXX_VERSION_MINOR) && (0 < LOG4CXX_VERSION_MAJOR || 13 <= LOG4CXX_VERSION_MINOR)
#define LOG4CXX_DISABLE_LOCATION_INFO 1
#else
#undef LOG4CXX_LOCATION
#define LOG4CXX_LOCATION ::log4cxx::spi::LocationInfo()
#endif

#include <log4cxx/logger.h>
#include <log4cxx/logmanager.h>
#undef max
#undef min
#include <ostream>

/// Extensions to log4cxx
namespace log4cxx
{
class File;

/// Changes a logger level for the instance'os lifetime
class ScopedLevelChange
{
    LoggerPtr m_logger;
    LevelPtr m_savedLevel;
public: // ...structors
    /// Set \c loggerName to level \c level
    template <class StringType>
    ScopedLevelChange(const StringType& loggerName, const LevelPtr& level)
        : m_logger(Logger::getLogger(loggerName))
    {
        if (m_logger || !!(m_logger = LogManager::getLogger(loggerName)))
        {
            m_savedLevel = m_logger->getLevel();
            m_logger->setLevel(level);
        }
    }
    /// Set \c logger to \c level
    ScopedLevelChange(const LoggerPtr& logger, const LevelPtr& level)
        : m_logger(logger)
        , m_savedLevel(m_logger->getLevel())
    {
        m_logger->setLevel(level);
    }
    ~ScopedLevelChange()
    {
        if (m_logger)
            m_logger->setLevel(m_savedLevel);
    }
};

} // namespace log4cxx

#ifdef LOG4CXX_STR
// A string parameter type for version 0.10.0
typedef log4cxx::LogString LoggerStringType;
#else
// A string parameter type for version 0.9.7
typedef log4cxx::String LoggerStringType;
#endif
namespace boost { namespace filesystem { class path; } }

namespace Foundation
{

using LoggerPtr = log4cxx::LoggerPtr;

/// Retrieve the \c name logger pointer.
/// Configure Log4cxx on the first call.
LoggerPtr GetLogger(const std::string& name);

/// A .properties file with current process name in the current directory
/// or the same directory as this executable
bool GetPropertiesFile(log4cxx::File& propertyFile);

/// Initialise Log4cxx by reading the properties file
void InitialiseLogger(boost::filesystem::path* configFileUsed = 0);

typedef std::ostream& (*oStreamFunc)(std::ostream&);

/// Streamable array elements
template <typename T, typename S, typename D = T>
class SeparatedArray
{
    const T *m_vec;
    size_t m_len;
    S m_separator;
    size_t m_perLine;
public:
    SeparatedArray(const T *vec, size_t len, S separ, size_t perLine = 10)
        : m_vec(vec)
        , m_len(len)
        , m_separator(separ)
        , m_perLine(perLine)
    {}
    void Write(std::ostream& os) const
    {
        if (0 < m_perLine && m_perLine <= m_len)
            os << std::endl;
        for (size_t i = 0; i < m_len; ++i)
        {
            if (0 < i)
            {
                if (0 < m_perLine && 0 == (i % m_perLine))
                    os << std::endl;
                else
                    os << m_separator;
            }
            os << (D)m_vec[i];
        }
    }
};

/// Put \c S separated type \c D elements of \c v onto \c os
    template <typename T, typename S, typename D>
    std::ostream&
 operator<<(std::ostream& os, const SeparatedArray<T, S, D>& v)
{ v.Write(os); return os;  }

/// Streamable comma separated array elements
    template <typename T>
class CommaSeparatedArray : public SeparatedArray<T, char>
{
public:
    CommaSeparatedArray(const T *vec, size_t len, size_t perLine = 10)
        : SeparatedArray<T,char>(vec, len, ',', perLine)
    {}
};

/// Put comma separated elements of \c v onto \c os
    template <typename T>
    std::ostream&
operator<<(std::ostream& os, const CommaSeparatedArray<T>& v)
{ v.Write(os); return os;  }

/// Streamable space separated unsigned int array elements
    template <typename T>
class SpaceSeparatedUnsignedArray : public SeparatedArray<T, char, unsigned int>
{
    typedef SeparatedArray<T, char, unsigned int> BaseType;
public:
    SpaceSeparatedUnsignedArray(const T *vec, size_t len, size_t perLine = 10)
        : BaseType(vec, len, ' ', perLine)
    {}
};

/// Put space separated unsigned values of \c v onto \c os
    template <typename T>
    std::ostream&
operator<<(std::ostream& os, const SpaceSeparatedUnsignedArray<T>& v)
{ v.Write(os); return os;  }

/// Streamable newline separated array elements
    template <typename T>
class MultiLineArray : public SeparatedArray<T, oStreamFunc>
{
public:
    MultiLineArray(const T *vec, int len)
        : SeparatedArray<T, oStreamFunc>(vec, len, std::endl, 0)
    {}
};

/// Put newline separated elements of \c v onto \c os
    template <typename T>
std::ostream& operator<<(std::ostream& os, const MultiLineArray<T>& v)
{ v.Write(os); return os;  }

/// Streamable comma separated iterator range
    template <typename T>
class CommaSeparatedItems
{
    T m_begin;
    T m_end;
    size_t m_perLine;
public:
    CommaSeparatedItems(T& begin, T& end, size_t perLine = 10)
        : m_begin(begin)
        , m_end(end)
        , m_perLine(perLine)
    {}
    void Write(std::ostream& os) const
    {
        if (0 < m_perLine && m_perLine <= size_t(m_end - m_begin))
            os << std::endl;
        T iter = m_begin;
        for (int i = 0; iter != m_end; ++iter, ++i)
        {
            if (0 < i)
            {
                if (0 < m_perLine && 0 == (i % m_perLine))
                    os << std::endl;
                else
                    os << ',';
            }
            os << *iter;
        }
    }
};

/// Put comma separated elements of \c v onto \c os
    template <typename T>
    std::ostream&
operator<<(std::ostream& os, const CommaSeparatedItems<T>& v)
{ v.Write(os); return os;  }

} // namespace Foundation

#endif // _LOGGER_HEADER_
