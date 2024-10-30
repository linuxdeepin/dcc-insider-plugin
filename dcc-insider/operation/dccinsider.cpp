// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dccinsider.h"

#include "dde-control-center/dccfactory.h"
#include "insiderworker.h"

namespace dde {
namespace insider {
DccInsider::DccInsider(QObject *parent)
    : QObject(parent)
    , m_insider(new InsiderWorker(this))
{
    m_currentItems.append({ m_insider->displayManager(), m_insider->inputMethod() });
    connect(m_insider, &InsiderWorker::displayManagerChanged, this, &DccInsider::updateCurrentItem);
    connect(m_insider, &InsiderWorker::inputMethodChanged, this, &DccInsider::updateCurrentItem);
}

DccInsider::~DccInsider() { }

QStringList DccInsider::currentItems() const
{
    return m_currentItems;
}

void DccInsider::setCurrentItem(const QString &item)
{
    if (item == "lightdm" || item == "treeland") {
        m_insider->setDisplayManager(item);
    } else if (item == "fcitx5" || item == "deepin-im") {
        m_insider->setInputMethod(item);
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
    } else if (item == "fcitx5") {
        m_currentItems.removeOne("deepin-im");
        m_currentItems.append("fcitx5");
    } else if (item == "deepin-im") {
        m_currentItems.removeOne("fcitx5");
        m_currentItems.append("deepin-im");
    }
    Q_EMIT currentItemsChanged(m_currentItems);
}
DCC_FACTORY_CLASS(DccInsider)
} // namespace insider
} // namespace dde

#include "dccinsider.moc"
