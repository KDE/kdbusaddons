// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusVariant>
#include <QQmlEngine>

/*!
    Helper to create QDBusVariant objects from QML. This is useful when you need to pass a dbus variant of a specific
    type. Notably you can't define int16 and friends in QML, so instead you send your number into this wrapper and it
    will return a QDBusVariant. This QDbusVariant is then consumed as-is by the pack system.

    \note Only useful when you have a literal v in your type signature. For explicit types in the type signature we
          do the right thing automatically.
*/
class DBusVariantWrapper : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    using QObject::QObject;
    Q_INVOKABLE QDBusVariant byte(const QVariant &arg);
    Q_INVOKABLE QDBusVariant boolean(const QVariant &arg);
    Q_INVOKABLE QDBusVariant int16(const QVariant &arg);
    Q_INVOKABLE QDBusVariant uint16(const QVariant &arg);
    Q_INVOKABLE QDBusVariant int32(const QVariant &arg);
    Q_INVOKABLE QDBusVariant uint32(const QVariant &arg);
    Q_INVOKABLE QDBusVariant int64(const QVariant &arg);
    Q_INVOKABLE QDBusVariant uint64(const QVariant &arg);
    Q_INVOKABLE QDBusVariant double_(const QVariant &arg);
    Q_INVOKABLE QDBusVariant unixFd(const QVariant &arg);
    Q_INVOKABLE QDBusVariant string(const QVariant &arg);
    Q_INVOKABLE QDBusVariant objectPath(const QVariant &arg);
    Q_INVOKABLE QDBusVariant signature(const QVariant &arg);
    Q_INVOKABLE QDBusVariant container(const QString &signature, const QVariant &arg);
};
