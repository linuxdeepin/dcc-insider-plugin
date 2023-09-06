#include "insidermodule.h"

#include <widgets/settingsgroup.h>
#include <widgets/itemmodule.h>
#include <widgets/dcclistview.h>

#include <DStandardItem>
#include <QLabel>
#include <QVBoxLayout>

// PackageKit-Qt
#include <Daemon>
#include <QProcess>

using namespace DCC_NAMESPACE;
DWIDGET_USE_NAMESPACE

InsiderModule::InsiderModule(QObject *parent)
    : PageModule("insider", tr("Technology Preview"), parent)
{
    PackageKit::Daemon::setHints(QStringList{"interactive=true"});

    m_availableLaunchers = new QStandardItemModel(this);

    DStandardItem * legacyLauncher = new DStandardItem(tr("Currently stable launcher (dde-launcher)"));
    legacyLauncher->setData("dde-launcher", Dtk::UserRole);
    m_availableLaunchers->appendRow(legacyLauncher); // role: package name

    DStandardItem * newLauncher = new DStandardItem(tr("Technology preview launcher (dde-launchpad)"));
    newLauncher->setData("dde-launchpad", Dtk::UserRole);
    m_availableLaunchers->appendRow(newLauncher); // role: package name

    appendChild(new ItemModule("launchpadTitle", tr("New Launcher")));
    m_launcherList = new ItemModule(
        "selectLauncher", QString(),
        [this](ModuleObject *) -> QWidget * {
            DCCListView * launcherListview = new DCCListView();
            launcherListview->setModel(m_availableLaunchers);

            connect(launcherListview, &DListView::clicked,
                    this, [this](const QModelIndex &index){
                        QStandardItem * checkedItem = m_availableLaunchers->itemFromIndex(index);
                        installPackage(checkedItem->data(Dtk::UserRole).toString());
                    });

            return launcherListview;
        },
        false
    );
    appendChild(m_launcherList);

    PackageKit::Daemon::global()->connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, [this](){
        m_launcherList->setEnabled(!PackageKit::Daemon::isRunning());
    });

//    qDebug() << PackageKit::Daemon::isRunning();
//    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this,
//            [this](){
//                qDebug() << "running" << PackageKit::Daemon::isRunning();
//                m_launcherList->setEnabled(!PackageKit::Daemon::isRunning());
//            });
//    connect(PackageKit::Daemon::global(), &PackageKit::Daemon::transactionListChanged, this,
//            [this](const QStringList &tids){
//                qDebug() << "txList" << tids;
//                m_launcherList->setEnabled(tids.isEmpty());
//            });
}

InsiderModule::~InsiderModule()
{
    //
}

void InsiderModule::active()
{
    checkInstalledLauncher();
}

void InsiderModule::deactive()
{

}

void InsiderModule::installPackage(const QString packageName)
{
    PackageKit::Transaction * tx = PackageKit::Daemon::resolve(packageName);
    connect(tx, &PackageKit::Transaction::package, this,
            [tx, this](PackageKit::Transaction::Info info, const QString &packageID, const QString &summary){
                qDebug() << "Package resolved:" << packageID << info << summary;
                PackageKit::Transaction * nestedTx = PackageKit::Daemon::installPackage(packageID);
                connect(nestedTx, &PackageKit::Transaction::finished, this,
                        [this](PackageKit::Transaction::Exit status, uint runtime){
                            qDebug() << "Package install result:" << status << runtime;
                            if (status == PackageKit::Transaction::ExitSuccess) {
                                cleanupLauncherProcess();
                            }
                            checkInstalledLauncher();
                        });
                connect(nestedTx, &PackageKit::Transaction::errorCode, this,
                        [this](PackageKit::Transaction::Error error, const QString &details){
                            qDebug() << "Package install error:" << error << details;
                            checkInstalledLauncher();
                        });
            });
}

void InsiderModule::cleanupLauncherProcess()
{
    QProcess proc;
    proc.start("killall", {"dde-launcher"});
    proc.waitForFinished(2000);
    proc.start("killall", {"dde-launchpad"});
    proc.waitForFinished(2000);
}

void InsiderModule::checkInstalledLauncher()
{
    m_launcherSearchResults.clear();
    PackageKit::Transaction * tx = PackageKit::Daemon::searchNames("launch", PackageKit::Transaction::FilterInstalled);
    connect(tx, &PackageKit::Transaction::package, this,
            [tx, this](PackageKit::Transaction::Info info, const QString &packageID, const QString &summary){
                qDebug() << "Package search result:" << packageID << info << summary;
                m_launcherSearchResults.append(PackageKit::Daemon::packageName(packageID));
            });
    connect(tx, &PackageKit::Transaction::finished, this,
            [this](PackageKit::Transaction::Exit status, uint runtime){
                qDebug() << "Search finished." << status << runtime;
                int availableLauncherCount = m_availableLaunchers->rowCount();
                for (int i = 0; i < availableLauncherCount; i++) {
                    QStandardItem * item = m_availableLaunchers->item(i);
                    item->setCheckState(m_launcherSearchResults.contains(item->data(Dtk::UserRole).toString()) ? Qt::Checked : Qt::Unchecked);
                }
            });
}
