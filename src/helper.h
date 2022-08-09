#include <KAuth>

#include <QEventLoop>

#include <memory>

using namespace KAuth;

class Helper : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "dev.jonmagon.kdiskmark.helper")

Q_SIGNALS:
    void quit();

public Q_SLOTS:
    ActionReply init(const QVariantMap& args);
    Q_SCRIPTABLE QVariantMap listStorages();
    Q_SCRIPTABLE void exit();

    ActionReply dropcache(const QVariantMap& args);

private:
    std::unique_ptr<QEventLoop> m_loop;
};
