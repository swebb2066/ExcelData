#include <Foundation/WindowsEnvironment.h>
#include <Foundation/Logger.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <windows.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace Foundation
{

// Retrieve the system error number for the last-error code
struct LastErrorMessage
{
    const char* m_pFunction;
    std::string m_section;
    LastErrorMessage(const char* pFunction, const std::string& section)
        : m_pFunction(pFunction)
        , m_section(section)
        {}
    void Write(std::ostream& os) const
    {
        os << m_section << ": " << m_pFunction << " failed: 0x" << std::hex << GetLastError();
    }
};
    inline std::ostream&
operator<<(std::ostream& os, const LastErrorMessage& err)
{
    err.Write(os);
    return os;
}

/// A section of the registry
class RegistryEnvironmentSection : public EnvironmentSection
{
protected: // Attributes
    const StringType& m_baseName;
    const StringType& m_partitionName;

public: // ...structors
    RegistryEnvironmentSection(const StringType& baseName, const StringType& partitionName, const StringType& key)
        : EnvironmentSection(key)
        , m_baseName(baseName)
        , m_partitionName(partitionName)
    {
    }

public: // Hooked accessors
    /// The integer value after the \c name= entry in this section.
    int GetInt(const char* name, int defaultValue) const override
    {
        int result;
        if (!GetRegistryInt(name, result))
            result = defaultValue;
        LOG4CXX_DEBUG(m_log, "GetInt: from " << m_section << ": " << name  << '=' << result);
        return result;
    }

    /// The string value after the \c name= entry in this section.
    StringType GetString(const char* name, const char* defaultValue) const override
    {
        StringType result;
        char buf[1000];
        if (GetRegistryString(name, buf, sizeof (buf)))
            result = buf;
        LOG4CXX_DEBUG(m_log, "GetString: from " << m_section << ": " << name << '=' << result);
        return result;
    }

public: // Accessors
    /// The integer value after the \c name= entry in this section.
    int GetRegistryInt(const char* name, int& result) const
    {
        LOG4CXX_TRACE(m_log, "GetInt: HKLM\\" << m_section << '\\' << name);
        bool found = false;
        HKEY handle;
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, "Software", &handle))
        {
            DWORD type, value;
            DWORD cbSize = sizeof (DWORD);
            StringType key = m_baseName + '\\' + m_partitionName + '\\' + m_section;
            if (ERROR_SUCCESS == RegGetValue(handle, key.c_str(), name, RRF_RT_DWORD, &type, &value, &cbSize))
            {
                result = value;
                LOG4CXX_DEBUG(m_log, "GetRegistryInt: HKLM\\" << key << '=' << value);
                found = true;
            }
            RegCloseKey(handle);
        }
        return found;
    }

    // Get a string value for the key \c name from the registry hive at \c section
    bool GetRegistryString(const char* name, char* resultBuf, size_t resultSize) const
    {
        LOG4CXX_TRACE(m_log, "GetRegistryString: HKLM\\" << m_section << '\\' << name);
        bool found = false;
        HKEY handle;
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, "Software", &handle))
        {
            DWORD type = REG_SZ;
            DWORD cbSize = DWORD(resultSize - 1);
            StringType key = m_baseName + '\\' + m_partitionName + '\\' + m_section;
            if (ERROR_SUCCESS == RegGetValue(handle, key.c_str(), name, RRF_RT_REG_SZ, &type, resultBuf, &cbSize))
            {
                resultBuf[cbSize] = 0;
                LOG4CXX_DEBUG(m_log, "GetRegistryString: HKLM\\" << key << '=' << resultBuf);
                found = true;
            }
            RegCloseKey(handle);
        }
        return found;
    }

