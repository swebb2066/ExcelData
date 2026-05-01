#ifndef _SAFE_ARRAY_WRAPPER_H_
#define _SAFE_ARRAY_WRAPPER_H_
#include <Foundation/Logger.h>
#include <boost/array.hpp>
#include "comutil.h"
#include <OAIdl.h>

/// Provide access to the parameters of a one or two dimensional SAFEARRAY
class SafeArrayData
{
protected: // Attributes
    SAFEARRAY* m_data;
    long m_lBound[2], m_uBound[2];
    VARTYPE m_valueType;

public: // ...structor
    /// The SAFEARRAY at \c data
    SafeArrayData() : m_data(0)
    {
    }
    /// The SAFEARRAY at \c data
    SafeArrayData(SAFEARRAY* data) : m_data(data)
    {
        SetAttributes();
    }
    /// A new SAFEARRAY for items of \c type with \c dimCount dimensions and \c bounds defined by
    SafeArrayData(VARTYPE type, int dim, const SAFEARRAYBOUND* bound) :
        m_data(SafeArrayCreate(type, dim, const_cast<SAFEARRAYBOUND*>(bound)))
    {
        assert(0 < dim && dim <= 2);
        SetAttributes();
    }

public: // Conversion operators
    operator SAFEARRAY*() { return m_data; }

public: // Modifiers
    /// Use the SAFEARRAY at \c data
    SafeArrayData& operator=(SAFEARRAY* data)
    {
        m_data = data;
        SetAttributes();
        return *this;
    }

public: // Accessors
    /// Is this a valid SafeArray
    bool IsValid() const { return m_data != 0; }

    /// Is \c index a valid index for the dimension \c dim
    bool IsIndexValid(int dim, long index) const { return m_lBound[dim] <= index && index <= m_uBound[dim]; }

    /// The number of bytes in an elememt
    int ElementSize() const { return SafeArrayGetElemsize(m_data); }

    /// The number of entries in a dimension
    int DimensionSize(int dim) const { return (m_uBound[dim] - m_lBound[dim] + 1); }

    /// The dimension containing \c count values
    int FindDimensionOfSize(int count) const
    {
        if (!m_data)
            return -1;
        else if (DimensionSize(0) == count)
            return 0;
        else if (1 < SafeArrayGetDim(m_data) && DimensionSize(1) == count)
            return 1;
        return -1;
    }

    /// The dimension containing the most values
    int LargestDimension() const
    {
        if (SafeArrayGetDim(m_data) < 2 || (m_uBound[1] - m_lBound[1]) < (m_uBound[0] - m_lBound[0]))
            return 0;
        else
            return 1;
    }

    /// Put the low bound indices into \c result
    int GetLowIndex(long *result, int indexSize) const
    {
        int dim;
        for (dim = 0; dim < indexSize && dim < (int)SafeArrayGetDim(m_data); ++dim)
            result[dim] = m_lBound[dim];
        return dim;
    }

    /// Extract the element at \c index from the SafeArray into \c result
    template <typename T, typename C = T>
    void GetElement(long index[2], T& result) const
    {
        switch (m_valueType)
        {
            case VT_I4:
            {
                LONG value;
                SafeArrayGetElement(m_data, index, &value);
                result = C(value);
                break;
            }
            case VT_UI4:
            {
                ULONG value;
                SafeArrayGetElement(m_data, index, &value);
                result = C(value);
                break;
            }
            case VT_UI1:
            {
                BYTE value;
                SafeArrayGetElement(m_data, index, &value);
                result = C(value);
                break;
            }
            case VT_I2:
            {
                SHORT value;
                SafeArrayGetElement(m_data, index, &value);
                result = C(value);
                break;
            }
            case VT_R4:
            {
                FLOAT value;
                SafeArrayGetElement(m_data, index, &value);
                result = C(value);
                break;
            }
            case VT_R8:
            {
                DOUBLE value;
                SafeArrayGetElement(m_data, index, &value);
                result = C(value);
                break;
            }
            case VT_VARIANT:
            {
                _variant_t value;
                SafeArrayGetElement(m_data, index, &value);
                result = C(value);
            }
        }
        LOG4CXX_DEBUG(m_log, "GetElement:" << " type " << m_valueType << " index [" << index[0] << ", " << index[1] << "] result " << result);
    }

