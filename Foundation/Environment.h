#ifndef FOUNDATION_ENVIRONMENT_HDR_
#define FOUNDATION_ENVIRONMENT_HDR_

#include <Foundation/Config.h>
#include <Foundation/Logger.h>
#include <memory>
#include <string>
#include <sstream>

namespace Foundation
{

/// The abstract base class for accessing the values of environment variables.
class EnvironmentSection
{
public: // Types
    using StringType = std::string;

protected: // Attributes
    StringType m_section;

public: // ...structors
    EnvironmentSection(const StringType& section) : m_section(section) {}
    virtual ~EnvironmentSection() {}

public: // Property accessors
    const StringType& GetSectionName() const { return m_section; }

public: // Accessors
    /// The integer value of the variable \c name, with \c defaultValue returned if the variable is not present.
    virtual int GetInt(const char* name, int defaultValue) const;

    /// The string value of the variable \c name, with \c defaultValue returned if the variable is not present.
    virtual StringType GetString(const char* name, const char* defaultValue) const;

    /// The string value of the variable \c name with %Var% instances replaced by process variables.
    /// Returns \c defaultValue if the variable is not present in the environment.
    StringType GetExpandedString(const char* name, const char* defaultValue) const;

    /// Update \c result from the environment variable \c name, return true if result was changed
    template <typename T>
    bool GetValue(const char* name, T& result) const
    {
        static const char uniqueStr[] = "2389179B-E078-437f-A25B-CDA740A28ADB";
        StringType strValue(GetString(name, uniqueStr));
        if (strValue == uniqueStr)
            return false;
        std::istringstream valueStream(strValue);
        valueStream >> result;
        return !valueStream.fail();
    }

public: // Methods
    /// Associate \c value with the variable \c name
    virtual bool PutInt(const char* name, int value);

    /// Associate \c value with the variable \c name
    virtual bool PutString(const char* name, const char* value);

    /// Update the environment variable \c name with \c newValue
    template <typename T>
    bool PutValue(const char* name, T& newValue)
    {
        std::ostringstream valueStream;
        valueStream << newValue << std::ends;
        return PutString(name, valueStream.str().c_str());
    }
};

/// The abstract base class and singleton for accessing sections of environement variables.
class Environment
{
public: // Types
    using StringType = std::string;
    using Ptr = std::unique_ptr<Environment>;
    using SectionPtr = std::unique_ptr<EnvironmentSection>;

public: // ...structors
    /// Environment variables from the default sources
    static Environment* GetInstance();

    /// Writable environment variables initialised from \c sourceName
    static Ptr GetWritableSource(const StringType& sourceName);

    /// Release resources of the default source
    static void DeleteInstance();

    /// Release resouces
    virtual ~Environment() {}

public: // Accessors
    /// The section of environment cached variables with name \c key, optionally refreshed after \c secondsToLive.
    SectionPtr GetCachedSection(const StringType& key, double secondsToLive = 0) const;

    /// The section of default environment variables with name \c key.
    virtual SectionPtr GetDefaultSection(const StringType& key) const = 0;

    /// The section of environment variables with name \c key.
    virtual SectionPtr GetSection(const StringType& key) const = 0;

    /// The source of values from the section of environment variables with name \c sectionKey.
    virtual StringType GetSource(const StringType& sectionKey = StringType()) const = 0;

    /// The alternate source of environment variable values
    virtual StringType GetAlternateSource(const StringType& ext = ".ini") const;

    /// The primary source of the environment variable values
    virtual StringType GetPrimeSource(const StringType& ext = ".ini") const;

public: // Methods
    /// Dismount the writable removable drive
    virtual bool DismountRemoveableMediaPath() { return false; }

    /// The path prefix of a writable removable drive
    virtual StringType GetRemoveableMediaPathPrefix() { return StringType(); }

    /// Set the primary source of the environment variable values
    virtual void PutPrimeSource(const StringType& sourceFile) = 0;

protected: // Class attributes
    static Environment* m_instance; //!< The shared instance of this
    static StringType m_pathPrefix; //!< The path of the current executable file without the extension
    static StringType m_baseName; //!< A name under which files for this application are stored
    static StringType m_partitionName; //!< A relative name under which files for this application are stored

public: // Class methods
    ///
    /// The path to \c fileName where application data files are stored
    static StringType GetAppDataPath(const StringType& fileName = StringType());

    /// The path \c subDir where configuration files are stored (where a normal user can write)
    static StringType GetConfigDataPath(const StringType& subDir = StringType());

    /// The path to a file with extension \c ext named like the current executable file
    static StringType GetConfigFile(const StringType& ext);

    // A string with %Var% instances in \c inString replaced by the value of the named process variable.
    static StringType GetExpandedString(const StringType& inString);

    /// The path to a file named \c stem with extension \c ext in a local directory
    static StringType GetLocalFile(const StringType& stem, const StringType& ext = StringType());

    /// The per process environment variables.
    static EnvironmentSection& GetProcessVariables();

    /// The path prefix for temporary files
    static const StringType& GetTempFilePathPrefix();

    /// The path prefix for user files
    static const StringType& GetUserDirectoryPrefix();

    /// The identifier of the current user
    static StringType GetLoginName();

    /// The identifier for the current system
    static StringType GetSystemName();

    /// Get the name under which application data files are stored
    static const StringType& GetUniqueName() { return m_baseName; }

    /// Get the relative name under which application data files are stored
    static const StringType& GetPartitionName() { return m_partitionName; }

    /// The path of the current executable file without the extension
    static const StringType& PathPrefix();

    /// Set the name under which application data files are stored
    static void PutUniqueName(const StringType& basePart, const StringType& relativePart = StringType())
    { m_baseName = basePart; m_partitionName = relativePart; }

protected: // Not publicably creatable
    Environment(const StringType& pathPrefix = StringType())
    {
        if (!pathPrefix.empty())
            m_pathPrefix = pathPrefix;
    }

protected: // Class support methods
    /// Ensure changes are allowed to the local file composed from \c stem and \c ext
    static void MakeLocalFileWritable(const StringType& stem, const StringType& ext = StringType());

private: // non copyable
    Environment(const Environment&) {}
    Environment& operator=(Environment&) { return *this; }

private:
    static LoggerPtr m_log;
};

} // namespace Foundation

#endif

