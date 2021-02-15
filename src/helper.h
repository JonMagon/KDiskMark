#include <KAuth>

using namespace KAuth;

class Helper : public QObject
{
    Q_OBJECT
    public Q_SLOTS:
        ActionReply dropcache(const QVariantMap& args);
};
