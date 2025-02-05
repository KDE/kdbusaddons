// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <dbus/dbus.h>

#include <QDBusError>

/*!
    Simple wrapper around DBusError to RAII it.
    To turn this into a QDBusError you'll want to use its DBusError constructor.
*/
class KDBusError
{
public:
    KDBusError();
    ~KDBusError();

    Q_DISABLE_COPY_MOVE(KDBusError)

    DBusError *get();

private:
    DBusError m_error{};
};
