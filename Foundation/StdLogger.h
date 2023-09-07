#ifndef FOUNDATION_STD_LOGGER_HDR_
#define FOUNDATION_STD_LOGGER_HDR_

#include <Foundation/Logger.h>

namespace Foundation
{

/// Streamable comma separated array elements of a std::vector
    template <typename T>
class CommaSeparatedVector : public SeparatedArray<T, char>
{
    typedef SeparatedArray<T, char> BaseType;
public:
    CommaSeparatedVector(const std::vector<T>& vec, size_t perLine = 10)
        : BaseType(vec.empty() ? 0 : &vec[0], vec.size(), ',', perLine)
    {}
};

/// Streamable comma separated array elements of a std::vector<bool>
    template <>
class CommaSeparatedVector<bool>
{
    const std::vector<bool>& m_vec;
    size_t m_perLine;
public:
    CommaSeparatedVector(const std::vector<bool>& vec, size_t perLine = 10)
        : m_vec(vec)
        , m_perLine(perLine)
    {}
    void Write(std::ostream& os) const
    {
        if (0 < m_perLine && m_perLine <= m_vec.size())
            os << std::endl;
        for (size_t i = 0; i < m_vec.size(); ++i)
        {
            if (0 < i)
            {
                if (0 < m_perLine && 0 == (i % m_perLine))
                    os << std::endl;
                else
                    os << ',';
            }
            os << m_vec[i];
        }
    }
};

/// Put a comma separated list of the elements of \c vec onto the stream \c os
    template <typename T>
    typename std::ostream& 
operator<<(typename std::ostream& os, const CommaSeparatedVector<T>& vec)
{ vec.Write(os); return os;  }

/// Provides a textual representation of a std::map
    template <typename K, typename T>
class CommaSeparatedMap
{
    const std::map<K,T>& m_data;
    size_t m_perLine;
public:
    CommaSeparatedMap(const typename std::map<K,T>& data, size_t perLine = 10)
        : m_data(data)
        , m_perLine(perLine)
    {}
    void Write(typename std::ostream& os) const
    {
        if (0 < m_perLine && m_perLine <= m_data.size())
            os << std::endl;
        typename std::map<K,T>::const_iterator iter = m_data.begin();
        for (int i = 0; iter !=  m_data.end(); ++iter, ++i)
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

/// Put a comma separated list of the elements of \c m onto the stream \c os
    template <typename K, typename T>
    typename std::ostream& 
operator<<(typename std::ostream& os, const CommaSeparatedMap<K,T>& m)
{ m.Write(os); return os; }

} // namespace Foundation

/// Support for sending vector, map and pair to a std::ostream
namespace std
{

/// Put a comma separated list of the elements of \c vec onto the stream \c os
    template <typename T>
    ostream&
operator<<(ostream& os, const vector<T>& vec)
{ os << Foundation::CommaSeparatedVector<T>(vec); return os; }

/// Put a colan separated pair of the elements of \c data onto the stream \c os
    template <typename A, typename B>
    ostream&
operator<<(ostream& os, const pair<A, B>& data)
{ os << data.first << ':' << data.second; return os; }

/// Put a comma separated list of the colan separated pairs of \c m onto the stream \c os
template <typename K, typename T>
    ostream&
operator<<(ostream& os, const map<K,T>& m)
{ os << Foundation::CommaSeparatedMap<K,T>(m); return os; }

} // namespace std

#endif // FOUNDATION_STD_LOGGER_HDR_
