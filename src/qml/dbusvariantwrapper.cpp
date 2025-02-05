// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "dbusvariantwrapper.h"

#include <QDBusUnixFileDescriptor>

#include "pack.h"

QDBusVariant DBusVariantWrapper::byte(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_BYTE_AS_STRING));
}

QDBusVariant DBusVariantWrapper::boolean(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_BOOLEAN_AS_STRING));
}

QDBusVariant DBusVariantWrapper::int16(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_INT16_AS_STRING));
}

QDBusVariant DBusVariantWrapper::uint16(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_UINT16_AS_STRING));
}

QDBusVariant DBusVariantWrapper::int32(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_INT32_AS_STRING));
}

QDBusVariant DBusVariantWrapper::uint32(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_UINT32_AS_STRING));
}

QDBusVariant DBusVariantWrapper::int64(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_INT64_AS_STRING));
}

QDBusVariant DBusVariantWrapper::uint64(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_UINT64_AS_STRING));
}

QDBusVariant DBusVariantWrapper::double_(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_DOUBLE_AS_STRING));
}

QDBusVariant DBusVariantWrapper::unixFd(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_UNIX_FD_AS_STRING));
}

QDBusVariant DBusVariantWrapper::string(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_STRING_AS_STRING));
}

QDBusVariant DBusVariantWrapper::objectPath(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_OBJECT_PATH_AS_STRING));
}

QDBusVariant DBusVariantWrapper::signature(const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_SIGNATURE_AS_STRING));
}

QDBusVariant DBusVariantWrapper::container(const QString &signature, const QVariant &arg)
{
    return QDBusVariant(KDBusAddons::pack(arg, qUtf8Printable(signature)));
}
