// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <dbus/dbus.h>

#include <QDBusMetaType>
#include <QQmlContext>
#include <private/qdbusmessage_p.h>

namespace KDBusAddons
{

void registerTypes();

[[nodiscard]] QVariant pack(const QVariant &arg, const char *signature);
[[nodiscard]] QDBusError pack(QDBusMessage &method, const QVariantList &args, const char *signature);

} // namespace KDBusAddons
