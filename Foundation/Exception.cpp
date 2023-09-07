#include <Foundation/Exception.h>

namespace Foundation
{

Exception::Exception(const std::string& msg)
    : m_description(msg)
{}

Exception::~Exception() throw()
{}

    const char*
Exception::what() const throw()
{
    return m_description.c_str();
}

MissingValueException::MissingValueException(const std::string& item)
    : Exception(item + ": not found")
{   }

MissingValueException::MissingValueException(const std::string& item, const std::string& context)
    : Exception(item + ": not found - see " + context)
{   }

MissingFileException::MissingFileException(const boost::filesystem::path& fileName)
    : Exception(fileName.string() + ": not found")
{   }

MissingFileException::MissingFileException(const boost::filesystem::path& fileName, const std::string& context)
    : Exception(fileName.string() + ": not found - see " + context)
{   }

BadFileException::BadFileException(const std::string& msg, const boost::filesystem::path& fileName)
    : Exception(msg + " in " + fileName.string())
{   }

BadFileException::BadFileException(const std::string& msg, const boost::filesystem::path& fileName, const std::string& context)
    : Exception(msg + " in " + fileName.string() + " in " + context)
{   }

InvalidValue::InvalidValue(const std::string& itemName, const std::string& context)
    : Exception("Invalid " + itemName + " at " + context)
{   }

ReadFileException::ReadFileException(const boost::filesystem::path& fileName)
    : Exception(fileName.string() + ": could not be read")
{   }

ReadFileException::ReadFileException(const boost::filesystem::path& fileName, const std::string& context)
    : Exception(fileName.string() + ": could not be read - see " + context)
{   }

WriteFileException::WriteFileException(const boost::filesystem::path& fileName)
    : Exception(fileName.string() + ": could not write")
{   }

WriteFileException::WriteFileException(const boost::filesystem::path& fileName, const std::string& context)
    : Exception(fileName.string() + ": could not write - see " + context)
{   }

UnsupportedValueException::UnsupportedValueException(const std::string& context, const std::string& unsupportedItem)
    : Exception(context + ": the value " + unsupportedItem + " is not supported")
{   }

UnsupportedTypeException::UnsupportedTypeException(const std::type_info& unsupportedType, const std::string& context)
    : Exception("The type " + std::string(unsupportedType.name()) + " is not supported. " + context)
{   }

} // namespace Foundation
