// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef INSIDERWORKER_H
#define INSIDERWORKER_H

#include <QObject>

namespace dde {
namespace insider {
class PkPackagesProxy;

class InsiderWorker : public QObject
{
    Q_OBJECT
public:
    explicit InsiderWorker(QObject *parent = nullptr);
    ~InsiderWorker() override;

    QString displayManager() const;
    QString inputMethod() const;

public Q_SLOTS:
    void setDisplayManager(const QString &id);
    void setInputMethod(const QString &id);

Q_SIGNALS:
    void displayManagerChanged(const QString &displayManager);
    void inputMethodChanged(const QString &inputMethod);

protected Q_SLOTS:
    void checkEnabledDisplayManager();
    void checkEnabledInputMethod();

    void onDisplayManagerFinished();
    void onInputMethodFinished();

    void switchDisplayManager(bool isNew);
    void switchInputMethod(bool isNew);
    void installDDEShell();
    bool installPackage(const QString &package);

private:
    QString m_displayManager;
    QString m_inputMethod;
    PkPackagesProxy *m_pkProxy;
};
} // namespace insider
} // namespace dde
#endif // INSIDERWORKER_H
