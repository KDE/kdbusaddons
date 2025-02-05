// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusArgument>
#include <QJsonValue>

namespace KDBusAddons
{

// Unpacking DBus arguments is fairly simple.
// We stream data out of the QDBusArgument and recurse until we find a basic type we can easily stream.
[[nodiscard]] QJsonValue unpack(const QDBusArgument &arg);

} // namespace KDBusAddons
