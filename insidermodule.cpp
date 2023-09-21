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
#include <QProcess>

// PackageKit-Qt
#include <Daemon>

using namespace DCC_NAMESPACE;
DWIDGET_USE_NAMESPACE

InsiderModule::InsiderModule(QObject *parent)
    : PageModule("insider", tr("Technology Preview"), parent)
{
    PackageKit::Daemon::setHints(QStringList{"interactive=true"});

    // Launcher Switcher
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
                        installLauncher(checkedItem->data(Dtk::UserRole).toString());
                    });

            return launcherListview;
        },
        false
    );

    appendChild(m_launcherList);

    PackageKit::Daemon::global()->connect(PackageKit::Daemon::global(), &PackageKit::Daemon::isRunningChanged, this, [this](){
        m_launcherList->setEnabled(!PackageKit::Daemon::isRunning());
    });

    // Display manager
    m_availableDm = new QStandardItemModel(this);

    DStandardItem * lightdm = new DStandardItem(tr("Currently stable Display Manager (lightdm)"));
    lightdm->setData("lightdm", Dtk::UserRole);
    m_availableDm->appendRow(lightdm); // role: package name

    DStandardItem * ddm = new DStandardItem(tr("Technology preview Display Manager/Window Manager (ddm/treeland)"));
    ddm->setData("treeland", Dtk::UserRole);
    m_availableDm->appendRow(ddm);

    appendChild(new ItemModule("dmTitle", tr("New Display Manager")));
    m_dmList = new ItemModule(
        "selectDisplayManager", QString(),
        [this](ModuleObject *) -> QWidget * {
            DCCListView * dmListview = new DCCListView();
            dmListview->setModel(m_availableDm);

            connect(dmListview, &DListView::clicked,
                    this, [this](const QModelIndex &index){
                        QStandardItem * checkedItem = m_availableDm->itemFromIndex(index);
                        QString packageName = checkedItem->data(Dtk::UserRole).toString();;
                        installDisplayManager(packageName);
            });

            return dmListview;
        },
        false
        );

    appendChild(m_dmList);

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
    checkEnabledDisplayManager();
}

void InsiderModule::deactive()
{

}

/// Launcher

void InsiderModule::installLauncher(const QString packageName)
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

/// Display Manager

void InsiderModule::installDisplayManager(const QString packageName)
{
    bool isNew = packageName == "treeland";

    PKUtils::resolve(packageName).then([this, isNew](const PKUtils::PkPackages packages) {
        if (packages.isEmpty()) return;

        PKUtils::installPackage(packages.constFirst()).then([this, isNew](){
            switchDisplayManager(isNew);
            checkEnabledDisplayManager();
        }, [this](const std::exception & e){
            PKUtils::PkError::printException(e);
            checkEnabledDisplayManager();
        });
    });
}

void InsiderModule::checkEnabledDisplayManager()
{
    QProcess process;
    process.setProgram("systemctl");
    process.setArguments(QStringList() << "is-enabled" << "lightdm.service");
    process.start();
    process.waitForFinished();

    bool isLightdm = process.readAllStandardOutput().trimmed() == "enabled";

    int availableDmCount = m_availableDm->rowCount();
    for (int i = 0; i < availableDmCount; i++) {
        QStandardItem * item = m_availableDm->item(i);
        bool isChecked = isLightdm == (item->data(Dtk::UserRole).toString()=="lightdm");
        item->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
    }
}

void InsiderModule::switchDisplayManager(bool isNew)
{
    QProcess process;
    process.setProgram("systemctl");
    if (isNew) {
        // systemd service named ddm, not treeland
        process.setArguments(QStringList() << "enable" << "ddm.service" << "-f");
    } else {
        process.setArguments(QStringList() << "enable" << "lightdm.service" << "-f");
    }
    process.start();
    process.waitForFinished();
    qDebug() << "switchDisplayManager: " << process.readAll();
}

