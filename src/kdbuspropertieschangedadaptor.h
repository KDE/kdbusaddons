// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>

#pragma once

#include <QObject>

#include "kdbusaddons_export.h"

class QDBusConnection;

/**
 * Implements org.freedesktop.DBus.Properties.PropertiesChanged for a QObject.
 * An adaptor object can be used to adapt any QObject with Q_PROPERTY defintions
 * to send PropertiesChanged whenever a signal changes.
 *
 * Adaptees must have Q_CLASSINFO("D-Bus Interface", ...) set!
 * Adaptees must have Q_PROPERTY definitions with NOTIFY signals!
 *
 * @since 6.12
 */
class KDBUSADDONS_EXPORT KDBusPropertiesChangedAdaptor : public QObject
{
    Q_OBJECT
public:
    /**
     * @param objectPath the dbus object path to send the signal from (e.g. /org/kde/someobject)
     * @param adaptee the object to adapt
     * @param bus the bus to send the signal on
     * @param parent the parent object
     */
    KDBusPropertiesChangedAdaptor(const QString &objectPath, QObject *adaptee, const QDBusConnection &bus, QObject *parent = nullptr);
    ~KDBusPropertiesChangedAdaptor() override;
    Q_DISABLE_COPY_MOVE(KDBusPropertiesChangedAdaptor)

    /**
     * Send targeted PropertiesChanged signals to a specified service only.
     * This is useful when the adapator is used in a secure context where properties shouldn't be leaked to the world.
     */
    void setTargetService(const QString &service);

private Q_SLOTS:
    void _k_onPropertyChanged();

private:
    std::unique_ptr<class KDBusPropertiesChangedAdaptorPrivate> d;
};
