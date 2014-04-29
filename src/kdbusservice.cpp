/* This file is part of libkdbusaddons

   Copyright (c) 2011 David Faure <faure@kde.org>
   Copyright (c) 2011 Kevin Ottens <ervin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) version 3, or any
   later version accepted by the membership of KDE e.V. (or its
   successor approved by the membership of KDE e.V.), which shall
   act as a proxy defined in Section 6 of version 3 of the license.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "kdbusservice.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include "kdbusservice_adaptor.h"
#include "kdbusserviceextensions_adaptor.h"

class KDBusServicePrivate
{
public:
    KDBusServicePrivate()
        : registered(false),
          exitValue(0)
    {}

    QString generateServiceName()
    {
        const QCoreApplication *app = QCoreApplication::instance();
        const QString domain = app->organizationDomain();
        const QStringList parts = domain.split(QLatin1Char('.'), QString::SkipEmptyParts);

        QString reversedDomain;
        if (parts.isEmpty()) {
            reversedDomain = QLatin1String("local.");
        } else {
            Q_FOREACH (const QString &part, parts) {
                reversedDomain.prepend(QLatin1Char('.'));
                reversedDomain.prepend(part);
            }
        }

        return reversedDomain + app->applicationName();
    }

    bool registered;
    QString serviceName;
    QString errorMessage;
    int exitValue;
};

KDBusService::KDBusService(StartupOptions options, QObject *parent)
    : QObject(parent), d(new KDBusServicePrivate)
{
    new KDBusServiceAdaptor(this);
    new KDBusServiceExtensionsAdaptor(this);
    QDBusConnectionInterface *bus = 0;

    if (!QDBusConnection::sessionBus().isConnected() || !(bus = QDBusConnection::sessionBus().interface())) {
        d->errorMessage = QLatin1String("Session bus not found\n"
                                        "To circumvent this problem try the following command (with Linux and bash)\n"
                                        "export $(dbus-launch)");
    }

    if (bus) {
        d->serviceName = d->generateServiceName();
        QString objectPath = QLatin1Char('/') + d->serviceName;
        objectPath.replace(QLatin1Char('.'), QLatin1Char('/'));

        if (options & Multiple) {
            const QString pid = QString::number(QCoreApplication::applicationPid());
            d->serviceName += QLatin1Char('-') + pid;
        }

        QDBusConnection::sessionBus().registerObject(QLatin1String("/MainApplication"), QCoreApplication::instance(),
                QDBusConnection::ExportAllSlots |
                QDBusConnection::ExportScriptableProperties |
                QDBusConnection::ExportAdaptors);
        QDBusConnection::sessionBus().registerObject(objectPath, this,
                QDBusConnection::ExportAdaptors);

        d->registered = bus->registerService(d->serviceName) == QDBusConnectionInterface::ServiceRegistered;

        if (!d->registered) {
            if (options & Unique) {
                // Already running so it's ok!
                QDBusInterface iface(d->serviceName, objectPath);
                iface.setTimeout(5 * 60 * 1000); // Application can take time to answer
                QVariantMap platform_data;
                platform_data.insert(QStringLiteral("desktop-startup-id"), QString::fromUtf8(qgetenv("DESKTOP_STARTUP_ID")));
                QDBusReply<int> reply;
                if (QCoreApplication::arguments().count() > 1) {
                    reply = iface.call(QLatin1String("CommandLine"), QCoreApplication::arguments(), platform_data);
                } else {
                    reply = iface.call(QLatin1String("Activate"), platform_data);
                }
                if (reply.isValid()) {
                    exit(reply.value());
                } else {
                    d->errorMessage = reply.error().message();
                }
            } else {
                d->errorMessage = QLatin1String("Couldn't register name '")
                                  + d->serviceName
                                  + QLatin1String("' with DBUS - another process owns it already!");
            }

        } else {
            if (QCoreApplication *app = QCoreApplication::instance()) {
                connect(app, SIGNAL(aboutToQuit()), this, SLOT(unregister()));
            }
        }
    }

    if (!d->registered && ((options & NoExitOnFailure) == 0)) {
        qCritical() << d->errorMessage;
        exit(1);
    }
}

KDBusService::~KDBusService()
{
    delete d;
}

bool KDBusService::isRegistered() const
{
    return d->registered;
}

QString KDBusService::errorMessage() const
{
    return d->errorMessage;
}

void KDBusService::setExitValue(int value)
{
    d->exitValue = value;
}

void KDBusService::unregister()
{
    QDBusConnectionInterface *bus = 0;
    if (!d->registered || !QDBusConnection::sessionBus().isConnected() || !(bus = QDBusConnection::sessionBus().interface())) {
        return;
    }
    bus->unregisterService(d->serviceName);
}

void KDBusService::Activate(const QVariantMap &platform_data)
{
    Q_UNUSED(platform_data);
    // TODO QX11Info::setNextStartupId(platform_data.value("desktop-startup-id"))
    emit activateRequested(QStringList());
    // TODO (via hook) KStartupInfo::appStarted(platform_data.value("desktop-startup-id"))
    // ^^ same discussion as below
}

void KDBusService::Open(const QStringList &uris, const QVariantMap &platform_data)
{
    Q_UNUSED(platform_data);
    // TODO QX11Info::setNextStartupId(platform_data.value("desktop-startup-id"))
    emit openRequested(QUrl::fromStringList(uris));
    // TODO (via hook) KStartupInfo::appStarted(platform_data.value("desktop-startup-id"))
    // ^^ not needed if the app actually opened a new window.
    // Solution 1: do it all the time anyway (needs API in QX11Info)
    // Solution 2: pass the id to the app and let it use KStartupInfo::appStarted if reusing a window
}

void KDBusService::ActivateAction(const QString &action_name, const QVariantList &maybeParameter, const QVariantMap &platform_data)
{
    Q_UNUSED(platform_data);
    // This is a workaround for DBus not supporting null variants.
    const QVariant param = maybeParameter.count() == 1 ? maybeParameter.first() : QVariant();
    emit activateActionRequested(action_name, param);
    // TODO (via hook) KStartupInfo::appStarted(platform_data.value("desktop-startup-id"))
    // if desktop-startup-id is set, the action is supposed to show a window (since it could be
    // called when the app is not running)
}

int KDBusService::CommandLine(const QStringList &arguments, const QVariantMap &platform_data)
{
    Q_UNUSED(platform_data);
    d->exitValue = 0;
    // The TODOs here only make sense if this method can be called from the GUI.
    // If it's for pure "usage in the terminal" then no startup notification got started.
    // But maybe one day the workspace wants to call this for the Exec key of a .desktop file?
    // TODO QX11Info::setNextStartupId(platform_data.value("desktop-startup-id"))
    emit activateRequested(arguments);
    // TODO (via hook) KStartupInfo::appStarted(platform_data.value("desktop-startup-id"))
    return d->exitValue;
}
