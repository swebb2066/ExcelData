#include <Foundation/LogMessages.h>
#include <Foundation/Environment.h>
#include <product_version.h>

namespace Foundation
{

/// Make a clean exit log entry
void CleanExitLogMessage(const log4cxx::LoggerPtr& log)
{
    log4cxx::LoggerPtr mylog = log;
    if (0 == mylog)
        mylog = log4cxx::Logger::getRootLogger();
    std::string marker("----------------------");
    std::string fairwell
        ( marker
        + " Goodbye "
        + marker
        );
    std::string introLine(fairwell.size(), '-');
    LOG4CXX_INFO(mylog
        ,  '\n' << introLine
        << '\n' << fairwell
        << '\n' << introLine
        );
}

/// Add a message to separate this process instance log messages
void UserAndVersionLogMessage(const log4cxx::LoggerPtr& log)
{
    log4cxx::LoggerPtr mylog = log;
    if (0 == mylog)
        mylog = log4cxx::Logger::getRootLogger();
    std::string marker("----------------------");
    std::string greeting
        ( marker
        + " Welcome " + Environment::GetLoginName()
        + " on " + Environment::GetSystemName()
        + " Version " + getProductVersion() + " "
        + marker
        );
    std::string introLine(greeting.size(), '-');
    LOG4CXX_INFO(mylog
        ,  '\n' << introLine
        << '\n' << greeting
        << '\n' << introLine
        );
}

} // namespace Foundation
