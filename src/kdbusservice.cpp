/* This file is part of libkdbusaddons

   Copyright (c) 2011 David Faure <faure@kde.org>
   Copyright (c) 2011 Kevin Ottens <ervin@kde.org>
   Copyright (c) 2019 Harald Sitter <sitter@kde.org>

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

#include <QCoreApplication>
#include <QDebug>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>

#include "KDBusServiceIface.h"
#include "FreeDesktopApplpicationIface.h"

#include "config-kdbusaddons.h"

#if HAVE_X11
#include <QX11Info>
#endif

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
            reversedDomain = QStringLiteral("local.");
        } else {
            for (const QString &part : parts) {
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

// Wraps a serviceName registration.
class Registration : public QObject {
    Q_OBJECT
public:
    enum class Register {
        RegisterWitoutQueue,
        RegisterWithQueue
    };

    Registration(KDBusService *s_, KDBusServicePrivate *d_, KDBusService::StartupOptions options_)
        : s(s_)
        , d(d_)
        , options(options_)
    {
        if (!QDBusConnection::sessionBus().isConnected() || !(bus = QDBusConnection::sessionBus().interface())) {
            d->errorMessage = QLatin1String("Session bus not found\n"
                                            "To circumvent this problem try the following command (with Linux and bash)\n"
                                            "export $(dbus-launch)");
        } else {
            generateServiceName();
        }
    }

    void run() {
        if (bus) {
            registerOnBus();
        }

        if (!d->registered && ((options & KDBusService::NoExitOnFailure) == 0)) {
            qCritical() << d->errorMessage;
            exit(1);
        }
    }

    private:

    void generateServiceName()
    {
        d->serviceName = d->generateServiceName();
        objectPath = QLatin1Char('/') + d->serviceName;
        objectPath.replace(QLatin1Char('.'), QLatin1Char('/'));
        objectPath.replace(QLatin1Char('-'), QLatin1Char('_')); // see spec change at https://bugs.freedesktop.org/show_bug.cgi?id=95129

        if (options & KDBusService::Multiple) {
            bool inSandbox = false;
            if (!qEnvironmentVariableIsEmpty("XDG_RUNTIME_DIR")) {
                inSandbox = QFileInfo::exists(QString::fromUtf8(qgetenv("XDG_RUNTIME_DIR")) + QLatin1String("/flatpak-info"));
            }

            if (inSandbox) {
                d->serviceName += QStringLiteral(".kdbus-") + QDBusConnection::sessionBus().baseService().replace(QRegularExpression(QStringLiteral("[\\.:]")), QStringLiteral("_"));
            } else {
                d->serviceName += QLatin1Char('-') + QString::number(QCoreApplication::applicationPid());
            }
        }
    }

    void registerOnBus()
    {
        auto bus = QDBusConnection::sessionBus();
        bool objectRegistered = false;
        objectRegistered = bus.registerObject(QStringLiteral("/MainApplication"),
                                              QCoreApplication::instance(),
                                              QDBusConnection::ExportAllSlots |
                                              QDBusConnection::ExportScriptableProperties |
                                              QDBusConnection::ExportAdaptors);
        if (!objectRegistered) {
            qWarning() << "Failed to register /MainApplication on DBus";
            return;
        }

        objectRegistered = bus.registerObject(objectPath, s, QDBusConnection::ExportAdaptors);
        if (!objectRegistered) {
            qWarning() << "Failed to register" << objectPath << "on DBus";
            return;
        }

        attemptRegistration();

        if (d->registered) {
            if (QCoreApplication *app = QCoreApplication::instance()) {
                connect(app, &QCoreApplication::aboutToQuit,
                        s, &KDBusService::unregister);
            }
        }
    }

    void attemptRegistration()
    {
        Q_ASSERT(!d->registered);

        auto queueOption = QDBusConnectionInterface::DontQueueService;

        if (options & KDBusService::Unique) {
            // When a process crashes and gets auto-restarted by KCrash we may
            // be in this code path "too early". There is a bit of a delay
            // between the restart and the previous process dropping off of the
            // bus and thus releasing its registered names. As a result there
            // is a good chance that if we wait a bit the name will shortly
            // become registered.

            queueOption = QDBusConnectionInterface::QueueService;

            connect(bus, &QDBusConnectionInterface::serviceRegistered,
                    this, [this](const QString &service) {
                if (service != d->serviceName) {
                    return;
                }

                d->registered = true;
                registrationLoop.quit();
            });
        }

        d->registered =
                (bus->registerService(d->serviceName, queueOption) == QDBusConnectionInterface::ServiceRegistered);

        if (d->registered) {
            return;
        }

        if (options & KDBusService::Replace) {
            auto message = QDBusMessage::createMethodCall(d->serviceName,
                                                    QStringLiteral("/MainApplication"),
                                                    QStringLiteral("org.qtproject.Qt.QCoreApplication"),
                                                    QStringLiteral("quit"));
            QDBusConnection::sessionBus().asyncCall(message);
            waitForRegistration();
        } else if (options & KDBusService::Unique) {
            // Already running so it's ok!
            QVariantMap platform_data;
            platform_data.insert(QStringLiteral("desktop-startup-id"), QString::fromUtf8(qgetenv("DESKTOP_STARTUP_ID")));
            if (QCoreApplication::arguments().count() > 1) {
                OrgKdeKDBusServiceInterface iface(d->serviceName, objectPath, QDBusConnection::sessionBus());
                iface.setTimeout(5 * 60 * 1000); // Application can take time to answer
                QDBusReply<int> reply = iface.CommandLine(QCoreApplication::arguments(), QDir::currentPath(), platform_data);
                if (reply.isValid()) {
                    exit(reply.value());
                } else {
                    d->errorMessage = reply.error().message();
                }
            } else {
                OrgFreedesktopApplicationInterface iface(d->serviceName, objectPath, QDBusConnection::sessionBus());
                iface.setTimeout(5 * 60 * 1000); // Application can take time to answer
                QDBusReply<void> reply = iface.Activate(platform_data);
                if (reply.isValid()) {
                    exit(0);
                } else {
                    d->errorMessage = reply.error().message();
                }
            }

            // service did not respond in a valid way....
            // let's wait to see if our queued registration finishes perhaps.
            waitForRegistration();
        }

        if (!d->registered) { // either multi service or failed to reclaim name
            d->errorMessage = QLatin1String("Couldn't register name '")
                    + d->serviceName
                    + QLatin1String("' with DBUS - another process owns it already!");
        }
    }

    void waitForRegistration()
    {
        QTimer quitTimer;
        // Wait a bit longer when we know this instance was restarted. There's
        // a very good chance we'll eventually get the name once the defunct
        // process closes its sockets.
        quitTimer.start(qEnvironmentVariableIsSet("KCRASH_AUTO_RESTARTED") ? 8000 : 2000);
        connect(&quitTimer, &QTimer::timeout, &registrationLoop, &QEventLoop::quit);
        registrationLoop.exec();
    }

    QDBusConnectionInterface *bus = nullptr;
    KDBusService *s = nullptr;
    KDBusServicePrivate *d = nullptr;
    KDBusService::StartupOptions options;
    QEventLoop registrationLoop;
    QString objectPath;
};

KDBusService::KDBusService(StartupOptions options, QObject *parent)
    : QObject(parent), d(new KDBusServicePrivate)
{
    new KDBusServiceAdaptor(this);
    new KDBusServiceExtensionsAdaptor(this);

    Registration registration(this, d, options);
    registration.run();
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

QString KDBusService::serviceName() const
{
    return d->serviceName;
}

void KDBusService::unregister()
{
    QDBusConnectionInterface *bus = nullptr;
    if (!d->registered || !QDBusConnection::sessionBus().isConnected() || !(bus = QDBusConnection::sessionBus().interface())) {
        return;
    }
    bus->unregisterService(d->serviceName);
}

void KDBusService::Activate(const QVariantMap &platform_data)
{
    Q_UNUSED(platform_data);
#if HAVE_X11
    if (QX11Info::isPlatformX11()) {
        QX11Info::setAppTime(QX11Info::getTimestamp());
    }
#endif
    // TODO QX11Info::setNextStartupId(platform_data.value("desktop-startup-id"))
    emit activateRequested(QStringList(), QString());
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
    // This is a workaround for D-Bus not supporting null variants.
    const QVariant param = maybeParameter.count() == 1 ? maybeParameter.first() : QVariant();
    emit activateActionRequested(action_name, param);
    // TODO (via hook) KStartupInfo::appStarted(platform_data.value("desktop-startup-id"))
    // if desktop-startup-id is set, the action is supposed to show a window (since it could be
    // called when the app is not running)
}

int KDBusService::CommandLine(const QStringList &arguments, const QString &workingDirectory, const QVariantMap &platform_data)
{
    Q_UNUSED(platform_data);
    d->exitValue = 0;
    // The TODOs here only make sense if this method can be called from the GUI.
    // If it's for pure "usage in the terminal" then no startup notification got started.
    // But maybe one day the workspace wants to call this for the Exec key of a .desktop file?
    // TODO QX11Info::setNextStartupId(platform_data.value("desktop-startup-id"))
    emit activateRequested(arguments, workingDirectory);
    // TODO (via hook) KStartupInfo::appStarted(platform_data.value("desktop-startup-id"))
    return d->exitValue;
}

#include "kdbusservice.moc"
