// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "pkpackagesproxy.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QEventLoop>

namespace dde {
namespace insider {
static QString PK_NAME = QStringLiteral("org.freedesktop.PackageKit");
static QString PK_OFFLINE_INTERFACE = QStringLiteral("org.freedesktop.PackageKit.Offline");
static QString PK_PATH = QStringLiteral("/org/freedesktop/PackageKit");
static QString PK_TRANSACTION_INTERFACE = QStringLiteral("org.freedesktop.PackageKit.Transaction");

static QString DBUS_PROPERTIES = QStringLiteral("org.freedesktop.DBus.Properties");

PkPackagesProxy::PkPackagesProxy(QObject *parent)
    : QObject(parent)
    , m_error(0)
{
}

PkPackagesProxy::~PkPackagesProxy() { }

uint PkPackagesProxy::error() const
{
    return m_error;
}

QString PkPackagesProxy::packageID() const
{
    return m_packageID;
}

void PkPackagesProxy::searchNames(const QString &search)
{
    init();
    QDBusMessage msg = QDBusMessage::createMethodCall(PK_NAME, m_path, PK_TRANSACTION_INTERFACE, "SearchNames");
    msg << (quint64)2 << QStringList{ search };
    QDBusPendingReply<void> reply = QDBusConnection::systemBus().call(msg);
    waitForFinished();
}

void PkPackagesProxy::resolve(const QString &search)
{
    init();
    QDBusMessage msg = QDBusMessage::createMethodCall(PK_NAME, m_path, PK_TRANSACTION_INTERFACE, "Resolve");
    msg << (quint64)2 << QStringList{ search };
    QDBusPendingReply<void> reply = QDBusConnection::systemBus().call(msg);
    qDebug() << "resolve Package:" << m_path << reply.error() << search;
    waitForFinished();
}

void PkPackagesProxy::installPackage(const QString &packageId)
{
    init();
    QDBusMessage msg = QDBusMessage::createMethodCall(PK_NAME, m_path, PK_TRANSACTION_INTERFACE, "InstallPackages");
    msg << (quint64)2 << QStringList{ packageId };
    QDBusPendingReply<void> reply = QDBusConnection::systemBus().call(msg);
    qDebug() << "install Package:" << m_path << reply.error() << packageId;
    waitForFinished();
}

void PkPackagesProxy::init()
{
    m_error = 0;
    m_packageID.clear();

    QDBusMessage msg = QDBusMessage::createMethodCall(PK_NAME, PK_PATH, PK_NAME, "CreateTransaction");
    QDBusPendingReply<QDBusObjectPath> reply = QDBusConnection::systemBus().call(msg);
    if (reply.isError()) { }
    m_path = reply.value().path();
    qDebug() << "Create Transaction:" << m_path;
    QDBusConnection::systemBus().connect(PK_NAME, m_path, PK_TRANSACTION_INTERFACE, QLatin1String("Destroy"), this, SLOT(onDestroy()));
    QDBusConnection::systemBus().connect(PK_NAME, m_path, PK_TRANSACTION_INTERFACE, QLatin1String("Package"), this, SLOT(Package(uint, QString, QString)));
    QDBusConnection::systemBus().connect(PK_NAME, m_path, PK_TRANSACTION_INTERFACE, QLatin1String("ErrorCode"), this, SLOT(ErrorCode(uint, QString)));
    QDBusConnection::systemBus().connect(PK_NAME, m_path, PK_TRANSACTION_INTERFACE, QLatin1String("Finished"), this, SLOT(Finished(uint, uint)));
}

void PkPackagesProxy::waitForFinished()
{
    QEventLoop loop;
    connect(this, &PkPackagesProxy::dbusFinished, &loop, &QEventLoop::quit);
    loop.exec();
}

void PkPackagesProxy::onDestroy()
{
    Q_EMIT dbusFinished();
    m_path.clear();
}

void PkPackagesProxy::Package(uint info, const QString &packageID, const QString &summary)
{
    qDebug() << "Package" << m_path << info << packageID << summary;
    if (info == 1 || m_packageID.isEmpty()) {
        m_packageID = packageID;
    }
}

void PkPackagesProxy::ErrorCode(uint error, const QString &details)
{
    qDebug() << "ErrorCode" << m_path << error << details;
    m_error = error;
}

void PkPackagesProxy::Finished(uint status, uint runtime)
{
    qDebug() << "Finished" << m_path << status << runtime;
    Q_EMIT dbusFinished();
    m_path.clear();
}
} // namespace insider
} // namespace dde
