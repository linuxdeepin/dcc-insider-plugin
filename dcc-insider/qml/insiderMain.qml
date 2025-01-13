// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import org.deepin.dcc 1.0

DccTitleObject {
    name: "dmTitle"
    parentName: "insider"
    weight: 10
    displayName: qsTr("New Display Manager")

    DccObject {
        name: "selectDisplayManager"
        parentName: "insider"
        weight: 20
        pageType: DccObject.Item
        page: DccGroupView {}
        ViewItem {
            name: "lightdm"
            parentName: "selectDisplayManager"
            weight: 10
            displayName: qsTr("Currently stable Display Manager (LightDM)")
        }
        ViewItem {
            name: "treeland"
            parentName: "selectDisplayManager"
            weight: 20
            displayName: qsTr("Technology preview Display Manager/Window Manager (DDM/Treeland)")
            description: qsTr("When experiencing the Treeland environment in a virtual machine, ensure 3D acceleration is enabled. Please note that the current Treeland environment does not support running Wine applications.")
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
        ViewItem {
            name: "fcitx5"
            parentName: "selectInputMethod"
            weight: 10
            displayName: qsTr("Currently stable Input Method")
        }
        ViewItem {
            name: "deepin-im"
            parentName: "selectInputMethod"
            weight: 20
            displayName: qsTr("Technology preview Input Method (deepin-im)")
        }
    }
}
