// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "dbusvariantwrapper.h"

#include <QDBusUnixFileDescriptor>

#include "pack.h"

// namespace
// {
// QDBusVariant wrap(const QVariant &jsArg, const QVariant &arg, int type)
// {
//     qDebug() << arg << type;
//     switch (type) {
//     case DBUS_TYPE_BOOLEAN:
//         return QDBusVariant(QVariant::fromValue(arg.value<bool>()));
//     case DBUS_TYPE_INT16:
//         return QDBusVariant(QVariant::fromValue(arg.value<qint16>()));
//     case DBUS_TYPE_INT32:
//         return QDBusVariant(QVariant::fromValue(arg.value<qint32>()));
//     case DBUS_TYPE_INT64:
//         return QDBusVariant(QVariant::fromValue(arg.value<qint64>()));
//     case DBUS_TYPE_UINT16:
//         return QDBusVariant(QVariant::fromValue(arg.value<quint16>()));
//     case DBUS_TYPE_UINT32:
//         return QDBusVariant(QVariant::fromValue(arg.value<quint32>()));
//     case DBUS_TYPE_UINT64:
//         return QDBusVariant(QVariant::fromValue(arg.value<quint64>()));
//     case DBUS_TYPE_BYTE:
//         return QDBusVariant(KDBusAddons::pack(arg, DBUS_TYPE_BYTE_AS_STRING));
//     case DBUS_TYPE_DOUBLE:
//         return QDBusVariant(QVariant::fromValue(arg.value<double>()));
//     case DBUS_TYPE_STRING:
//         return QDBusVariant(QVariant::fromValue(arg.value<QString>()));
//     case DBUS_TYPE_UNIX_FD:
//         return QDBusVariant(QVariant::fromValue(QVariant::fromValue(QDBusUnixFileDescriptor(arg.value<int>()))));
//     case DBUS_TYPE_OBJECT_PATH:
//         return QDBusVariant(QVariant::fromValue(QDBusObjectPath(arg.value<QString>())));
//     case DBUS_TYPE_SIGNATURE:
//         return QDBusVariant(QVariant::fromValue(QVariant::fromValue(QDBusSignature(arg.value<QString>()))));
//     case DBUS_TYPE_VARIANT:
//         return QDBusVariant(arg);
//     case DBUS_TYPE_ARRAY:
//     case DBUS_TYPE_STRUCT:
//         qFatal("Unsupported type");
//     }
//     return {};
// }
// } // namespace

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
