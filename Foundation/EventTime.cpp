
#include <Foundation/EventTime.h>
#include <boost/thread/thread.hpp>
#include <ostream>
#include <iomanip>

namespace Foundation
{

// Initialise an event at system start
    EventTime
EventTime::m_eventBase = EventTime::Now();

    EventTime::MillisecondCountType
EventTime::operator-(const EventTime& right) const
{
    TimeType result;
    result.sec = m_utc.sec - right.m_utc.sec;
    result.nsec = m_utc.nsec - right.m_utc.nsec;
    return MillisecondCountType((result.sec * MillisecsPerSec) + (result.nsec / NsecsPerMillisec));
}

    EventTime
EventTime::AtOffset(const SecondCountType& seconds, const MicrosecondCountType& microsec) const
{
    EventTime result;
    result.m_utc.sec = m_utc.sec + seconds;
    result.m_utc.nsec = boost::int_fast32_t(m_utc.nsec + (microsec * NsecsPerMicrosec));
    if (0 < result.m_utc.sec)
    {
        while (result.m_utc.nsec < 0)
            result.m_utc.nsec += NsecsPerSec, --result.m_utc.sec;
    }
    while (NsecsPerSec < result.m_utc.nsec)
        result.m_utc.nsec -= NsecsPerSec, ++result.m_utc.sec;
    return result;
}

// The elapsed seconds since \c other.
    double
EventTime::GetDurationSince(const EventTime& right) const
{
    double result;
    result = double(m_utc.sec - right.m_utc.sec) + double(m_utc.nsec - right.m_utc.nsec) / double(NsecsPerSec);
    return result;
}

    EventTime::MillisecondCountType
EventTime::GetMillisecondOffset() const
{
    return long((*this) - m_eventBase);
}

    EventTime
EventTime::AtMillisecondOffset(const MillisecondCountType n)
{
    return m_eventBase + n;
}

    EventTime::MillisecondCountType
EventTime::MillisecondsSinceMidnight() const
{
    EventTime midnight(Now());
    midnight.m_utc.sec -= midnight.m_utc.sec % SecsPerDay;
    midnight.m_utc.nsec = 0;
    return long((*this) - midnight);
}

    EventTime
EventTime::TodayAtMillisecond(const MillisecondCountType n)
{
    EventTime result(Now());
    result.m_utc.sec -= (result.m_utc.sec % SecsPerDay) - (n / MillisecsPerSec);
    result.m_utc.nsec = (n % MillisecsPerSec) * NsecsPerMillisec;
    return result;
}

    EventTime
EventTime::Now()
    {
        EventTime result;
#if BOOST_VERSION < 105000
        boost::xtime_get(&result.m_utc, boost::TIME_UTC);
#else
        boost::xtime_get(&result.m_utc, boost::TIME_UTC_);
#endif
        return result;
    }

    EventTime
EventTime::NowPlus(int secs, int millisecs)
{
    EventTime result(EventTime::Now().AtOffset(secs, millisecs * MicrosecsPerMillisec));
    return result;
}

    void
EventTime::Delay(int secs, int millisecs)
{
    EventTime until(NowPlus(secs, millisecs));
    DelayUntil(until);
}

    void
EventTime::DelayUntil(const EventTime& until)
{
    boost::thread::sleep(until.boost_xtime());
}

// Format the event time onto the output stream
    void
EventTime::Write(std::ostream& os) const
{
    os << int(m_utc.sec) << "." << std::setw(3) << std::setfill('0') << int(m_utc.nsec / NsecsPerMillisec);
}

} // namespace Foundation

