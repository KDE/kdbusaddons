// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "unpack.h"

#include <QJsonArray>
#include <QJsonObject>

using namespace Qt::StringLiterals;

// Unpacking DBus arguments is fairly simple.
// We stream data out of the QDBusArgument and recurse until we find a basic type we can easily stream.
[[nodiscard]] QJsonValue KDBusAddons::unpack(const QDBusArgument &arg)
{
    const auto signature = arg.currentSignature();
    switch (arg.currentType()) {
    case QDBusArgument::BasicType:
        // https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types
        if (signature == 's'_L1 /* STRING */ || //
            signature == 'y'_L1 /* BYTE */ || //
            signature == 'o'_L1 /* OBJECT_PATH */) {
            QString s;
            arg >> s;
            return s;
        }

        if (signature == 'b'_L1 /* BOOLEAN */) {
            bool b = false;
            arg >> b;
            return b;
        }

        if (signature == 'n'_L1 /* INT16 */ || //
            signature == 'q'_L1 /* UINT16 */ || //
            signature == 'i'_L1 /* INT32 */ || //
            signature == 'u'_L1 /* UINT32 */ || //
            signature == 'x'_L1 /* INT64 */ || //
            signature == 't'_L1 /* UINT64 */ || //
            signature == 'd'_L1 /* DOUBLE */ || //
            signature == 'h'_L1 /* UNIX_FD */) {
            // Numbers in JSON are always double anyway. Unpack as such.
            double i = 0;
            arg >> i;
            return i;
        }

        // SIGNATURE is not supported because I have not been able to find a single example of it

        qFatal("Unsupported basic type");
        break;
    case QDBusArgument::VariantType: {
        QVariant v;
        arg >> v;
        return QJsonValue::fromVariant(v);
    }
    case QDBusArgument::ArrayType:
    case QDBusArgument::StructureType: {
        auto structure = QJsonArray();
        arg.beginStructure();
        while (!arg.atEnd()) {
            structure.append(unpack(arg));
        }
        arg.endStructure();
        return structure;
    }
    case QDBusArgument::MapType: {
        auto obj = QJsonObject();
        arg.beginMap();
        while (!arg.atEnd()) {
            arg.beginMapEntry();
            auto key = unpack(arg);
            auto value = unpack(arg);
            obj[key.toString()] = value;
            arg.endMapEntry();
        }
        arg.endMap();
        return obj;
    }
    case QDBusArgument::MapEntryType:
        // Shouldn't get here. MapType unpacks entries!
    case QDBusArgument::UnknownType:
        break;
    }
    qFatal("Unsupported type");
    return {};
}
