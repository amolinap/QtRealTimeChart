/**
 * @file
 *   @brief Definition of TIME file
 *
 *   @author Alejandro Molina Pulido <am.alex09@gmail.com>
 */
#ifndef TIME_H
#define TIME_H

#include <QObject>
#include <QDateTime>


/**
 * @brief Object TIME
 *
 * This static class represents the TIME into Application
 */
class QtConfiguration : public QObject
{
public:
    /** @brief Get ground time in milliseconds */
    static quint64 groundTimeUsecs()
    {
        QDateTime time = QDateTime::currentDateTime();
        time = time.toUTC();

        /* Return seconds and milliseconds, in milliseconds unit */
        quint64 microseconds = time.toTime_t() * static_cast<quint64>(1000000);

        return static_cast<quint64>(microseconds + (time.time().msec()*1000));
    }

    static QString timeUpFromUsecs(quint32 usecs)
    {
        quint64 filterTime = usecs / 1000;
        int sec = static_cast<int>(filterTime - static_cast<int>(filterTime / 60) * 60);
        int min = static_cast<int>(filterTime / 60)-((static_cast<int>(filterTime / 60)/60)*60);
        int hours = static_cast<int>((filterTime / 60)/60);
        QString timeText;

        return timeText.sprintf("%02d:%02d:%02d", hours, min, sec);
    }

    /**
         * Convert milliseconds to an QDateTime object. This method converts the amount of
         * milliseconds since 1.1.1970, 00:00 UTC (unix epoch) to a QDateTime date object.
         *
         * @param msecs The milliseconds since unix epoch (in Qt unsigned 64bit integer type quint64)
         * @return The QDateTime object set to corresponding date and time
         * @deprecated Will the replaced by time helper class
         **/
        static QDateTime msecToQDateTime(quint64 msecs)
        {
            QDateTime time = QDateTime();
            /* Set date and time depending on the seconds since unix epoch,
                 * integer division truncates the milliseconds */
            time.setTime_t(msecs / 1000);
            /* Add the milliseconds, modulo returns the milliseconds part */
            return time.addMSecs(msecs % 1000);
        }

        /**
         * @brief Convenience method to get the milliseconds time stamp for now
         *
         * The timestamp is created at the instant of calling this method. It is
         * defined as the number of milliseconds since unix epoch, which is
         * 1.1.1970, 00:00 UTC.
         *
         * @return The number of milliseconds elapsed since unix epoch
         * @deprecated Will the replaced by time helper class
         **/
        static quint64 getGroundTimeNow()
        {
            QDateTime time = QDateTime::currentDateTime();
            time = time.toUTC();
            /* Return seconds and milliseconds, in milliseconds unit */
            quint64 milliseconds = time.toTime_t() * static_cast<quint64>(1000);
            return static_cast<quint64>(milliseconds + time.time().msec());
        }
};
#endif // TIME_H
