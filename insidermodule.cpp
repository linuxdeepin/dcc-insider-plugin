// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "insidermodule.h"
#include "pkutils.h"

#include <algorithm>
#include <exception>
#include <tuple>

#include <widgets/settingsgroup.h>
#include <widgets/itemmodule.h>
#include <widgets/dcclistview.h>

#include <DStandardItem>
#include <QLabel>
#include <QProcess>
#include <QVBoxLayout>

// PackageKit-Qt
#include <Daemon>

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
    PKUtils::resolve(packageName).then([this](const PKUtils::PkPackages packages) {
        if (packages.isEmpty()) return;

        PKUtils::installPackage(packages.constFirst()).then([this](){
            cleanupLauncherProcess();
            checkInstalledLauncher();
        }, [this](const std::exception & e){
            PKUtils::PkError::printException(e);
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
    PKUtils::searchNames("launch", PackageKit::Transaction::FilterInstalled).then([this](const PKUtils::PkPackages packages) {
        QStringList packageNames;
        for (const PKUtils::PkPackage & pkg : packages) {
            QString pkgId;
            std::tie(std::ignore, pkgId, std::ignore) = pkg;
            packageNames.append(PackageKit::Daemon::packageName(pkgId));
        }

        int availableLauncherCount = m_availableLaunchers->rowCount();
        for (int i = 0; i < availableLauncherCount; i++) {
            QStandardItem * item = m_availableLaunchers->item(i);
            item->setCheckState(packageNames.contains(item->data(Dtk::UserRole).toString()) ? Qt::Checked : Qt::Unchecked);
        }
    });
}