public: // Hooked methods
    /// Associate \c value with the variable \c name
    bool PutInt(const char* name, int value) override
    {
        LOG4CXX_DEBUG(m_log, "PutInt: " << name << '=' << value << " to " << m_section);
        bool result = false;
        HKEY handle;
        StringType sectionkey = "Software\\" + m_baseName + '\\' + m_partitionName;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sectionkey.c_str(), 0, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, &handle))
        {
            int32_t dw = value;
            if (ERROR_SUCCESS == RegSetKeyValue(handle, m_section.c_str(), name, REG_DWORD, &dw, 4))
                result = true;
            else
                LOG4CXX_WARN(m_log, LastErrorMessage("Set value", m_section + '\\' + name));
            RegCloseKey(handle);
        }
        else
            LOG4CXX_WARN(m_log, LastErrorMessage("Open for write", sectionkey));
        return result;
    }

    /// Associate \c value with the variable \c name
    bool PutString(const char* name, const char* value) override
    {
        LOG4CXX_DEBUG(m_log, "PutString: " << name << '=' << value << " to " << m_section);
        bool result = false;
        HKEY handle;
        StringType sectionkey = "Software\\" + m_baseName + '\\' + m_partitionName;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sectionkey.c_str(), 0, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, &handle))
        {
            DWORD cbSize = strlen(value) + 1;
            if (ERROR_SUCCESS == RegSetKeyValue(handle, m_section.c_str(), name, REG_SZ, value, cbSize))
                result = true;
            else
                LOG4CXX_WARN(m_log, LastErrorMessage("Set value", m_section + '\\' + name));
            RegCloseKey(handle);
        }
        else
            LOG4CXX_WARN(m_log, LastErrorMessage("Open for write", sectionkey));
        return result;
    }

private:
    static log4cxx::LoggerPtr m_log;
};

    log4cxx::LoggerPtr
RegistryEnvironmentSection::m_log(log4cxx::Logger::getLogger("RegistryEnvironmentSection"));

/// A section of the current configuration
class WindowsEnvironment::Section : public EnvironmentSection
{
    typedef EnvironmentSection BaseType;
protected: // Attributes
    const StringType& m_primeFile;
    const StringType& m_defaultFile;
    RegistryEnvironmentSection m_registry;

public: // ...structors
    Section(const WindowsEnvironment& parent, const StringType& file, const StringType& key, const StringType& defaultFile)
        : EnvironmentSection(key)
        , m_primeFile(file)
        , m_defaultFile(defaultFile)
        , m_registry(parent.GetUniqueName(), parent.GetPartitionName(), key)
    {
        LOG4CXX_TRACE(m_log, "create: " << m_section << " primeFile " << m_primeFile << " defaultFile " << m_defaultFile);
    }

public: // Accessors
    /// The integer value after the \c name= entry in this section.
    int GetInt(const char* name, int defaultValue) const
    {
        const char* source;
        char buf[1000];
        int result;
        if (0 != GetPrivateProfileString(m_section.c_str(), name, "", buf, sizeof(buf), m_primeFile.c_str()))
            result = GetPrivateProfileInt(m_section.c_str(), name, defaultValue, source = m_primeFile.c_str());
        else if (0 != GetPrivateProfileString(m_section.c_str(), name, "", buf, sizeof(buf), m_defaultFile.c_str()))
            result = GetPrivateProfileInt(m_section.c_str(), name, defaultValue, source = m_defaultFile.c_str());
        else if (m_registry.GetRegistryInt(name, result))
            source = "Registry";
        else
        {
            source = "Default";
            return BaseType::GetInt(name, defaultValue);
        }
        LOG4CXX_DEBUG(m_log, "GetInt: from " << m_section << " in " << source << ": " << name  << '=' << result);
        return result;
    }

    /// The string value after the \c name= entry in this section.
    StringType GetString(const char* name, const char* defaultValue) const
    {
        StringType result;
        const char* source;
        char buf[1000];
        if (0 != GetPrivateProfileString(m_section.c_str(), name, "", buf, sizeof(buf), source = m_primeFile.c_str()))
            result = buf;
        else if (0 != GetPrivateProfileString(m_section.c_str(), name, "", buf, sizeof(buf), source = m_defaultFile.c_str()))
            result = buf;
        else if (m_registry.GetRegistryString(name, buf, sizeof (buf)))
        {
            source = "Registry";
            result = buf;
        }
        else
        {
            source = "Default";
            result = BaseType::GetString(name, defaultValue);
        }
        LOG4CXX_DEBUG(m_log, "GetString: from " << m_section << " in " << source << ": " << name << '=' << result);
        return result;
    }

public: // Methods
    /// Associate \c value with the variable \c name
    bool PutInt(const char* name, int value)
    {
        LOG4CXX_DEBUG(m_log, "PutInt: " << name << '=' << value << " to " << m_section);
        std::stringstream buf;
        buf << value;
        return PutString(name, buf.str().c_str());
    }

    /// Associate \c value with the variable \c name
    bool PutString(const char* name, const char* value)
    {
        LOG4CXX_DEBUG(m_log, "PutString: " << name << '=' << value << " to " << m_section << " in " << m_primeFile);
        return WritePrivateProfileString(m_section.c_str(), name, value, m_primeFile.c_str()) ? true : false;
    }

private:
    static log4cxx::LoggerPtr m_log;
};

    log4cxx::LoggerPtr
