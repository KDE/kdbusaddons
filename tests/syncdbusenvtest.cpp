/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QCoreApplication>
#include <kupdatelaunchenvironmentjob.h>

// This test syncs the current environment of the spawned process to systemd/whatever
// akin to dbus-update-activation-environment
// it can then be compared with "systemd-run --user -P env" or watched with dbus-monitor

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QEventLoop e;
    auto job = new KUpdateLaunchEnvironmentJob(QProcessEnvironment::systemEnvironment());
    QObject::connect(job, &KUpdateLaunchEnvironmentJob::finished, &e, &QEventLoop::quit);
    return e.exec();
}
