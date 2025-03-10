/*
    SPDX-FileCopyrightText: 2020 Kai Uwe Broulik <kde@broulik.de>
    SPDX-FileCopyrightText: 2021 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KUPDATELAUNCHENVIRONMENTJOB_H
#define KUPDATELAUNCHENVIRONMENTJOB_H

#include <kdbusaddons_export.h>

#include <QProcessEnvironment>

#include <memory>

class QString;
class KUpdateLaunchEnvironmentJobPrivate;

/*!
 * \class KUpdateLaunchEnvironmentJob
 * \inmodule KDBusAddons
 * \brief Job for updating the launch environment.
 *
 * This job adds or updates an environment variable in process environment that will be used
 * when a process is launched. This includes:
 *
 * \list
 * \li DBus activation
 * \li Systemd units
 * \li Plasma-session
 * \endlist
 *
 * Environment variables are sanitized before uploading.
 *
 * This object deletes itself after completion, similar to KJobs.
 *
 * Example usage:
 *
 * \code
 * QProcessEnvironment newEnv;
 * newEnv.insert("VARIABLE"_s, "value"_s);
 * auto job = new KUpdateLaunchEnvironmentJob(newEnv);
 * QObject::connect(job, &KUpdateLaunchEnvironmentJob::finished, &SomeClass, &SomeClass::someSlot);
 * \endcode
 *
 * Porting from KF5 to KF6:
 *
 * The class UpdateLaunchEnvironmentJob was renamed to KUpdateLaunchEnvironmentJob.
 *
 * \since 6.0
 */
class KDBUSADDONS_EXPORT KUpdateLaunchEnvironmentJob : public QObject
{
    Q_OBJECT

public:
    /*!
     * Creates a new job for the given launch \a environment.
     */
    explicit KUpdateLaunchEnvironmentJob(const QProcessEnvironment &environment);
    ~KUpdateLaunchEnvironmentJob() override;

Q_SIGNALS:
    /*!
     * Emitted when the job is finished, before the object is automatically deleted.
     */
    void finished();

private:
    KDBUSADDONS_NO_EXPORT void start();

private:
    std::unique_ptr<KUpdateLaunchEnvironmentJobPrivate> const d;
};

#endif
