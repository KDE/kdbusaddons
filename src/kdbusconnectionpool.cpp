/*
    This file is part of the KDE Frameworks.
    SPDX-FileCopyrightText: 2010 Sebastian Trueg <trueg@kde.org>
    SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kdbusconnectionpool.h"
#include <QCoreApplication>
#include <QThread>
#include <QThreadStorage>

#if KDBUSADDONS_BUILD_DEPRECATED_SINCE(5, 68)
namespace
{
QAtomicInt s_connectionCounter;

class KDBusConnectionPoolPrivate
{
public:
    KDBusConnectionPoolPrivate()
        : m_connection(QDBusConnection::connectToBus(QDBusConnection::SessionBus, //
                                                     QStringLiteral("KDBusConnectionPoolConnection%1").arg(newNumber())))
    {
    }

    ~KDBusConnectionPoolPrivate()
    {
        QDBusConnection::disconnectFromBus(m_connection.name());
    }

    QDBusConnection connection() const
    {
        return m_connection;
    }

private:
    static int newNumber()
    {
        return s_connectionCounter.fetchAndAddRelaxed(1);
    }

    QDBusConnection m_connection;
};
} // namespace

static QThreadStorage<KDBusConnectionPoolPrivate *> s_perThreadConnection;

QDBusConnection KDBusConnectionPool::threadConnection()
{
    Q_ASSERT(QCoreApplication::instance() != nullptr);
    if (QCoreApplication::instance()->thread() == QThread::currentThread()) {
        return QDBusConnection::sessionBus();
    }
    if (!s_perThreadConnection.hasLocalData()) {
        s_perThreadConnection.setLocalData(new KDBusConnectionPoolPrivate);
    }

    return s_perThreadConnection.localData()->connection();
}
#endif
