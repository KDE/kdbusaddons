/*
    This file is part of the KDE Frameworks.
    SPDX-FileCopyrightText: 2010 Sebastian Trueg <trueg@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KDBUSCONNECTIONPOOL_H
#define KDBUSCONNECTIONPOOL_H

#include <kdbusaddons_export.h>

#if KDBUSADDONS_ENABLE_DEPRECATED_SINCE(5, 68)
#include <QDBusConnection>

/**
 * @namespace KDBusConnectionPool
 * Provides utility functions working around the problem
 * of QDBusConnection not being thread-safe.
 * @deprecated since 5.68 QDBusConnection has been fixed.
 */
namespace KDBusConnectionPool
{
/**
 * The KDBusConnectionPool works around the problem
 * of QDBusConnection not being thread-safe. As soon as that
 * has been fixed (either directly in libdbus or with a work-
 * around in Qt) this method can be dropped in favor of
 * QDBusConnection::sessionBus().
 *
 * Note that this will create a thread-local QDBusConnection
 * object, which means whichever thread this is called
 * from must have both an event loop and be as long-lived as
 * the object using it. If either condition is not met, the
 * returned QDBusConnection will not send or receive D-Bus
 * events (calls, return values, etc).
 *
 * Using this within libraries can create complexities for
 * application developers working with threads as its use
 * in the library may not be apparent to the application
 * developer, and so functionality may appear to be broken
 * simply due to the nature of the thread from which this
 * ends up being called from. Library developers using
 * this facility are strongly encouraged to note this
 * caveat in the library's documentation.
 *
 * @deprecated since 5.68 Use QDBusConnection::sessionBus() instead.
 * QDBusConnection is nowadays safe to use in multiple threads as well.
 */
KDBUSADDONS_DEPRECATED_VERSION(5, 68, "Use QDBusConnection::sessionBus()")
KDBUSADDONS_EXPORT QDBusConnection threadConnection();
}
#endif

#endif

