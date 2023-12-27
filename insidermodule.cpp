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
    checkEnabledDisplayManager();
}

void InsiderModule::deactive()
{

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

