/*
    SPDX-FileCopyrightText: 2020 Kai Uwe Broulik <kde@broulik.de>
    SPDX-FileCopyrightText: 2021 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KDBUSADDONS_UPDATELAUNCHENVIRONMENTJOB_H
#define KDBUSADDONS_UPDATELAUNCHENVIRONMENTJOB_H

#include <kdbusaddons_export.h>

#include <QProcessEnvironment>

#include <memory>

class QString;
class UpdateLaunchEnvironmentJobPrivate;

/**
 * @class UpdateLaunchEnvironmentJob updatelaunchenvironmentjob.h <UpdateLaunchEnvironmentJob>
 *
 * Job for updating the launch environment.
 *
 * This job adds or updates an environment variable in process environment that will be used
 * when a process is launched:
 * This includes:
 *  - DBus activation
 *  - Systemd units
 *  - Plasma-session
 *  - KInit (deprecated)
 *
 * Environment variables are sanitized before uploading.
 *
 * This object deletes itself after completion, similar to KJobs
 *
 * @since 5.84
 */
class KDBUSADDONS_EXPORT UpdateLaunchEnvironmentJob : public QObject
{
    Q_OBJECT

public:
    explicit UpdateLaunchEnvironmentJob(const QProcessEnvironment &environment);
    ~UpdateLaunchEnvironmentJob() override;

Q_SIGNALS:
    void finished();

private:
    KDBUSADDONS_NO_EXPORT void start();

private:
    std::unique_ptr<UpdateLaunchEnvironmentJobPrivate> const d;
};

#endif
