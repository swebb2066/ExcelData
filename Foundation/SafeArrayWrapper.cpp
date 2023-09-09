#include "stdafx.h"
#include "SafeArrayWrapper.h"

    log4cxx::LoggerPtr
SafeArrayData::m_log(log4cxx::Logger::getLogger("SafeArrayData"));

/// Generate an empty SafeArray of \c type
    SAFEARRAY*
MakeEmptySafeArray(int type)
{
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = 0;
    return SafeArrayCreate(type, 1, rgsabound);
}
