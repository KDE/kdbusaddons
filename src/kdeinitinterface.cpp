/*
    This file is part of libkdbusaddons

    SPDX-FileCopyrightText: 2013 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kdeinitinterface.h"
#include "kdbusaddons_debug.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QLockFile>
#include <QProcess>
#include <QStandardPaths>

#include <QCoreApplication>
#include <QLibraryInfo>

void KDEInitInterface::ensureKdeinitRunning()
{
    QDBusConnectionInterface *dbusDaemon = QDBusConnection::sessionBus().interface();
    if (dbusDaemon->isServiceRegistered(QStringLiteral("org.kde.klauncher5"))) {
        return;
    }
    qCDebug(KDBUSADDONS_LOG) << "klauncher not running... launching kdeinit";

    QLockFile lock(QDir::tempPath() + QLatin1Char('/') + QLatin1String("startkdeinitlock"));
    // If we can't get the lock, then someone else is already in the process of starting kdeinit.
    if (!lock.tryLock()) {
        // Wait for that to happen, by locking again 30 seconds max.
        if (!lock.tryLock(30000)) {
            qCWarning(KDBUSADDONS_LOG) << "'kdeinit5' is taking more than 30 seconds to start.";
            return;
        }
        // Check that the DBus name is up, i.e. the other process did manage to do it successfully.
        if (dbusDaemon->isServiceRegistered(QStringLiteral("org.kde.klauncher5"))) {
            return;
        }
    }
    // Try to launch kdeinit.
    QString srv = QStandardPaths::findExecutable(QStringLiteral("kdeinit5"));
    // If not found in system paths, search other paths
    if (srv.isEmpty()) {
        const QStringList searchPaths = QStringList() << QCoreApplication::applicationDirPath() // then look where our application binary is located
                                                      << QLibraryInfo::location(QLibraryInfo::BinariesPath); // look where exec path is (can be set in qt.conf)
        srv = QStandardPaths::findExecutable(QStringLiteral("kdeinit5"), searchPaths);
        if (srv.isEmpty()) {
            qCWarning(KDBUSADDONS_LOG) << "Can not find 'kdeinit5' executable at " << qgetenv("PATH") << searchPaths.join(QStringLiteral(", "));
            return;
        }
    }

    QStringList args;
#ifndef Q_OS_WIN
    args += QStringLiteral("--suicide");
#endif
    // NOTE: kdeinit5 is supposed to finish quickly, certainly in less than 30 seconds.
    QProcess::execute(srv, args);
}
