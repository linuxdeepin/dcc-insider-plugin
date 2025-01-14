// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dccinsider.h"

#include "dde-control-center/dccfactory.h"
#include "insiderworker.h"

#include <QFile>

namespace dde {
namespace insider {
DccInsider::DccInsider(QObject *parent)
    : QObject(parent)
    , m_insider(new InsiderWorker(this))
{
    m_currentItems.append({ m_insider->displayManager() });
    connect(m_insider, &InsiderWorker::displayManagerChanged, this, &DccInsider::updateCurrentItem);
}

DccInsider::~DccInsider() { }

QStringList DccInsider::currentItems() const
{
    return m_currentItems;
}

bool DccInsider::isLive() const
{
    QFile file("/proc/cmdline");
    if (file.open(QFile::ReadOnly)) {
        return file.readAll().contains(" boot=live ");
    }
    return false;
}

void DccInsider::setCurrentItem(const QString &item)
{
    if (item == "lightdm" || item == "treeland") {
        m_insider->setDisplayManager(item);
    }
}

void DccInsider::updateCurrentItem(const QString &item)
{
    if (item == "lightdm") {
        m_currentItems.removeOne("treeland");
        m_currentItems.append("lightdm");
    } else if (item == "treeland") {
        m_currentItems.removeOne("lightdm");
        m_currentItems.append("treeland");
    }
    Q_EMIT currentItemsChanged(m_currentItems);
}
DCC_FACTORY_CLASS(DccInsider)
} // namespace insider
} // namespace dde

#include "dccinsider.moc"
