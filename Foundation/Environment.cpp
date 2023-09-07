#include <Foundation/Environment.h>
#include <Foundation/EventTime.h>
#include <Foundation/Logger.h>
#include <ostream>
#include <map>
#ifdef WIN32
#include <windows.h>
#include "Psapi.h"
#pragma comment(lib, "psapi.lib")
#endif
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <boost/version.hpp>
#include <log4cxx/logmanager.h>

namespace Foundation
{

//////////////////////////////////////////
// EnvironmentSection implementation

// The integer value of the variable \c name, with \c defaultValue return if the variable is not present in the environment.
    int
EnvironmentSection::GetInt(const char* name, int defaultValue) const
{
    int result = defaultValue;
    GetValue(name, result);
    return result;
}

// The string value of the variable \c name, with \c defaultValue return if the variable is not present in the environment.
    EnvironmentSection::StringType
EnvironmentSection::GetString(const char* name, const char* defaultValue) const
{
    EnvironmentSection& env = Environment::GetProcessVariables();
    StringType key = m_section;
    key += ".";
    key += name;
    StringType value = env.GetString(key.c_str(), "");
    if (value.empty())
        value = env.GetString(name, defaultValue);
    return value;
}

// The string value of the variable \c name with %Var% instances replaced by process variables.
// Returns \c defaultValue if the variable is not present in the environment.
    EnvironmentSection::StringType
EnvironmentSection::GetExpandedString(const char* name, const char* defaultValue) const
{
    return Environment::GetExpandedString(GetString(name, defaultValue));
}

// Associate \c value with the variable \c name
    bool
EnvironmentSection::PutInt(const char* name, int value)
{
    UNUSED(name);
    UNUSED(value);
    return false;
}

// Associate \c value with the variable \c name
    bool
EnvironmentSection::PutString(const char* name, const char* value)
{
    UNUSED(name);
    UNUSED(value);
    return false;
};

//////////////////////////////////////////
// Environment implementation

    LoggerPtr
Environment::m_log(log4cxx::Logger::getLogger("Environment"));

// The shared instance of this
    Environment*
Environment::m_instance = 0;

// The path of the current executable file without the extension
    Environment::StringType
Environment::m_pathPrefix;

// A name under which files are stored
    Environment::StringType
Environment::m_baseName("EyeInHand");

// A relative name under which files are stored
    Environment::StringType
Environment::m_partitionName;

// Template only -
// GetInstance is implemented in the file that implements the specialisation of Environment
//  Environment*
//Environment::GetInstance()
//{
//  if (m_instance == 0)
//  {
//      m_instance = new XXXEnvironment;
//      atexit(DeleteInstance);
//  }
//  return m_instance;
//}

