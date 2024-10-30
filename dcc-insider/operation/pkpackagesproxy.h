// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef PKPACKAGESPROXY_H
#define PKPACKAGESPROXY_H

#include <QObject>

namespace dde {
namespace insider {
class PkPackagesProxy : public QObject
{
    Q_OBJECT
public:
    explicit PkPackagesProxy(QObject *parent = nullptr);
    ~PkPackagesProxy() override;

    uint error() const;
    QString packageID() const;

public Q_SLOTS:
    void searchNames(const QString &search);
    void resolve(const QString &search);
    void installPackage(const QString &packageId);

protected Q_SLOTS:
    void init();
    void waitForFinished();

    void onDestroy();
    void Package(uint info, const QString &packageID, const QString &summary);
    void ErrorCode(uint, const QString &);
    void Finished(uint, uint);
Q_SIGNALS:
    void dbusFinished();

protected:
    QString m_path;
    uint m_error;
    QString m_packageID;
};
} // namespace insider
} // namespace dde
#endif // PKPACKAGESPROXY_H
