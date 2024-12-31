// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import org.deepin.dcc 1.0

DccObject {
    backgroundType: DccObject.ClickStyle
    pageType: DccObject.Editor
    onActive: dccData.setCurrentItem(name)
    page: DccCheckIcon {
        visible: dccData.currentItems.includes(dccObj.name)
    }
}
