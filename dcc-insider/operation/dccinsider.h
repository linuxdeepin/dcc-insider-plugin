// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QObject>

#pragma once

namespace dde {
namespace insider {
class InsiderWorker;

class DccInsider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList currentItems READ currentItems NOTIFY currentItemsChanged)

public:
    explicit DccInsider(QObject *parent = nullptr);
    ~DccInsider() override;
    QStringList currentItems() const;

public Q_SLOTS:
    void setCurrentItem(const QString &item);

Q_SIGNALS:
    void currentItemsChanged(const QStringList &currentItems);

protected Q_SLOTS:
    void updateCurrentItem(const QString &item);

private:
    QStringList m_currentItems;
    InsiderWorker *m_insider;
};
} // namespace insider
} // namespace dde
