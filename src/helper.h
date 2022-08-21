#include <KAuth>
#include <QDBusAbstractAdaptor>
#include <QDBusContext>
#include <QEventLoop>
#include <QProcess>

#include <memory>

using namespace KAuth;

class Helper;

class HelperAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "dev.jonmagon.kdiskmark.helper")

public:
    explicit HelperAdaptor(Helper *parent);

public slots:
    Q_SCRIPTABLE QVariantMap listStorages();
    Q_SCRIPTABLE void prepareFile(const QString &benchmarkFile, int fileSize, bool fillZeros, const QString &rw);
    Q_SCRIPTABLE void startTest(const QString &benchmarkFile, int measuringTime, int fileSize, int randomReadPercentage, bool fillZeros,
                                int blockSize, int queueDepth, int threads, const QString &rw);
    Q_SCRIPTABLE QVariantMap flushPageCache();
    Q_SCRIPTABLE bool removeFile(const QString &benchmarkFile);
    Q_SCRIPTABLE void stopCurrentTask();
    Q_SCRIPTABLE void exit();

signals:
    Q_SCRIPTABLE void taskFinished(bool, QString, QString);

private:
    Helper *m_parentHelper;
};

class Helper : public QObject, public QDBusContext
{
    Q_OBJECT

public:
    Helper()
    {
        m_helperAdaptor = new HelperAdaptor(this);
        QObject::connect(this, &Helper::taskFinished, m_helperAdaptor, &HelperAdaptor::taskFinished);
    }

public:
    QVariantMap listStorages();
    void prepareFile(const QString &benchmarkFile, int fileSize, bool fillZeros, const QString &rw);
    void startTest(const QString &benchmarkFile, int measuringTime, int fileSize, int randomReadPercentage, bool fillZeros,
                   int blockSize, int queueDepth, int threads, const QString &rw);
    QVariantMap flushPageCache();
    bool removeFile(const QString &benchmarkFile);
    void stopCurrentTask();
    void exit();

private:
    void testFilePath(const QString &benchmarkFile);

public slots:
    ActionReply init(const QVariantMap& args);

signals:
    void taskFinished(bool, QString, QString);

private:
    HelperAdaptor *m_helperAdaptor;
    std::unique_ptr<QEventLoop> m_loop;
    QProcess *m_process;
};
