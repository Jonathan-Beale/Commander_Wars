#ifndef PERFTRACE_H
#define PERFTRACE_H

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QMutex>
#include <QPair>
#include <QString>

#include <algorithm>
#include <vector>

/**
 * @brief startup benchmarking. If the environment variable COW_PERF_TRACE holds a file name, each mark appends the line
 * "<ms since epoch>;<process id>;<name>" to this file. Does nothing if the variable isn't set.
 * The marks are evaluated by hex_balance/startup_waterfall.py
 */
namespace PerfTrace
{
    inline bool enabled()
    {
        static const bool traceEnabled = !qgetenv("COW_PERF_TRACE").isEmpty();
        return traceEnabled;
    }
    inline void mark(const QString & name);
    /**
     * @brief markTrue mark which can be used inside of a condition
     */
    inline bool markTrue(const char* name);
    /**
     * @brief script call statistics: calls and nanoseconds per function name
     */
    struct JsStats
    {
        QHash<QString, QPair<qint64, qint64>> data;
        QMutex mutex;
    };
    inline JsStats & jsStats()
    {
        static JsStats stats;
        return stats;
    }
    inline void addJsCall(const QString & name, qint64 nanoseconds)
    {
        JsStats & stats = jsStats();
        QMutexLocker locker(&stats.mutex);
        auto & entry = stats.data[name];
        entry.first += 1;
        entry.second += nanoseconds;
    }
    /**
     * @brief dumpJsCalls writes the summed up script calls since the start as marks
     */
    inline void dumpJsCalls()
    {
        if (!enabled())
        {
            return;
        }
        std::vector<std::pair<QString, QPair<qint64, qint64>>> entries;
        qint64 calls = 0;
        qint64 nanoseconds = 0;
        {
            JsStats & stats = jsStats();
            QMutexLocker locker(&stats.mutex);
            for (auto iter = stats.data.constBegin(); iter != stats.data.constEnd(); ++iter)
            {
                entries.push_back({iter.key(), iter.value()});
                calls += iter.value().first;
                nanoseconds += iter.value().second;
            }
        }
        std::sort(entries.begin(), entries.end(), [](const auto & lhs, const auto & rhs)
        {
            return lhs.second.second > rhs.second.second;
        });
        mark("js total;" + QString::number(calls) + ";" + QString::number(nanoseconds / 1000000));
        for (qsizetype i = 0; i < static_cast<qsizetype>(entries.size()) && i < 30; ++i)
        {
            mark("js;" + entries[i].first + ";" + QString::number(entries[i].second.first) + ";" + QString::number(entries[i].second.second / 1000000));
        }
    }
    inline void mark(const QString & name)
    {
        static const QByteArray path = qgetenv("COW_PERF_TRACE");
        if (path.isEmpty())
        {
            return;
        }
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        QFile file(QString::fromLocal8Bit(path));
        if (file.open(QIODevice::Append))
        {
            file.write(QByteArray::number(QDateTime::currentMSecsSinceEpoch()) + ";" +
                       QByteArray::number(QCoreApplication::applicationPid()) + ";" + name.toUtf8() + "\n");
        }
    }
    inline bool markTrue(const char* name)
    {
        if (enabled())
        {
            mark(QString::fromLatin1(name));
        }
        return true;
    }
}

#endif // PERFTRACE_H
