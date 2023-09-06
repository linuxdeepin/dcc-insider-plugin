#pragma once

#include <interface/pagemodule.h>

namespace DCC_NAMESPACE {
class ItemModule;
}

class QStandardItemModel;
class InsiderModule : public DCC_NAMESPACE::PageModule
{
    Q_OBJECT
public:
    explicit InsiderModule(QObject * parent = nullptr);
    ~InsiderModule();

//    virtual QWidget *page() override;

protected:
    virtual void active() override;
    virtual void deactive() override;

private:
    void installPackage(const QString packageName);
    void cleanupLauncherProcess();
    void checkInstalledLauncher();

    QStringList m_launcherSearchResults;
    QStandardItemModel * m_availableLaunchers;
    DCC_NAMESPACE::ItemModule * m_launcherList;
};
