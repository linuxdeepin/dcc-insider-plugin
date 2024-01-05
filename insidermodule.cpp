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
#include <QDir>

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

    // Input Method
    m_availableIM = new QStandardItemModel(this);

    DStandardItem *fcitx5= new DStandardItem(tr("Currently stable Input Method"));
    fcitx5->setData("fcitx5", Dtk::UserRole);
    m_availableIM->appendRow(fcitx5); // role: package name

    DStandardItem *dim = new DStandardItem(tr("Technology preview Input Method (deepin-im)"));
    dim->setData("deepin-im", Dtk::UserRole);
    m_availableIM->appendRow(dim);

    appendChild(new ItemModule("imTitle", tr("New Input Method")));
    m_imList = new ItemModule(
        "selectInputMethod", "",
        [this](ModuleObject *) -> QWidget * {
            auto *imListView = new DCCListView();
            imListView->setModel(m_availableIM);

            connect(imListView, &DListView::clicked, this, [this](const QModelIndex &index) {
                QStandardItem *checkedItem = m_availableIM->itemFromIndex(index);
                if (!checkedItem->isEnabled()) {
                    return;
                }
                QString packageName = checkedItem->data(Dtk::UserRole).toString();;
                installInputMethod(packageName);
            });

            return imListView;
        },
        false
    );
    appendChild(m_imList);

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
    checkEnabledInputMethod();
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

static bool imConfigIsDim() {
    auto home = QDir::home();
    QFile file(home.filePath(".xinputrc"));
   
    if (file.exists() && file.open(QFile::ReadOnly)) {
        QTextStream in (&file);
        QString line;
        do {
            line = in.readLine();
            if (!line.contains("run_im dim")) {
                return true;
            }
        } while (!line.isNull());
    }

    return false;
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

    if (isLightdm) {
        if (imConfigIsDim()) {
            switchInputMethod(false);
            checkEnabledInputMethod();
        }
    }

    int availableIMCount = m_availableIM->rowCount();
    for (int i = 0; i < availableIMCount; i++) {
        QStandardItem *item = m_availableIM->item(i);
        if (item->data(Dtk::UserRole).toString() == "deepin-im") {
            item->setEnabled(!isLightdm);
        }
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

void InsiderModule::installInputMethod(const QString &packageName) {
    bool isNew = packageName == "deepin-im";

    qWarning() << "package:" << packageName;
    PKUtils::resolve(packageName).then([this, isNew] (const PKUtils::PkPackages packages) {
        if (packages.isEmpty()) return;

        PKUtils::installPackage(packages.constFirst()).then([this, isNew]() {
            switchInputMethod(isNew);
            checkEnabledInputMethod();
        }, [this](const std::exception & e) {
            PKUtils::PkError::printException(e);
            checkEnabledInputMethod();
        });
    });
}

void InsiderModule::checkEnabledInputMethod() {
    bool isDim = imConfigIsDim();

    int availableIMCount = m_availableIM->rowCount();
    for (int i = 0; i < availableIMCount; i++) {
        QStandardItem *item = m_availableIM->item(i);
        bool isChecked = isDim == (item->data(Dtk::UserRole).toString() == "deepin-im");
        item->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
    }
}

void InsiderModule::switchInputMethod(bool isNew) {
    if (isNew) {
        QProcess process;
        process.setProgram("im-config");
        process.setArguments({"-n", "dim"});
        process.start();
        process.waitForFinished();
    } else {
        if (imConfigIsDim()) {
            QDir home = QDir::home();
            home.remove(".xinputrc");
        }
    }
}