    /// Replace the element at \c index with \c newVal
    template <typename T>
    void PutElement(long index[2], const T& newVal) const
    {
        LOG4CXX_DEBUG(m_log, "PutElement:" << " type " << m_valueType << " index [" << index[0] << ", " << index[1] << "] value " << newVal);
        switch (m_valueType)
        {
            case VT_I4:
            {
                LONG value = (LONG)newVal;
                SafeArrayPutElement(m_data, index, &value);
                break;
            }
            case VT_UI4:
            {
                ULONG value = (ULONG)newVal;
                SafeArrayPutElement(m_data, index, &value);
                break;
            }
            case VT_UI1:
            {
                BYTE value = (BYTE)newVal;
                SafeArrayPutElement(m_data, index, &value);
                break;
            }
            case VT_I2:
            {
                SHORT value = (SHORT)newVal;
                SafeArrayPutElement(m_data, index, &value);
                break;
            }
            case VT_R4:
            {
                FLOAT value = (FLOAT)newVal;
                SafeArrayPutElement(m_data, index, &value);
                break;
            }
            case VT_R8:
            {
                DOUBLE value = (DOUBLE)newVal;
                SafeArrayPutElement(m_data, index, &value);
                break;
            }
        }
    }

    /// Get elements from the SafeArray into the vector \c vec
    template <typename T>
    int GetElementVector(T* vec, int vecSize)
    {
        long index[2];
        int dim1 = LargestDimension();
        int dim2 = dim1 ? 0 : 1; // The other dimension index
        int i = 0;
        for (index[dim2] = m_lBound[dim2]; i < vecSize && index[dim2] <= m_uBound[dim2]; ++index[dim2])
            for (index[dim1] = m_lBound[dim1]; i < vecSize && index[dim1] <= m_uBound[dim1]; ++index[dim1])
                GetElement(index, vec[i++]);
        return i;
    }

    /// Get elements from the SafeArray into the range \c begin to \c end
    template <typename OutIter>
    size_t GetStringVector(OutIter begin, OutIter end)
    {
        long index[2];
        int dim1 = LargestDimension();
        int dim2 = dim1 ? 0 : 1; // The other dimension index
        OutIter i = begin;
        for (index[dim2] = m_lBound[dim2]; i != end && index[dim2] <= m_uBound[dim2]; ++index[dim2])
            for (index[dim1] = m_lBound[dim1]; i != end && index[dim1] <= m_uBound[dim1]; ++index[dim1])
                GetElement<std::string, _bstr_t>(index, *i++);
        return i - begin;
    }

    /// Get elements from \c row (in [1..DimensionSize(1)]) into the range \c begin to \c end
    template <typename OutIter>
    size_t GetStringVector(long row, OutIter begin, OutIter end)
    {
        long index[2] = { row + m_lBound[0] - 1, m_lBound[1] };
        OutIter i = begin;
        for (; i != end && index[1] <= m_uBound[1]; ++index[1])
            GetElement<std::string, _bstr_t>(index, *i++);
        return i - begin;
    }

public: // Methods
    /// Output as text to \c os
    void Write(std::ostream& os) const
    {
        if (!IsValid())
        {
            os << "Null";
            return;
        }
        static const int MaxArrayDimensions = 2;
        long indices[MaxArrayDimensions] = {m_lBound[0], m_lBound[1]};
        int iDim, nDims = SafeArrayGetDim(m_data);
        bool more = true;
        os << std:: endl;
        while (more)
        {
            if (2 < nDims && indices[0] == m_lBound[0] && indices[1] == m_lBound[1])
                os << "[:,:," << Foundation::CommaSeparatedArray<long>(&indices[2], nDims-2) << ']' << std:: endl;
            double value;
            GetElement(indices, value);
            os << value;
            more = false;
            for (iDim = 0; !more && iDim < nDims; ++iDim)
            {
                // Matlab display order - rows down, columns across
                int dim = iDim;
                if (1 < nDims && dim < 2)
                    dim = (dim == 0 ? 1 : 0);
                ++indices[dim];
                if (m_uBound[dim] + 1 <= indices[dim])
                    indices[dim] = m_lBound[dim];
                else
                    more = true;
            }
            if (1 < iDim)
                os << std::endl;
            else
                os << ' ';
        }
    }

