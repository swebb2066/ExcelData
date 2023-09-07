#ifndef _EVENT_TIME_H_
#define _EVENT_TIME_H_

#include <Foundation/Config.h>
#include <boost/thread/xtime.hpp>
#include <ostream>

namespace Foundation
{

/// Elapsed time
#if defined(BOOST_NO_INT64_T)
    using TimeDuration = boost::int32_t; // Milliseconds
#else
    using TimeDuration = boost::int64_t; // Milliseconds
#endif

/// An absolute time in milliseconds
class EventTime
{
public: // Constants
    enum Constants
    { MillisecsPerSec = 1000
    , MicrosecsPerMillisec = 1000
    , MicrosecsPerSec = (MillisecsPerSec * MillisecsPerSec)
    , NsecsPerMicrosec = 1000
    , NsecsPerSec = (MicrosecsPerSec * NsecsPerMicrosec)
    , NsecsPerMillisec = 1000000
    , SecsPerDay = (24*60*60)
    };

public: // Types
    using SecondCountType = TimeDuration;
    using MillisecondCountType = long;
    using MicrosecondCountType = long;
    using TimeType = boost::xtime;

protected: // Attributes
    TimeType m_utc; //!< The time

protected: // Class attributes
    static EventTime m_eventBase; //!< The time at process startup

public: // ...structors
    /// A time in the dim distant past (1-Jan-1970?)
    EventTime()
    {
        m_utc.sec = 0;
        m_utc.nsec = 0;
    }
    /// A time described by \c sec and \c microsec
    EventTime(const SecondCountType& sec, const MicrosecondCountType& microsec)
    {
        m_utc.sec = sec;
        m_utc.nsec = microsec * NsecsPerMicrosec;
    }

public: // Operators
    /// Is \c other the same as this?
    bool operator==(const EventTime& other) const
    {
        return boost::xtime_cmp(m_utc, other.m_utc) == 0;
    }

    /// Is \c other prior to this?
    bool operator<(const EventTime& other) const
    {
        return boost::xtime_cmp(m_utc, other.m_utc) < 0;
    }

    /// Is \c other prior or the same as this?
    bool operator<=(const EventTime& other) const
    {
        return boost::xtime_cmp(m_utc, other.m_utc) <= 0;
    }

    /// Is \c other after this?
    bool operator>(const EventTime& other) const
    {
        return boost::xtime_cmp(m_utc, other.m_utc) > 0;
    }

    /// Is \c other the same or after this?
    bool operator>=(const EventTime& other) const
    {
        return boost::xtime_cmp(m_utc, other.m_utc) >= 0;
    }

    /// The elapsed milliseconds since \c other.
    MillisecondCountType operator-(const EventTime& other) const;

    /// A time \c offset milliseconds after this.
    EventTime operator+(const MillisecondCountType& offset) const
    {
        return AtOffset(offset / MillisecsPerSec, (offset % MillisecsPerSec) * MicrosecsPerMillisec);
    }

    /// A time \c offset milliseconds before this.
    EventTime operator-(const MillisecondCountType& offset) const
    {
        return AtOffset(-offset / MillisecsPerSec, (-offset % MillisecsPerSec) * MicrosecsPerMillisec);
    }

    /// Is this a valid time?
    bool IsValid() const { return m_utc.sec != 0; }

public: // Accessors
    /// The time after \c seconds have elapsed.
    template <typename RealType>
    EventTime AtDurationOffset(const RealType& seconds) const
    {
        double n, f;
        f = modf(seconds, &n);
        return AtOffset(SecondCountType(n), MicrosecondCountType(f * double(MicrosecsPerSec)));
    }

    /// Elapsed milliseconds since this process started.
    MillisecondCountType GetMillisecondOffset() const;

    /// Microseconds component of this time.
    MicrosecondCountType MicrosecondCount() const { return MicrosecondCountType((m_utc.nsec + NsecsPerMicrosec / 2) / NsecsPerMicrosec); }

    /// Elapsed milliseconds since midnight today.
    MillisecondCountType MillisecondsSinceMidnight() const;

    /// The elapsed seconds since \c other.
    double GetDurationSince(const EventTime& other) const;

    /// Seconds since time began.
    SecondCountType SecondCount() const { return SecondCountType(m_utc.sec); }

    /// Format the event time onto the output stream
    void Write(std::ostream& os) const;

    /// The underlying implementation object
    const TimeType& boost_xtime() const { return m_utc; }

public: // Factory methods

    /// The time this process started
    static EventTime BaseTime() { return m_eventBase; }

    /// A time at \c offset milliseconds from the time this process started
    static EventTime AtMillisecondOffset(const MillisecondCountType offset);

    /// A time \c offset milliseconds since midnight today.
    static EventTime TodayAtMillisecond(const MillisecondCountType offset);

    /// The current time.
    static EventTime Now();

    /// The time \c secs and \c millsecs after the current time.
    static EventTime NowPlus(int secs, int millisecs = 0);

    /// Suspend this thread for \c secs seconds and \c millsecs milliseconds.
    static void Delay(int secs, int millisecs = 0);

    /// Suspend this thread until the time \c until.
    static void DelayUntil(const EventTime& until);

protected:
    /// The time after \c seconds have elapsed.
    EventTime AtOffset(const SecondCountType& seconds, const MicrosecondCountType& microsec) const;

};

inline std::ostream& operator<<(std::ostream& s, const EventTime& t)
    { t.Write(s); return s;  }

} // namespace Foundation


#endif //_EVENT_TIME_H_