    void
Environment::DeleteInstance()
{
    delete m_instance;
    m_instance = 0;
}

// The path of the current executable file without the extension
    const Environment::StringType&
Environment::PathPrefix()
{
    if (m_pathPrefix.empty())
    {
        static const int bufSize = 2000;
        char buf[bufSize+1];
#ifdef WIN32
// Allow a debug executable to be used with release configuration files
#       ifdef _DEBUG
        HANDLE hProcess = GetCurrentProcess();
        char moduleBaseNameBuffer[500];
        moduleBaseNameBuffer[0] = '\\';
        GetModuleBaseNameA(hProcess, 0, &moduleBaseNameBuffer[1], sizeof(moduleBaseNameBuffer) - 1);
        GetCurrentDirectory(bufSize - sizeof(moduleBaseNameBuffer), buf);
        strncat(buf, moduleBaseNameBuffer, sizeof(moduleBaseNameBuffer));
#       else // Release
        GetModuleFileName(NULL, buf, bufSize);
#       endif
#else
        std::ostringstream exeLink;
        exeLink << "/proc/" << getpid() << "/exe" << std::ends;
        ssize_t bufCount = readlink(exeLink.str().c_str(), buf, bufSize);
        buf[bufCount] = 0;
#endif
        StringType programName(buf);
        size_t dotIndex = programName.rfind('.');
        m_pathPrefix = programName.substr(0, dotIndex);
    }
    if (m_log)
        LOG4CXX_TRACE(m_log, "PathPrefix: " << m_pathPrefix);
    return m_pathPrefix;
}

// The path to \c fileName where application data files are stored
    Environment::StringType
Environment::GetAppDataPath(const StringType& fileName)
{
    using PathType = boost::filesystem::path;
#ifdef WIN32
    PathType result(GetProcessVariables().GetString("LocalAppData", ""));
#else
    PathType result("/var/local");
    if (!m_log)
        m_log = GetLogger("Environment");
#endif
    result /= m_baseName;
    if (!m_partitionName.empty())
        result /= m_partitionName;
    if (!fileName.empty())
        result /= fileName;
    LOG4CXX_DEBUG(m_log, "GetAppDataPath: " << result);
    return result.string();
}

// The path \c subDir where configuration files are stored where a normal user can write
    Environment::StringType
Environment::GetConfigDataPath(const StringType& subDir)
{
    using PathType = boost::filesystem::path;
#ifdef WIN32
    PathType result(GetProcessVariables().GetString("ProgramData", ""));
#else
    PathType result("/var/local");
    if (!m_log)
        m_log = GetLogger("Environment");
#endif
    result /= m_baseName;
    if (!m_partitionName.empty())
        result /= m_partitionName;
    if (!subDir.empty())
        result /= subDir;
    LOG4CXX_DEBUG(m_log, "GetConfigDataPath: " << result);
    return result.string();
}

// The path to a file with extension \c ext named like the current executable file
    Environment::StringType
Environment::GetConfigFile(const StringType& ext)
{
    using PathType = boost::filesystem::path;
    PathType exeFile(PathPrefix());
    return GetLocalFile(exeFile.stem().string(), ext);
}

// The path to a file named \c stem with extension \c ext in a local directory
    Environment::StringType
Environment::GetLocalFile(const StringType& stem, const StringType& ext)
{
    using PathType = boost::filesystem::path;
#ifdef WIN32
    PathType result(GetProcessVariables().GetString("ProgramData", ""));
#else
    PathType result("/var/local");
    if (!m_log)
        m_log = GetLogger("Environment");
#endif
    result /= m_baseName;
    if (!m_partitionName.empty())
        result /= m_partitionName;
    result /= stem;
    if (!ext.empty())
#if BOOST_VERSION < 105000
        result.replace_extension(ext);
#else
        result += ext;
#endif
    if (exists(result)) // Does a user modified version exist?
        ;
    else if (exists(result.filename())) // Does a current directory version exist?
        result = absolute(result.filename());
    else
    {
        LOG4CXX_TRACE(m_log, "GetConfigFile: !exists " << result);
        // Default to the location of executable
        result = PathType(PathPrefix()).parent_path() / stem;
        if (!ext.empty())
#if BOOST_VERSION < 105000
            result.replace_extension(ext);
#else
            result += ext;
#endif
    }
    LOG4CXX_TRACE(m_log, "GetConfigFile: " << result);
    return result.string();
}

// The primary source of the environment variable values
    Environment::StringType
Environment::GetPrimeSource(const StringType& ext) const
{
    StringType result;
    StringType primeVar = GetProcessVariables().GetString("ENVIRONMENT_PRIME_SOURCE", "");
    if (!primeVar.empty())
    {
        boost::filesystem::path name(primeVar);
        LOG4CXX_TRACE(m_log, "ENVIRONMENT_PRIME_SOURCE " << primeVar << " is_absolute? " << name.is_absolute() << " has_extension? " << name.has_extension());
        if (!name.is_absolute() && !name.has_extension())
            result = GetLocalFile(primeVar, ext);
        else if (!name.is_absolute() && name.has_extension())
            result = GetLocalFile(primeVar);
        else
            result = primeVar;
    }
    else
        result = GetConfigFile(ext);
    LOG4CXX_DEBUG(m_log, "GetPrimeSource: " << result);
    return result;
}

// The alternate source of environment variable values
    Environment::StringType
Environment::GetAlternateSource(const StringType& ext) const
{
    StringType result;
    StringType altVar = GetProcessVariables().GetString("ENVIRONMENT_ALT_SOURCE", "");
    StringType primeVar = GetProcessVariables().GetString("ENVIRONMENT_PRIME_SOURCE", "");
    if (!altVar.empty())
    {
        boost::filesystem::path name(altVar);
        LOG4CXX_TRACE(m_log, "ENVIRONMENT_ALT_SOURCE " << altVar << " is_absolute? " << name.is_absolute() << " has_extension? " << name.has_extension());
        if (!name.is_absolute() && !name.has_extension())
            result = GetLocalFile(altVar, ext);
        else if (!name.is_absolute() && name.has_extension())
            result = GetLocalFile(altVar);
        else
            result = altVar;
    }
    else if (!primeVar.empty())
        result = GetConfigFile(ext);
    else
        result = GetLocalFile("Default", ext);
    LOG4CXX_DEBUG(m_log, "GetAlternateSource: " << result);
    return result;
}

/// Access to the process environment
class ProcessEnvironment : public EnvironmentSection
{
    using BaseType = EnvironmentSection;
private:
    LoggerPtr m_log;

public: // ...structors
    ProcessEnvironment()
        : BaseType(StringType())
        , m_log(log4cxx::Logger::getLogger("ProcessEnvironment"))
    {}

public: // Accessors
    /// The integer value after the \c name= entry in this section.
    int GetInt(const char* name, int defaultValue) const
    {
        LOG4CXX_DEBUG(m_log, "GetInt: " << name);
        int result;
        if (!BaseType::GetValue(name, result))
            result = defaultValue;
        return result;
    }

