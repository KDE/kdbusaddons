/*
    This file is part of libkdbusaddons

    SPDX-FileCopyrightText: 2013 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KDEINIT_IFACE_H
#define KDEINIT_IFACE_H


#include <kdbusaddons_export.h>

/**
 * The KDEInitInterface namespace contains:
 *
 * <ul>
 *   <li>A function to ensure kdeinit and klauncher are running.
 *      This is necessary before making D-Bus calls to org.kde.klauncher5.
 * </ul>
 *
 * @since 5.0
 */
namespace KDEInitInterface
{
KDBUSADDONS_EXPORT void ensureKdeinitRunning();
}

#endif
