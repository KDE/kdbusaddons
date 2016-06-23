/* This file is part of libkdbusaddons

   Copyright (c) 2013 David Faure <faure@kde.org>

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

#include "kdeinitinterface.h"
#include "kdbusconnectionpool.h"

#include <QDBusConnectionInterface>
#include <QDir>
#include <QLockFile>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>

#include <QCoreApplication>
#include <QLibraryInfo>

void KDEInitInterface::ensureKdeinitRunning()
{
    QDBusConnectionInterface *dbusDaemon = KDBusConnectionPool::threadConnection().interface();
    if (dbusDaemon->isServiceRegistered(QStringLiteral("org.kde.klauncher5"))) {
        return;
    }
    qDebug() << "klauncher not running... launching kdeinit";

    QLockFile lock(QDir::tempPath() + QLatin1Char('/') + QLatin1String("startkdeinitlock"));
    if (!lock.tryLock()) {
        lock.lock();
        if (dbusDaemon->isServiceRegistered(QStringLiteral("org.kde.klauncher5"))) {
            return;    // whoever held the lock has already started it
        }
    }
    // Try to launch kdeinit.
    QString srv = QStandardPaths::findExecutable(QStringLiteral("kdeinit5"));
    // If not found in system paths, search other paths
    if (srv.isEmpty()) {
        const QStringList searchPaths = QStringList()
            << QCoreApplication::applicationDirPath() // then look where our application binary is located
            << QLibraryInfo::location(QLibraryInfo::BinariesPath); // look where exec path is (can be set in qt.conf)
        srv = QStandardPaths::findExecutable(QStringLiteral("kdeinit5"), searchPaths);
        if (srv.isEmpty()) {
            qWarning() << "Can not find 'kdeinit5' executable at " << qgetenv("PATH") << searchPaths.join(QStringLiteral(", "));
            return;
        }
    }

    QStringList args;
#ifndef Q_OS_WIN
    args += QStringLiteral("--suicide");
#endif
    QProcess::execute(srv, args);
}