    /// The string value after the \c name= entry in this section.
    StringType GetString(const char* name, const char* defaultValue) const
    {
        char* var = 0;
#ifdef WIN32
        char buffer[1000];
        if (0 < GetEnvironmentVariable(name, buffer, sizeof (buffer)))
            var = buffer;
#else
        var = getenv(name);
#endif
        if (var)
            return var;
        else
            return defaultValue;
    }

public: // Methods
    /// Associate \c value with the variable \c name
    bool PutInt(const char* name, int value)
    {
        LOG4CXX_DEBUG(m_log, "PutInt: " << name << '=' << value);
        return BaseType::PutValue(name, value);
    }

    /// Associate \c value with the variable \c name
    bool PutString(const char* name, const char* value)
    {
        LOG4CXX_DEBUG(m_log, "PutString: " << name << '=' << value);
#ifdef WIN32
        return SetEnvironmentVariable(name, value) == TRUE;
#else
        return setenv(name, value, true) == 0;
#endif
    }
};

/// Cached access to the named values
class CachedSection : public EnvironmentSection
{
    using BaseType = EnvironmentSection;
protected: // Types
    using AgedInt = std::pair<int, EventTime>;
    using IntCacheType = std::map<StringType, AgedInt>;
    using AgedString = std::pair<StringType, EventTime>;
    using StringCacheType = std::map<StringType, AgedString>;
    using SectionPtr = std::unique_ptr<EnvironmentSection>;

protected: // Attributes
    SectionPtr m_store;
    double m_secondsToLive;
    mutable IntCacheType m_intCache;
    mutable StringCacheType m_strCache;
    LoggerPtr m_log;

public: // ...structors
    /// Cached values in \c store, refreshed after \c secondsToLive
    CachedSection(SectionPtr store, double secondsToLive)
        : BaseType(store->GetSectionName())
        , m_store(std::move(store))
        , m_secondsToLive(secondsToLive)
        , m_log(GetLogger("CachedSection"))
        {}

public: // Accessors
    /// The integer value after the \c name= entry in this section.
    int GetInt(const char* name, int defaultValue) const
    {
        LOG4CXX_DEBUG(m_log, "GetInt: " << name);
        int result;
        StringType key(name);
        IntCacheType::const_iterator pItem = m_intCache.find(key);
        if (m_intCache.end() != pItem &&
            (!pItem->second.second.IsValid() || EventTime::Now() < pItem->second.second))
            result = pItem->second.first;
        else
        {
            if (!m_store->GetValue(name, result))
                result = defaultValue;
            EventTime until;
            if (0 < m_secondsToLive)
                until = EventTime::Now().AtDurationOffset(m_secondsToLive);
            m_intCache[key] = AgedInt(result, until);
        }
        return result;
    }