WindowsEnvironment::Section::m_log(log4cxx::Logger::getLogger("WindowsEnvironmentSection"));

// A .ini file named from \c pathPrefix or otherwise like this executable
WindowsEnvironment::WindowsEnvironment
    ( const StringType& pathPrefix
    , const StringType& primeSource
    , const StringType& defaultSource
    )
    : Environment(pathPrefix)
    , m_primeFile(primeSource.empty() ? GetPrimeSource() : GetLocalFile(primeSource, ".ini"))
    , m_defaultFile(defaultSource.empty() ? GetAlternateSource() : GetLocalFile(defaultSource, ".ini"))
{
    LOG4CXX_DEBUG(m_log, "create: " << this << " primeFile: " << m_primeFile <<  " alternateFile: " << m_defaultFile);
}

// Release resources
WindowsEnvironment::~WindowsEnvironment()
{
    LOG4CXX_DEBUG(m_log, "destroy: " << this);
}

// The section of default environment variables with name \c key
    Environment::SectionPtr
WindowsEnvironment::GetDefaultSection(const StringType& key) const
{
    LOG4CXX_DEBUG(m_log, "GetDefaultSection: " << key);
    return std::make_unique<RegistryEnvironmentSection>(m_baseName, m_partitionName, key);
}

// The \c key section
    Environment::SectionPtr
WindowsEnvironment::GetSection(const StringType& key) const
{
    LOG4CXX_DEBUG(m_log, "GetSection: " << key);
    return std::make_unique<Section>(*this, m_primeFile, key, m_defaultFile);
}

// The source of values from the section of environment variables with name \c sectionKey.
    Environment::StringType
WindowsEnvironment::GetSource(const StringType& sectionKey) const
{
    StringType source = m_primeFile;
    char buf[1000];
    if (!sectionKey.empty() && 0 == GetPrivateProfileSection(sectionKey.c_str(), buf, sizeof(buf), m_primeFile.c_str())
        && 0 != GetPrivateProfileSection(sectionKey.c_str(), buf, sizeof(buf), m_defaultFile.c_str()))
        source = m_defaultFile;
    return source;
}

// Set the primary source of the environment variable values
    void
WindowsEnvironment::PutPrimeSource(const StringType& source)
{
    LOG4CXX_DEBUG(m_log, "PutPrimeSource: " << source);
    m_primeFile = source;
}

/*
BOOL WINAPI GetVolumeInformation(
  _In_opt_   LPCTSTR lpRootPathName,
  _Out_opt_  LPTSTR lpVolumeNameBuffer,
  _In_       DWORD nVolumeNameSize,
  _Out_opt_  LPDWORD lpVolumeSerialNumber,
  _Out_opt_  LPDWORD lpMaximumComponentLength,
  _Out_opt_  LPDWORD lpFileSystemFlags,
  _Out_opt_  LPTSTR lpFileSystemNameBuffer,
  _In_       DWORD nFileSystemNameSize
);
*/
// The path prefix of a writable removable drive
    Environment::StringType
WindowsEnvironment::GetRemoveableMediaPathPrefix()
{
    LOG4CXX_DEBUG(m_log, "GetRemoveableMediaPathPrefix:");
    StringType result;
    char buffer[1024];
    GetLogicalDriveStrings(sizeof (buffer), buffer);
    for (char* pName = buffer; *pName != 0; pName += strlen(pName) + 1)
    {
        unsigned driveType = GetDriveType(pName);
        char volumeName[256+1];
        DWORD volumeSerialNumber = 0;
        DWORD maximumComponentLength = 0;
        DWORD fileSystemFlags;
        char fileSystemName[256+1];
        GetVolumeInformation
            ( pName
            , volumeName, sizeof (volumeName) - 1
            , &volumeSerialNumber
            , &maximumComponentLength
            , &fileSystemFlags
            , fileSystemName, sizeof (fileSystemName) - 1
            );
        LOG4CXX_DEBUG(m_log, "GetRemoveableMediaPathPrefix: " << pName
            << " driveType " << driveType
            << " volumeName " << volumeName
            << " fileSystemName " << fileSystemName
            << " volumeSerialNumber " << volumeSerialNumber
            << " maximumComponentLength " << maximumComponentLength
            << " fileSystemFlags " << std::hex << fileSystemFlags
            );
        if (driveType == 2 // Removable?
            && *fileSystemName // Non-phantom?
            && !(fileSystemFlags & FILE_READ_ONLY_VOLUME))
        {
            result = pName;
            if (result.back() != '\\')
                result.push_back('\\');
        }
    }
    return result;
}

