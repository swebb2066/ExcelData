#include <Foundation/Yaml.h>
namespace
{
    std::string
SizeDifferenceMessage
    ( const std::string& actualClause
    , int                actualSize
    , const std::string& expectedClause
    , int                expectedSize
    , const std::string& terminationClause = std::string()
    )
{
    std::stringstream ss;
    ss << actualClause << " (" << actualSize << ") "
       << expectedClause << " (" << expectedSize << ") "
       << terminationClause;
    return ss.str();
}

} // namespace

/// Extensions to yaml-cpp
namespace YAML
{

     std::string
FileLocation(const Mark& mark, const fs::path& fileName)
{
    if (mark.is_null())
        return fileName.string();
    std::stringstream output;
    output << "line " << (mark.line + 1) << ", column " << (mark.column + 1) << " in " << fileName;
    return output.str();
}

InvalidSize::InvalidSize
    ( const Mark&        mark
    , const std::string& itemName
    , int                actualSize
    , int                expectedSize
    , const std::string& context
    )
    : Exception(mark, SizeDifferenceMessage
        ( itemName + " size", actualSize
        , "is not the expected size", expectedSize
        ) + " in " + context)
{
}

InvalidSize::InvalidSize
    ( const Mark&        mark
    , const std::string& actualClause
    , int                actualSize
    , const std::string& expectedClause
    , int                expectedSize
    , const std::string& terminationClause
    , const std::string& context
    )
    : Exception(mark, SizeDifferenceMessage
        ( actualClause, actualSize
        , expectedClause, expectedSize
        , terminationClause
        ) + " in " + context)
{
}

// Replace this with a the copy in yaml-cpp when it is made public
    bool
IsAnchorChar(unsigned short ch)
{
    switch (ch)
    {
        case ',': case '[': case ']': case '{': case '}': // c-flow-indicator
        case ' ': case '\t': // s-white
        case 0xFEFF: // c-byte-order-mark
        case 0xA: case 0xD: // b-char
            return false;
        case 0x85:
            return true;
    }

    if (ch < 0x20)
        return false;

    if (ch < 0x7E)
        return true;

    if (ch < 0xA0)
        return false;
    if (ch >= 0xD800 && ch <= 0xDFFF)
        return false;
    if ((ch & 0xFFFE) == 0xFFFE)
        return false;
    if ((ch >= 0xFDD0) && (ch <= 0xFDEF))
        return false;
    if (ch > 0x10FFFF)
        return false;

    return true;
}

// Are all characters of \c name valid in an anchor
    bool
IsValidAnchor(const std::string& name)
{
    bool result = !name.empty();
    for ( std::string::const_iterator pChar = name.begin()
        ; result && name.end() != pChar
        ; ++pChar)
    {
#ifdef GetNextCodePointAndAdvanceIsPublic
        int codePoint;
        GetNextCodePointAndAdvance(codePoint, pChar, name.end());
        result = IsAnchorChar(codePoint);
#else
        result = IsAnchorChar(*pChar);
#endif
    }
    return result;
}

} // namespace YAML