    /// The string value after the \c name= entry in this section.
    StringType GetString(const char* name, const char* defaultValue) const
    {
        StringType result;
        StringType key(name);
        StringCacheType::const_iterator pItem = m_strCache.find(key);
        if (m_strCache.end() != pItem &&
            (!pItem->second.second.IsValid() || EventTime::Now() < pItem->second.second))
            result = pItem->second.first;
        else
        {
            result = m_store->GetString(name, defaultValue);
            EventTime until;
            if (0 < m_secondsToLive)
                until = EventTime::Now().AtDurationOffset(m_secondsToLive);
            m_strCache[key] = AgedString(result, until);
        }
        return result;
    }

public: // Methods
    /// Associate \c value with the variable \c name
    bool PutInt(const char* name, int value)
    {
        LOG4CXX_DEBUG(m_log, "PutInt: " << name << '=' << value);
        EventTime until;
        if (0 < m_secondsToLive)
            until = EventTime::Now().AtDurationOffset(m_secondsToLive);
        m_intCache[name] = AgedInt(value, until);
        return m_store->PutInt(name, value);
    }

    /// Associate \c value with the variable \c name
    bool PutString(const char* name, const char* value)
    {
        LOG4CXX_DEBUG(m_log, "PutString: " << name << '=' << value);
        EventTime until;
        if (0 < m_secondsToLive)
            until = EventTime::Now().AtDurationOffset(m_secondsToLive);
        m_strCache[name] = AgedString(value, until);
        return m_store->PutString(name, value);
    }
};

// The section of environment cached variables with name \c key, optionally refreshed after \c secondsToLive.
    Environment::SectionPtr
Environment::GetCachedSection(const StringType& key, double secondsToLive) const
{
    return std::make_unique<CachedSection>(this->GetSection(key), secondsToLive);
}

// The per process environment variables.
    EnvironmentSection&
Environment::GetProcessVariables()
{
    static ProcessEnvironment env;
    return env;
}


// A string with %Var% instances in \c inString replaced by the value of the named process variable.
    Environment::StringType
Environment::GetExpandedString(const StringType& inString)
{
    static const char uniqueStr[] = "2389179B-E078-437f-A25B-CDA740A28ADB";
    StringType result = inString;
    size_t varStart(0), varEnd(0), szDone(0);
    while (result.npos != (varStart = result.find('%', szDone))
        && result.npos != (varEnd = result.find('%', varStart + 1)))
    {
        StringType varName = result.substr(varStart + 1, varEnd - varStart - 1);
        StringType varValue = GetProcessVariables().GetString(varName.c_str(), uniqueStr);
        if (varValue != uniqueStr)
            result = result.substr(0, varStart) + varValue + result.substr(varEnd + 1);
        else // Skip the unexpanded variable
            szDone = varEnd + 1;
    }
    return result;
}

// Ensure changes are allowed to the local file composed from \c stem and \c ext
    void
Environment::MakeLocalFileWritable(const StringType& stem, const StringType& ext)
{
    boost::filesystem::path envFile(GetLocalFile(stem, ext));
    boost::filesystem::path configPath(GetConfigDataPath());
    LOG4CXX_DEBUG(m_log, "MakeLocalFileWritable: envFile " << envFile << " configPath " << configPath);
    if (!equivalent(envFile.parent_path(), configPath))
    {
        boost::filesystem::path writableFile = configPath / envFile.filename();
        boost::system::error_code ec;
        boost::filesystem::copy_file(envFile, writableFile, ec);
        LOG4CXX_DEBUG(m_log, "MakeLocalFileWritable: writableFile " << writableFile);
    }
}

} // namespace Foundation

