#include <QDBusAbstractAdaptor>
#include <QDBusContext>
#include <QEventLoop>
#include <QProcess>

#include <memory>

class Helper;
class QDBusServiceWatcher;

class HelperAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "dev.jonmagon.kdiskmark.helper")

public:
    explicit HelperAdaptor(Helper *parent);

public slots:
    Q_SCRIPTABLE QVariantMap prepareBenchmarkFile(const QString &benchmarkFile, int fileSize, bool fillZeros);
    Q_SCRIPTABLE QVariantMap startBenchmarkTest(int measuringTime, int fileSize, int randomReadPercentage, bool fillZeros, bool cacheBypass,
                                                int blockSize, int queueDepth, int threads, const QString &rw);
    Q_SCRIPTABLE QVariantMap flushPageCache();
    Q_SCRIPTABLE QVariantMap removeBenchmarkFile();
    Q_SCRIPTABLE QVariantMap stopCurrentTask();

signals:
    Q_SCRIPTABLE void taskFinished(bool, QString, QString);

private:
    Helper *m_parentHelper;
};

class Helper : public QObject, public QDBusContext
{
    Q_OBJECT

public:
    Helper();

public:
    QVariantMap prepareBenchmarkFile(const QString &benchmarkFile, int fileSize, bool fillZeros);
    QVariantMap startBenchmarkTest(int measuringTime, int fileSize, int randomReadPercentage, bool fillZeros, bool cacheBypass,
                                   int blockSize, int queueDepth, int threads, const QString &rw);
    QVariantMap flushPageCache();
    QVariantMap removeBenchmarkFile();
    QVariantMap stopCurrentTask();

private:
    bool isCallerAuthorized();
    bool testFilePath(const QString &benchmarkFile);

signals:
    void taskFinished(bool, QString, QString);

private:
    HelperAdaptor *m_helperAdaptor;
    QDBusServiceWatcher *m_serviceWatcher = nullptr;

    QProcess *m_process;
    QString m_benchmarkFile = QStringLiteral();
};
