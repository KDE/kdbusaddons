// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

import QtQuick
import org.kde.dbusaddons

Item {
    Systemd {
        id: systemd
        iface: DBusInterface {
            name: "org.freedesktop.systemd1"
            path: "/org/freedesktop/systemd1"
            iface: "org.freedesktop.systemd1.Manager"
            proxy: systemd
            bus: "session"
        }

        Component.onCompleted: {
            // get a property
            console.log(systemd.dbusUnitPath)

            // sync call
            let [result, data] = systemd.dbusReenableUnitFilesSync(["wireplumber.service"], true, false)
            console.log(result)
            console.log(data)

            // async call via Promise
            systemd.dbusReenableUnitFilesAsync(["wireplumber.service"], true, false).then(([result, args]) => {
                console.log("Unit reenabled", result, args)
            }).catch((error) => {
                console.error("Failed to reenable unit", error)
            })

            systemd.dbusSetUnitPropertiesAsync("wireplumber.service", true, [
                ["var", "value"],
                ["var2", "value2"],
            ]).then(([result, args]) => {
                console.log("Unit properties set", result, args)
            }).catch((error) => {
                console.error("Failed to set unit properties", error)
            })

            // complex signature
            systemd.dbusStartAuxiliaryScopeAsync("foo.service", [[1]], 42, [["foo", 42]])
        }
    }
}
