// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "insiderworker.h"

#include <QDir>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>


namespace dde {
namespace insider {
InsiderWorker::InsiderWorker(QObject *parent)
    : QObject(parent)
    , m_displayManager("lightdm")
    , m_notifyDisplayManagerChangedFlag(false)
{
    QMetaObject::invokeMethod(this, &InsiderWorker::checkEnabledDisplayManager, Qt::QueuedConnection);
}

InsiderWorker::~InsiderWorker() { }

QString InsiderWorker::displayManager() const
{
    return m_displayManager;
}

void InsiderWorker::setDisplayManager(const QString &id)
{
    if (m_displayManager == id) {
        return;
    }
    m_notifyDisplayManagerChangedFlag = true;
    bool isNew = id == "treeland";
    switchDisplayManager(isNew);
    checkEnabledDisplayManager();
}

void InsiderWorker::checkEnabledDisplayManager()
{
    QProcess *process = new QProcess(this);
    process->setProgram("systemctl");
    process->setArguments(QStringList() << "is-enabled"
                                        << "lightdm.service");
    connect(process, &QProcess::finished, this, &InsiderWorker::onDisplayManagerFinished, Qt::QueuedConnection);
    process->start();
}

void InsiderWorker::onDisplayManagerFinished()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process) {
        return;
    }
    bool isLightdm = process->readAllStandardOutput().trimmed() == "enabled";
    QString displayManager = isLightdm ? "lightdm" : "treeland";
    if (m_displayManager != displayManager) {
        m_displayManager = displayManager;
        Q_EMIT displayManagerChanged(m_displayManager);
        if (m_notifyDisplayManagerChangedFlag) {
            notifyDisplayManagerChanged();
            m_notifyDisplayManagerChangedFlag = false;
        }
    }
}

void InsiderWorker::switchDisplayManager(bool isNew)
{
    QProcess process;
    process.setProgram("/usr/bin/pkexec");
    if (isNew) {
        // systemd service named ddm, not treeland
        process.setArguments(QStringList() << "systemctl"
                                           << "enable"
                                           << "ddm.service"
                                           << "-f");
    } else {
        process.setArguments(QStringList() << "systemctl"
                                           << "enable"
                                           << "lightdm.service"
                                           << "-f");
    }

    process.start();
    process.waitForFinished();
    qDebug() << "switchDisplayManager: " << process.readAll();
}

void InsiderWorker::notifyDisplayManagerChanged()
{
    QStringList notificationActions;
    notificationActions.append("reboot");
    notificationActions.append(tr("Restart Now"));
    notificationActions.append("cancel");
    notificationActions.append(tr("Restart Later"));
    QVariantMap map;
    map.insert("deepin-dde-shell-action-reboot", "{\"pluginId\":\"org.deepin.ds.dde-shutdown\",\"method\":\"requestShutdown\",\"arguments\":[\"Restart\" ]}");
    QDBusMessage message = QDBusMessage::createMethodCall("org.deepin.dde.Notification1", "/org/deepin/dde/Notification1", "org.deepin.dde.Notification1", "Notify");
    message << QString("dde-control-center") << static_cast<uint>(0) 
            << QString("preferences-system") << QString(tr("Display manager switch successful")) 
            << QString(tr("Display manager switch successful, changes will take effect after a restart.")) << notificationActions 
            << map << 0;
    QDBusConnection::sessionBus().asyncCall(message);
}

} // namespace insider
} // namespace dde
