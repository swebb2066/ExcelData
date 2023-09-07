#ifndef FOUNDATION_VALUE_SOURCE_HDR
#define FOUNDATION_VALUE_SOURCE_HDR
#include <string>

namespace Foundation
{

/// Abstract interface of a value provider
class ValueSource
{
public: // Types
    using StringType = std::string;

public: // ...structors
    virtual ~ValueSource() {}

public: // Accessors
    /// The value of \c name
    virtual bool GetValue(const StringType& name, StringType& result) const = 0;
};

} // namespace Foundation

#endif // FOUNDATION_VALUE_SOURCE_HDR