    static log4cxx::LoggerPtr Log() { return m_log; }

protected: // Support methods
    /// Extract the type and the bounds from \c m_data
    void SetAttributes()
    {
        m_lBound[0] = 0, m_uBound[0] = 0;
        m_lBound[1] = 0, m_uBound[1] = 0;
        if (m_data &&
            SafeArrayGetVartype(m_data, &m_valueType) == S_OK &&
            1 <= SafeArrayGetDim(m_data) &&
            SafeArrayGetLBound(m_data, 1, &m_lBound[0]) == S_OK && // row bounds
            SafeArrayGetUBound(m_data, 1, &m_uBound[0]) == S_OK &&
            (SafeArrayGetDim(m_data) < 2 ||
            SafeArrayGetLBound(m_data, 2, &m_lBound[1]) == S_OK && // col bounds
            SafeArrayGetUBound(m_data, 2, &m_uBound[1]) == S_OK ))
        {
            if (SafeArrayGetDim(m_data) < 2)
            {
                LOG4CXX_DEBUG(m_log, "SafeArrayData:" << " type " << m_valueType << " length " << (m_uBound[0] - m_lBound[0] + 1));
            }
            else
                LOG4CXX_DEBUG(m_log, "SafeArrayData:" << " type " << m_valueType << " size " << (m_uBound[0] - m_lBound[0] + 1) << 'x' << (m_uBound[1] - m_lBound[1] + 1));
        }
    }


protected: // Class data
    static log4cxx::LoggerPtr m_log;
};

inline std::ostream& operator<<(std::ostream& s, const SafeArrayData& m)
{ m.Write(s); return s;  }

/// Iterator over elements of a vector stored in a one or two dimensional SAFEARRAY
template <typename T>
class SafeArrayVectorIterator
{
protected: // Attributes
    SafeArrayData m_source;
    long m_index[2];
    int m_activeDim;
    T m_item;

public: // ...structor
    // Extract a vector from a row or column of a SAFEARRAY
    SafeArrayVectorIterator(SAFEARRAY* data) : m_source(data), m_activeDim(m_source.LargestDimension())
    {
        // Initially at start
        if (IsValid())
            Start();
    }
    /// Extract a vector from a row or column of a SAFEARRAY in a VARIANT
    SafeArrayVectorIterator(VARIANT& data)
    {
        if (V_ISARRAY(&data))
        {
            m_source = data.parray;
            if (IsValid())
            {
                m_activeDim = m_source.LargestDimension();
                Start();
            }
        }
    }

public: // Operators
    SafeArrayVectorIterator& operator++() { Forth(); return *this;  }
    const T& operator*() const { return Item(); }

public: // Accessors
    /// Is this before the start or off the end
    bool Off() const
    {
        return !m_source.IsIndexValid(m_activeDim, m_index[m_activeDim]);
    }

    /// The current value: PRE: !Off()
    const T& Item() const { return m_item; }

    /// Is the type of the array consistant with the item type
    bool IsValid() const { return m_source.IsValid(); }

    /// The number of elements
    int Size() const { return m_source.DimensionSize(m_activeDim); }

public: // Methods
    /// Move to the first element - PRE: IsValid();
    void Start()
    {
        // Get the indixes of the first element
        m_source.GetLowIndex(m_index, 2);
        // Get the first element
        m_source.GetElement(m_index, m_item);
        LOG4CXX_DEBUG(m_source.Log(), "Start:" << " activeDim " << m_activeDim << " at " << m_index[0] << ',' << m_index[1]);
    }

    /// Move to the next item:" << " PRE: IsValid() && !Off()
    void Forth()
    {
        ++m_index[m_activeDim];
        if (!Off())
        {
            m_source.GetElement(m_index, m_item);
            LOG4CXX_DEBUG(m_source.Log(), "At:" << " " << m_index[0] << ',' << m_index[1]);
        }
    }
};

