// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusVariant>
#include <QQmlEngine>

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
