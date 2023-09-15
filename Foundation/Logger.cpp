#include <Foundation/Logger.h>
#include <log4cxx/logmanager.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/xml/domconfigurator.h>
#include <Foundation/Environment.h>

namespace Foundation
{

// A .xml file with current process name in the in the current directory
// or the same directory as this executable
bool GetXMLFile(log4cxx::File& xmlFile)
{
    std::string processName = Foundation::Environment::PathPrefix();
    xmlFile = processName + ".xml";
    // Check the current directory first
    log4cxx::File localxmlFile(xmlFile.getName());
    log4cxx::helpers::Pool p;
    if (localxmlFile.exists(p))
        xmlFile = localxmlFile;
    return xmlFile.exists(p);
}

// A .properties file with current process name in the in the current directory
// or the same directory as this executable
bool GetPropertiesFile(log4cxx::File& propertyFile)
{
    std::string processName = Foundation::Environment::PathPrefix();
    propertyFile = processName + ".properties";
    // Check the current directory first
    log4cxx::File localPropertyFile(propertyFile.getName());
    log4cxx::helpers::Pool p;
    if (localPropertyFile.exists(p))
        propertyFile = localPropertyFile;
    return propertyFile.exists(p);
}

// Initialise log4cxx by reading the properties file
void InitialiseLogger(fs::path* configFileUsed)
{
    static struct log4cxx_initializer
    {
        log4cxx::File f;
        log4cxx_initializer()
        {
            if (GetXMLFile(f))
				log4cxx::xml::DOMConfigurator::configure(f.getPath());
            else if (GetPropertiesFile(f))
                log4cxx::PropertyConfigurator::configure(f);
            else
                log4cxx::BasicConfigurator::configure();
        }
        ~log4cxx_initializer()
        {
            log4cxx::LogManager::shutdown();
        }
    } initialiser;
    if (configFileUsed)
        *configFileUsed = absolute(fs::path(initialiser.f.getPath()));
}

// Retrieve the \c name logger pointer.
// Configure Log4cxx on the first call.
LoggerPtr GetLogger(const std::string& name)
{
    InitialiseLogger();
    return name.empty()
        ? log4cxx::LogManager::getRootLogger()
        : log4cxx::LogManager::getLogger(name);
}

} // namespace Foundation