// Dismount the writable removable drive
    bool
WindowsEnvironment::DismountRemoveableMediaPath()
{
    LOG4CXX_TRACE(m_log, "DismountRemoveableMediaPath:");
    StringType volumeName("\\\\.\\");
    volumeName += GetRemoveableMediaPathPrefix();
    if (volumeName.back() == '\\')
        volumeName.pop_back();
    LOG4CXX_DEBUG(m_log, "DismountRemoveableMediaPath: " << volumeName);
    bool result = false;
    HANDLE hVolume = CreateFile
        ( volumeName.c_str()
        , GENERIC_READ | GENERIC_WRITE
        , FILE_SHARE_READ | FILE_SHARE_WRITE
        , NULL
        , OPEN_EXISTING
        , 0
        , NULL
        );
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        LOG4CXX_WARN(m_log, "Failed to open " << volumeName);
    }
    else
    {
        DWORD dwBytesReturned;
        PREVENT_MEDIA_REMOVAL PMRBuffer;
        PMRBuffer.PreventMediaRemoval = false;
        if (!DeviceIoControl
            ( hVolume
            , FSCTL_LOCK_VOLUME
            , NULL, 0
            , NULL, 0
            , &dwBytesReturned
            , NULL
            )
           )
           {
            LOG4CXX_WARN(m_log, "Cannot dismount " << volumeName << " while in use");
           }
        else if (!DeviceIoControl
                ( hVolume
                , FSCTL_DISMOUNT_VOLUME
                , NULL, 0
                , NULL, 0
                , &dwBytesReturned
                , NULL
                )
               )
               {
            LOG4CXX_WARN(m_log, "Failed to dismount " << volumeName << " - error " << std::hex << GetLastError());
               }
        else if (!DeviceIoControl
                ( hVolume
                , IOCTL_STORAGE_MEDIA_REMOVAL
                , &PMRBuffer, sizeof(PREVENT_MEDIA_REMOVAL)
                , NULL, 0
                , &dwBytesReturned
                , NULL
                )
               )
               {
            LOG4CXX_WARN(m_log, "Cannot enable removal of " << volumeName << " - error " << std::hex << GetLastError());
               }
        else
            result = true;
        CloseHandle(hVolume);
    }
    return result;
}

    log4cxx::LoggerPtr
WindowsEnvironment::m_log(log4cxx::Logger::getLogger("WindowsEnvironment"));

//////////////////////////////////////////
// Environment implementation

// Writable environment variables initialised from \c sourceName
    Environment::Ptr
Environment::GetWritableSource(const StringType& sourceName)
{
    MakeLocalFileWritable(sourceName, ".ini");
    return std::make_unique<WindowsEnvironment>(StringType(), sourceName);
}

// An environment defined by a .ini file named like the executable
    Environment*
Environment::GetInstance()
{
    if (m_instance == 0)
    {
        m_instance = new WindowsEnvironment();
        atexit(DeleteInstance);
    }
    return m_instance;
}

/// The path prefix for temporary files
    const Environment::StringType&
Environment::GetTempFilePathPrefix()
{
    static StringType tempPath;
    if (tempPath.empty())
    {
        tempPath = GetProcessVariables().GetString("TEMP", "");
        tempPath += '\\';
    }
    return tempPath;
}

/// The path prefix for user files
    const Environment::StringType&
Environment::GetUserDirectoryPrefix()
{
    static StringType userDirPath;
    if (userDirPath.empty())
    {
        userDirPath = GetProcessVariables().GetString("USERPROFILE", "");
        userDirPath += '\\';
    }
    return userDirPath;
}

// The identifier of the current user
    Environment::StringType
Environment::GetLoginName()
{
    static const int BUFFER_SIZE = 200;
    char buf[BUFFER_SIZE];
    DWORD  bufCharCount = BUFFER_SIZE;
    GetUserName(buf, &bufCharCount);
    return buf;
}

// The identifier for the current system
    Environment::StringType
Environment::GetSystemName()
{
    static const int BUFFER_SIZE = 200;
    char buf[BUFFER_SIZE];
    DWORD  bufCharCount = BUFFER_SIZE;
    GetComputerName(buf, &bufCharCount);
    return buf;
}

} // namespace Foundation