/// Iterator over elements of pairs of vectors stored in a two dimensional SAFEARRAY
template <typename T>
class SafeArrayVectorPairIterator
{
public: // Types
    typedef SafeArrayVectorPairIterator<T> ThisType;
    typedef T VectorType;
    typedef std::pair<VectorType, VectorType> ItemType;

protected: // Attributes
    SafeArrayData m_source;
    ItemType m_item;
    long m_index[2];
    int m_vectorDim;

public: // ...structor
    // Extract a vector from a row or column of a SAFEARRAY
    SafeArrayVectorPairIterator(SAFEARRAY* data) : m_source(data), m_vectorDim(m_source.FindDimensionOfSize((int)m_item.first.size()))
    {
        m_index[0] = m_index[1] = -1; // Off()
        if (IsValid())
            Start();
    }
    /// Extract a vector from a row or column of a SAFEARRAY in a VARIANT
    SafeArrayVectorPairIterator(VARIANT& data) : m_vectorDim(m_source.FindDimensionOfSize((int)m_item.first.size()))
    {
        if (V_ISARRAY(&data))
        {
            m_source = data.parray;
            if (IsValid())
                Start();
        }
    }

public: // Operators
    ThisType& operator++() { Forth(); return *this;  }
    const ItemType& operator*() const { return Item(); }

public: // Accessors
    /// Is this before the start or off the end: PRE: IsValid()
    bool Off() const
    {
        int otherDim = !m_vectorDim; // Switch 0 & 1
        return !m_source.IsIndexValid(otherDim, m_index[otherDim]);
    }

    /// The current value: PRE: !Off()
    const ItemType& Item() const {  return m_item; }

    /// Is the type of the array consistant with the item type
    bool IsValid() const { return m_source.IsValid() && 0 <= m_vectorDim; }

public: // Methods
    /// Move to the first element - PRE: IsValid();
    void Start()
    {
        LOG4CXX_DEBUG(m_source.Log(), "Start:" << " vectorDim " << m_vectorDim);
        // Get the indices of the first element
        m_source.GetLowIndex(m_index, 2);
        --m_index[!m_vectorDim];
        Forth();
    }

    /// Move to the next item:" << " PRE: IsValid() && !Off()
    void Forth()
    {
        ++m_index[!m_vectorDim];
        // Get the first item
        if (!Off())
        {
            for (int j = 0; j < (int)m_item.first.size(); ++j)
            {
                m_source.GetElement(m_index, m_item.first[j]);
                ++m_index[m_vectorDim];
            }
            m_index[m_vectorDim] -= (long)m_item.first.size();
            ++m_index[!m_vectorDim];
            if (!Off())
            {
                for (int j = 0; j < (int)m_item.second.size(); ++j)
                {
                    m_source.GetElement(m_index, m_item.second[j]);
                    ++m_index[m_vectorDim];
                }
                m_index[m_vectorDim] -= (long)m_item.second.size();
                LOG4CXX_DEBUG(m_source.Log(), "At:" << " " << m_item.first << ';' << m_item.second);
            }
        }
    }
};

/// A vector stored in a one or two dimensional SAFEARRAY
template <typename T>
class SafeArrayVector
{
protected: // Types
    typedef T VectorType;
    typedef typename VectorType::ValueType ValueType;
    typedef typename SafeArrayVectorIterator<ValueType> StoreType;

protected: // Attributes
    StoreType m_source;
    VectorType m_item;
    bool m_valid;

public: // ...structor
    // Extract a vector from a row or column of a SAFEARRAY
    SafeArrayVector(SAFEARRAY* data)
        : m_source(data), m_valid(false)
    {
        SetItem();
    }
    /// Extract a vector from a row or column of a SAFEARRAY in a VARIANT
    SafeArrayVector(VARIANT& data)
        : m_valid(false)
    {
        if (V_ISARRAY(&data))
        {
            m_source = data.parray;
            SetItem();
        }
    }

public: // Accessors
    /// The transform value: PRE: IsValid()
    operator const VectorType&() const { return m_item; }

    /// Is the type of the array consistant with the item type
    bool IsValid() const { return m_valid; }

protected: // Support methods
    /// Extract values from m_source  into m_item
    void SetItem()
    {
        if (m_source.IsValid())
        {
            int j = 0;
            for (; !m_source.Off() && j < (int)m_item.size(); ++j, m_source.Forth())
                m_item[j] = m_source.Item();
            m_valid = (j == m_item.size());
        }
    }
};

/// An iterator over an array of vectors stored in a two dimensional SAFEARRAY
    template <typename T>
