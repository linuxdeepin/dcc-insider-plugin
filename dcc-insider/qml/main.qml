// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D

import org.deepin.dcc 1.0

DccTitleObject {
    name: "dmTitle"
    parentName: "insider"
    weight: 10
    displayName: qsTr("New Display Manager")

    Component {
        id: itemPage
        ItemDelegate {
            Layout.fillWidth: true
            text: dccObj.displayName
            hoverEnabled: true
            checkable: false
            checked: true
            cascadeSelected: true
            backgroundVisible: false
            content: DccCheckIcon {
                visible: dccData.currentItems.includes(dccObj.name)
            }
            background: DccItemBackground {
                separatorVisible: false
            }
            onClicked: {
                dccData.setCurrentItem(dccObj.name)
                console.log("onClicked", dccObj.name, dccData.currentItems, dccData.currentItems.includes(dccObj.name))
            }
        }
    }
    DccObject {
        name: "selectDisplayManager"
        parentName: "insider"
        weight: 20
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "lightdm"
            parentName: "selectDisplayManager"
            weight: 10
            displayName: qsTr("Currently stable Display Manager (lightdm)")
            pageType: DccObject.Item
            page: itemPage
        }
        DccObject {
            name: "treeland"
            parentName: "selectDisplayManager"
            weight: 20
            displayName: qsTr("Technology preview Display Manager/Window Manager (ddm/treeland)")
            pageType: DccObject.Item
            page: itemPage
        }
    }
    DccTitleObject {
        name: "imTitle"
        parentName: "insider"
        weight: 30
        visible: false
        displayName: qsTr("New Input Method")
    }
    DccObject {
        name: "selectInputMethod"
        parentName: "insider"
        weight: 40
        visible: false
        pageType: DccObject.Item
        page: DccGroupView {}
        DccObject {
            name: "fcitx5"
            parentName: "selectInputMethod"
            weight: 10
            displayName: qsTr("Currently stable Input Method")
            pageType: DccObject.Item
            page: itemPage
        }
        DccObject {
            name: "deepin-im"
            parentName: "selectInputMethod"
            weight: 20
            displayName: qsTr("Technology preview Input Method (deepin-im)")
            pageType: DccObject.Item
            page: itemPage
        }
    }
}
