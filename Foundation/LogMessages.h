#ifndef FOUNDATION_LOG_MESSAGES_HDR_
#define FOUNDATION_LOG_MESSAGES_HDR_
#include <log4cxx/logger.h>

namespace Foundation
{

/// Make a clean exit log entry
void CleanExitLogMessage(const log4cxx::LoggerPtr& log = log4cxx::LoggerPtr());

/// Add a message to separate log messages of this process instance
void UserAndVersionLogMessage(const log4cxx::LoggerPtr& log = log4cxx::LoggerPtr());

} // namespace Foundation

#endif // FOUNDATION_LOG_MESSAGES_HDR_
