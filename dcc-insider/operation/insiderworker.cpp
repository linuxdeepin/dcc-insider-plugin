// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "insiderworker.h"

#include "pkpackagesproxy.h"

#include <QDir>
#include <QProcess>

namespace dde {
namespace insider {
InsiderWorker::InsiderWorker(QObject *parent)
    : QObject(parent)
    , m_displayManager("lightdm")
    , m_inputMethod("fcitx5")
    , m_pkProxy(new PkPackagesProxy(this))
{
    QMetaObject::invokeMethod(this, &InsiderWorker::checkEnabledDisplayManager, Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, &InsiderWorker::checkEnabledInputMethod, Qt::QueuedConnection);
}

InsiderWorker::~InsiderWorker() { }

QString InsiderWorker::displayManager() const
{
    return m_displayManager;
}

QString InsiderWorker::inputMethod() const
{
    return m_inputMethod;
}

void InsiderWorker::setDisplayManager(const QString &id)
{
    if (m_displayManager == id) {
        return;
    }
    bool isNew = id == "treeland";
    if (isNew && !installPackage("ddm")) {
        return;
    }
    if (!installPackage(id)) {
        return;
    }
    switchDisplayManager(isNew);
    if (isNew) {
        setInputMethod("deepin-im");
        installDDEShell();
    }
    checkEnabledDisplayManager();
}

void InsiderWorker::setInputMethod(const QString &id)
{
    if (m_inputMethod == id) {
        return;
    }
    bool isNew = id == "deepin-im";
    if (!installPackage(id)) {
        return;
    }
    switchInputMethod(isNew);
    checkEnabledInputMethod();
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

static bool imConfigIsDim()
{
    auto home = QDir::home();
    QFile file(home.filePath(".xinputrc"));

    if (file.exists() && file.open(QFile::ReadOnly)) {
        QTextStream in(&file);
        QString line;
        do {
            line = in.readLine();
            if (line.contains("run_im dim")) {
                return true;
            }
        } while (!line.isNull());
    }

    return false;
}

void InsiderWorker::checkEnabledInputMethod()
{
    bool isDim = imConfigIsDim();
    QString inputMethod = isDim ? "deepin-im" : "fcitx5";
    if (m_inputMethod != inputMethod) {
        m_inputMethod = inputMethod;
        Q_EMIT inputMethodChanged(m_inputMethod);
    }
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
    }
}

void InsiderWorker::onInputMethodFinished() { }

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

void InsiderWorker::installDDEShell()
{
    if (installPackage("dde-shell")) {
        checkEnabledDisplayManager();
    }
}

bool InsiderWorker::installPackage(const QString &package)
{
    m_pkProxy->resolve(package);
    if (m_pkProxy->packageID().isEmpty()) {
        return false;
    }
    m_pkProxy->installPackage(m_pkProxy->packageID());
    if (m_pkProxy->error() != 0) {
        return false;
    }
    return true;
}

void InsiderWorker::switchInputMethod(bool isNew)
{
    if (isNew) {
        QProcess process;
        process.setProgram("im-config");
        process.setArguments({ "-n", "dim" });
        process.start();
        process.waitForFinished();
    } else {
        if (imConfigIsDim()) {
            QDir home = QDir::home();
            home.remove(".xinputrc");
        }
    }
}
} // namespace insider
} // namespace dde
