#ifndef FOUNDATION_EXCEPTION_HDR_
#define FOUNDATION_EXCEPTION_HDR_
#include <exception>
#include <string>
#include <boost/filesystem/path.hpp>

namespace Foundation
{

class Exception : public std::exception
{
protected: // Attributes
    std::string m_description;

public: // ...stuctors
    Exception(const std::string& msg);
    ~Exception() throw();

public: // Accessors
    const char* what() const throw();
};

class MissingValueException : public Exception
{
public: // ...stuctors
    MissingValueException(const std::string& item);
    MissingValueException(const std::string& item, const std::string& context);
};

class MissingFileException : public Exception
{
public: // ...stuctors
    MissingFileException(const boost::filesystem::path& fileName);
    MissingFileException(const boost::filesystem::path& fileName, const std::string& context);
};

class BadFileException : public Exception
{
public: // ...stuctors
    BadFileException(const std::string& msg, const boost::filesystem::path& fileName);
    BadFileException(const std::string& msg, const boost::filesystem::path& fileName, const std::string& context);
};

class InvalidValue : public Exception
{
public: // ...stuctors
    InvalidValue(const std::string& itemName, const std::string& context);
};

class ReadFileException : public Exception
{
public: // ...stuctors
    ReadFileException(const boost::filesystem::path& fileName);
    ReadFileException(const boost::filesystem::path& fileName, const std::string& context);
};

class WriteFileException : public Exception
{
public: // ...stuctors
    WriteFileException(const boost::filesystem::path& fileName);
    WriteFileException(const boost::filesystem::path& fileName, const std::string& context);
};

class UnsupportedValueException : public Exception
{
public: // ...stuctors
    UnsupportedValueException(const std::string& context, const std::string& unsupportedItem);
};

class UnsupportedTypeException : public Exception
{
public: // ...stuctors
    UnsupportedTypeException(const std::type_info& unsupportedType, const std::string& context);
};

} // namespace Foundation

#endif // FOUNDATION_EXCEPTION_HDR_
