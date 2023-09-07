#ifndef FOUNDATION_WINDOWS_INI_ENVIRONMENT_HDR_
#define FOUNDATION_WINDOWS_INI_ENVIRONMENT_HDR_

#include <Foundation/Environment.h>
#include <Foundation/Logger.h>

namespace Foundation
{

/// Provide access to sections of a configuration file
class WindowsEnvironment : public Environment
{
protected: // Attributes
    StringType m_primeFile;
    StringType m_defaultFile;

public: // ...structors
    /// A .ini file named from \c pathPrefix or otherwise like this executable
    WindowsEnvironment
        ( const StringType& pathPrefix = StringType()
        , const StringType& primeSource = StringType()
        , const StringType& defaultSource = StringType()
        );
    /// Release resources
    ~WindowsEnvironment();

public: // Accessors
    /// The section of default environment variables with name \c key.
    SectionPtr GetDefaultSection(const StringType& key) const override;

    /// The \c key section
    SectionPtr GetSection(const StringType& key) const override;

    /// The source of values from the section of environment variables with name \c key.
    StringType GetSource(const StringType& key = StringType()) const override;

public: // Methods
    /// Dismount the writable removable drive
    bool DismountRemoveableMediaPath();

    /// The path prefix of a writable removable drive
    StringType GetRemoveableMediaPathPrefix();

    /// Set the primary source of the environment variable values
    void PutPrimeSource(const StringType& source);

    /// Get an integer value for the key \c name from the registry hive at \c section
    bool GetRegistryInt(const StringType& section, const char* name, int& result) const;

    /// Get a string value for the key \c name from the registry hive at \c section
    bool GetRegistryString(const StringType& section, const char* name, char* resultBuf, size_t resultSize) const;

private:
    static log4cxx::LoggerPtr m_log;
    class Section;
};

} // namespace Foundation

#endif // FOUNDATION_WINDOWS_INI_ENVIRONMENT_HDR_

