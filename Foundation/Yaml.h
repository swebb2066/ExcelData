// Extensions to the YAML namespace
#ifndef FOUNDATION_YAML_HEADER__
#define FOUNDATION_YAML_HEADER__

#include <yaml-cpp/yaml.h>
#include <boost/filesystem/path.hpp>

/// Extensions to yaml-cpp
namespace YAML
{

/// The value of element \c elementName in the map \c node
   inline Node
ReqNode(const Node& node, const char* elementName)
{
    Node result = node[elementName];
    if (!result)
        throw KeyNotFound(node.Mark(), std::string(elementName));
    return result;
}

/// Output mark attributes onto the stream \c os
    inline std::ostream&
operator<<(std::ostream& os, const Mark& loc)
{
    os << loc.pos << '(' << loc.line << ',' << loc.column << ')';
    return os;
}

/// A string that describes a file location
     std::string
FileLocation(const Mark& mark, const boost::filesystem::path& fileName);

/// An exception that reports ambiguity
class AmbiguousItem : public Exception
{
public: // ...stuctors
    AmbiguousItem(const Mark& mark, const std::string& item1, const std::string& item2, const std::string& context)
        : Exception(mark, "Use either " + item1 + " or " + item2 + " in " + context)
    {   }
};

class ConflictingItem : public Exception
{
public: // ...stuctors
    ConflictingItem(const Mark& mark, const std::string& item1Name, const std::string& item2Name)
        : Exception(mark, item1Name + " is not compatible with " + item2Name)
    {   }
};

/// An exception that describes a file location
class MissingFileException : public Exception
{
public: // ...stuctors
    MissingFileException(const Mark& mark, const boost::filesystem::path& fileName)
        : Exception(mark, fileName.string() + ": not found")
    {   }
    MissingFileException(const Mark& mark, const boost::filesystem::path& fileName, const std::string& context)
        : Exception(mark, fileName.string() + ": not found - see " + context)
    {   }
};

class UnsupportedValueException : public Exception
{
public: // ...stuctors
    UnsupportedValueException(const Mark& mark, const std::string& unsupportedItem, const std::string& context)
        : Exception(mark, "The value " + unsupportedItem + " is not supported - see " + context)
    {   }
};

class RequiredItem : public Exception
{
    std::string MakeMessage(const char* keyName[])
    {
        std::stringstream ss;
        const char* separ = "";
        for (const char** pKeyName = keyName; *pKeyName; ++pKeyName)
        {
            if (keyName < pKeyName && !*(pKeyName + 1))
                separ = " or ";
            ss << separ << *pKeyName;
            separ = ", ";
        }
        return ss.str();
    }
public: // ...stuctors
    RequiredItem(const Mark& mark, const char* keyName[], const std::string& context)
        : Exception(mark, "One of " + MakeMessage(keyName)
        + (!context.empty() ? std::string(" required") : std::string(" required at " + context)))
    {   }
};

class RequiredItemType : public Exception
{
public: // ...stuctors
    RequiredItemType(const Mark& mark, const std::string& requiredType, const std::string& context)
        : Exception(mark, requiredType + " required at " + context)
    {   }
};

class RecursiveDefinition : public Exception
{
public: // ...stuctors
    RecursiveDefinition(const Mark& mark)
        : Exception(mark, "recursive definition")
    {   }
};

class StackTooDeep : public Exception
{
public: // ...stuctors
    StackTooDeep(const Mark& mark, const std::string& stackName, int limit)
        : Exception(mark, stackName + " stack exceeds " + std::to_string(limit) + " elements")
    {   }
};

class InvalidValue : public Exception
{
public: // ...stuctors
    InvalidValue(const Mark& mark, const std::string& itemName, const std::string& context)
        : Exception(mark, "Invalid " + itemName + " in " + context)
    {   }
};

class InvalidSize : public Exception
{
public: // ...stuctors
    InvalidSize
        ( const Mark&        mark
        , const std::string& itemName
        , int                actualSize
        , int                expectedSize
        , const std::string& context = std::string()
        );
    InvalidSize
        ( const Mark&        mark
        , const std::string& actualClause
        , int                actualSize
        , const std::string& expectedClause
        , int                expectedSize
        , const std::string& terminationClause = std::string()
        , const std::string& context = std::string()
        );
};

class UnexpectedItem : public Exception
{
public: // ...stuctors
    UnexpectedItem(const Mark& mark, const std::string& expected, const std::string& where)
        : Exception(mark, expected + " expected at " + where)
    {   }
};

// Replace this with a the copy in yaml-cpp when it is made public
    bool
IsAnchorChar(unsigned short ch);

// Are all characters of \c name valid in an anchor
    bool
IsValidAnchor(const std::string& name);

} // namespace YAML

#endif // FOUNDATION_YAML_HEADER__
