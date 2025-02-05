// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "kdbuserror.h"

KDBusError::KDBusError()
{
    dbus_error_init(&m_error);
}

KDBusError::~KDBusError()
{
    dbus_error_free(&m_error);
}

DBusError *KDBusError::get()
{
    return &m_error;
}
