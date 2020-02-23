/*
    This file is part of the KDE

    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2011 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kdbusinterprocesslock.h"

#include <QEventLoop>
#include <QDBusConnectionInterface>


class KDBusInterProcessLockPrivate
{
public:
    KDBusInterProcessLockPrivate(const QString &resource, KDBusInterProcessLock *parent)
        : m_resource(resource), m_parent(parent)
    {
        m_serviceName = QStringLiteral("org.kde.private.lock-%1").arg(m_resource);

        m_parent->connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceRegistered(QString)),
                          m_parent, SLOT(_k_serviceRegistered(QString)));
    }

    ~KDBusInterProcessLockPrivate()
    {
    }

    void _k_serviceRegistered(const QString &service)
    {
        if (service == m_serviceName) {
            emit m_parent->lockGranted(m_parent);
        }
    }

    QString m_resource;
    QString m_serviceName;
    KDBusInterProcessLock *m_parent;
};

KDBusInterProcessLock::KDBusInterProcessLock(const QString &resource)
    : d(new KDBusInterProcessLockPrivate(resource, this))
{
}

KDBusInterProcessLock::~KDBusInterProcessLock()
{
    delete d;
}

QString KDBusInterProcessLock::resource() const
{
    return d->m_resource;
}

void KDBusInterProcessLock::lock()
{
    QDBusConnection::sessionBus().interface()->registerService(d->m_serviceName,
            QDBusConnectionInterface::QueueService,
            QDBusConnectionInterface::DontAllowReplacement);
}

void KDBusInterProcessLock::unlock()
{
    QDBusConnection::sessionBus().interface()->unregisterService(d->m_serviceName);
}

void KDBusInterProcessLock::waitForLockGranted()
{
    QEventLoop loop;
    connect(this, &KDBusInterProcessLock::lockGranted, &loop, &QEventLoop::quit);
    loop.exec();
}

#include "moc_kdbusinterprocesslock.cpp"