class SafeVectorArrayIterator
{
public: // Types
    typedef T VectorType;
    typedef SafeVectorArrayIterator<T> ThisType;

protected: // Attributes
    SafeArrayData m_source;
    VectorType m_item;
    int m_vectorDim;
    long m_index[2];

public: // ...structor
    /// An iterator over the vectors in the SAFEARRAY \c data
    SafeVectorArrayIterator(SAFEARRAY* data)
        : m_source(data)
        , m_vectorDim(m_source.FindDimensionOfSize((int)m_item.size()))
    {
        m_index[0] = m_index[1] = -1; // Off()
    }
    /// An iterator over the vectors in \c data
    SafeVectorArrayIterator(VARIANT& data)
        : m_source(0)
        , m_vectorDim(-1)
    {
        m_index[0] = m_index[1] = -1; // Off()
        if (V_ISARRAY(&data))
        {
            m_source = data.parray;
            m_vectorDim = m_source.FindDimensionOfSize((int)m_item.size());
        }
        else if (V_VT(&data) != VT_EMPTY && V_VT(&data) != VT_NULL && V_VT(&data) != VT_ERROR)
        {
            LOG4CXX_WARN(m_source.Log(), "SafeVectorArrayIterator:" << " type " << V_VT(&data) << " is not valid.");
        }
    }

public: // Operators
    ThisType& operator++() { Forth(); return *this;  }
    const VectorType& operator*() const { return Item(); }

public: // Accessors
    /// Is the type of the array consistant with the item type
    bool IsValid() const { return m_source.IsValid() && 0 <= m_vectorDim; }

    /// The current value: PRE: !Off()
    const VectorType& Item() const {    return m_item; }

    /// Is this before the start or off the end: PRE: IsValid()
    bool Off() const
    {
        int otherDim = !m_vectorDim; // Switch 0 & 1
        return !m_source.IsIndexValid(otherDim, m_index[otherDim]);
    }

    /// The number of vectors
    int Size() const { return m_source.DimensionSize(!m_vectorDim); }

public: // Methods
    /// Move to the first element - PRE: IsValid();
    void Start()
    {
        LOG4CXX_DEBUG(m_source.Log(), "Start:" << " vectorDim " << m_vectorDim);
        // Get the indices of the first element
        m_source.GetLowIndex(m_index, 2);
        --m_index[!m_vectorDim];
        Forth();
    }

    /// Move to the next item:" << " PRE: IsValid() && !Off()
    void Forth()
    {
        ++m_index[!m_vectorDim];
        if (!Off())
        {
            // Convert the values
            for (int j = 0; j < (int)m_item.size(); ++j)
            {
                m_source.GetElement(m_index, m_item[j]);
                ++m_index[m_vectorDim];
            }
            m_index[m_vectorDim] -= (long)m_item.size();
        }
    }
};

/// Generate an empty SafeArray of \c type
    SAFEARRAY*
MakeEmptySafeArray(int type);

/// Convert a vector to a safe array of DOUBLE
template <typename T>
    SAFEARRAY*
MakeSafeArrayVector(int type, const T* vec, int vecSize)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = vecSize;
    SafeArrayData result(type, 1, rgsabound);
    LONG index[1];
    for (index[0] = 0; index[0] < LONG(rgsabound[0].cElements); ++index[0])
        result.PutElement(index, vec[index[0]]);
    return result;
}

/// Convert a 2D array to a safe array of DOUBLE
template <typename MatrixType>
    SAFEARRAY*
MakeSafeArrayMatrix(int type, const MatrixType& data, int rowCount, int colCount)
{
    SAFEARRAYBOUND rgsabound[2];
    rgsabound[0].lLbound = 1;
    rgsabound[0].cElements = rowCount;
    rgsabound[1].lLbound = 1;
    rgsabound[1].cElements = colCount;
    SafeArrayData result(type, 2, rgsabound);
    LONG index[2];
    for (index[0] = 1; index[0] <= LONG(rgsabound[0].cElements); ++index[0])
        for (index[1] = 1; index[1] <= LONG(rgsabound[1].cElements); ++index[1])
            result.PutElement(index, data(index[0], index[1]));
    return result;
}

/// Convert an array of vectors to a safe array of DOUBLE
template <typename MatrixType>
    SAFEARRAY*
MakeSafeArrayVectorVector(int type, const MatrixType& data, int rowCount, int colCount)
{
    SAFEARRAYBOUND rgsabound[2];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = rowCount;
    rgsabound[1].lLbound = 0;
    rgsabound[1].cElements = colCount;
    SafeArrayData result(type, 2, rgsabound);
    LONG index[2];
    for (index[0] = 0; index[0] < LONG(rgsabound[0].cElements); ++index[0])
        for (index[1] = 0; index[1] < LONG(rgsabound[1].cElements); ++index[1])
            result.PutElement(index, data[size_t(index[0])][size_t(index[1])]);
    return result;
}

#endif //_SAFE_ARRAY_WRAPPER_H_
