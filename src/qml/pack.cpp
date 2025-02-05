// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "pack.h"

#include <QDBusUnixFileDescriptor>

#include "kdbuserror.h"

using namespace Qt::StringLiterals;

// A glorified typeStream call.
// It allows us to stream complex types (structs, arrays) that QDBusArgument will only stream if it has a QMetaType for.
//
// The way this work is that the caller sets a temporary signature for this type.
// Then they call the operator<< which results in a typeStream call with the given context.
class FlexList
{
public:
    DBusSignatureIter *it;
    QVariantList data;
};
Q_DECLARE_METATYPE(FlexList);

QDBusArgument &operator<<(QDBusArgument &argument, const FlexList &);
const QDBusArgument &operator>>(const QDBusArgument &argument, FlexList &);

class FlexKey
{
public:
    DBusSignatureIter *it;
    QVariant data;
};
Q_DECLARE_METATYPE(FlexKey);

QDBusArgument &operator<<(QDBusArgument &argument, const FlexKey &);
const QDBusArgument &operator>>(const QDBusArgument &argument, FlexKey &);

class FlexValue
{
public:
    DBusSignatureIter *it;
    QVariant data;
};
Q_DECLARE_METATYPE(FlexValue);

QDBusArgument &operator<<(QDBusArgument &argument, const FlexValue &);
const QDBusArgument &operator>>(const QDBusArgument &argument, FlexValue &);

namespace
{
// Fairly crap but the alternative is making a class. Either isn't thread safe so whatever.
inline static QDBusError s_dbusError;

void clearDBusError()
{
    s_dbusError = QDBusError();
}

void setDBusError(const QDBusError &error)
{
    if (s_dbusError.isValid()) {
        return; // already have an error, thanks and good bye!
    }
    s_dbusError = error;
}

[[nodiscard]] QDBusError validSignature(const char *signature)
{
    KDBusError error;
    if (!dbus_signature_validate(signature, error.get())) {
        qWarning() << "Invalid signature" << signature;
        return QDBusError(error.get());
    }
    return {};
}

void flexStreamArrayEnumerable(QDBusArgument &dbus, DBusSignatureIter *it, const QVariantList &list)
{
    // Set the current signature.
    // e.g. if the input is `a(ss)` we currently report `(ss)`
    //      if the input is `aaas` we currently report `aas`
    // Mind that basic types are handled elsewhere. So we'll never get `as` in here.
    QDBusMetaType::registerCustomType(QMetaType::fromType<FlexList>(), dbus_signature_iter_get_signature(it));
    dbus.beginArray(QMetaType::fromType<FlexList>());

    // Next we treat the incoming data as a list (both array and struct are represented in QML as lists).
    // And finally we stream the list via FlexEnumerable.
    for (const auto &entry : list) {
        dbus << FlexList{.it = it, .data = entry.toList()};
    }

    dbus.endArray();
}

void flexStreamArrayValue(QDBusArgument &dbus, DBusSignatureIter *it, const QVariant &arg)
{
    // Works the same as flexStreamArrayEnumerable but for basic types.
    QDBusMetaType::registerCustomType(QMetaType::fromType<FlexValue>(), dbus_signature_iter_get_signature(it));
    dbus.beginArray(QMetaType::fromType<FlexValue>());

    const auto list = arg.toList();
    for (const auto &entry : list) {
        dbus << FlexValue{.it = it, .data = entry};
    }

    dbus.endArray();
}

void structStream(QDBusArgument &dbus, DBusSignatureIter *it, const QVariantList &data);
void mapStream(QDBusArgument &dbus, DBusSignatureIter *it, const QVariantMap &data);

void typeStream(QDBusArgument &dbus, DBusSignatureIter *it, const QVariant &arg)
{
    switch (dbus_signature_iter_get_current_type(it)) {
    case DBUS_TYPE_BOOLEAN:
        dbus << arg.value<bool>();
        return;
    case DBUS_TYPE_INT16:
        dbus << arg.value<qint16>();
        return;
    case DBUS_TYPE_INT32:
        dbus << arg.value<qint32>();
        return;
    case DBUS_TYPE_INT64:
        dbus << arg.value<qint64>();
        return;
    case DBUS_TYPE_UINT16:
        dbus << arg.value<quint16>();
        return;
    case DBUS_TYPE_UINT32:
        dbus << arg.value<quint32>();
        return;
    case DBUS_TYPE_UINT64:
        dbus << arg.value<quint64>();
        return;
    case DBUS_TYPE_BYTE:
        dbus << arg.value<uchar>();
        return;
    case DBUS_TYPE_DOUBLE:
        dbus << arg.value<double>();
        return;
    case DBUS_TYPE_STRING:
        dbus << arg.value<QString>();
        return;
    case DBUS_TYPE_UNIX_FD:
        dbus << QDBusUnixFileDescriptor(arg.value<int>());
        return;
    case DBUS_TYPE_OBJECT_PATH:
        dbus << QDBusObjectPath(arg.value<QString>());
        return;
    case DBUS_TYPE_SIGNATURE:
        dbus << QDBusSignature(arg.value<QString>());
        return;
    case DBUS_TYPE_VARIANT:
        if (arg.metaType()
            == QMetaType::fromType<QDBusVariant>()) { // short circuit for DBusVariantWrapper which feeds us a QDBusVariant already as "opaque" type
            dbus << arg.value<QDBusVariant>();
        } else {
            dbus << QDBusVariant(arg);
        }
        return;
    case DBUS_TYPE_ARRAY: {
        DBusSignatureIter recurseIt;
        dbus_signature_iter_recurse(it, &recurseIt);

        if (dbus_signature_iter_get_element_type(it) == DBUS_TYPE_DICT_ENTRY) {
            mapStream(dbus, &recurseIt, arg.toMap());
            return;
        }

        switch (dbus_signature_iter_get_current_type(&recurseIt)) {
        case DBUS_TYPE_ARRAY:
            if (dbus_signature_iter_get_element_type(&recurseIt) == DBUS_TYPE_DICT_ENTRY) {
                // Maps aren't real arrays. Treat them as values (recurses in on us and hits the above condition).
                flexStreamArrayValue(dbus, &recurseIt, arg);
                return;
            }
            [[fallthrough]];
        case DBUS_TYPE_STRUCT:
            // Structs and array containers are both implemented as lists on the QML side. Streamed as array of arrays.
            flexStreamArrayEnumerable(dbus, &recurseIt, arg.toList());
            return;
        default:
            // Any basic value gets streamed as array of values.
            flexStreamArrayValue(dbus, &recurseIt, arg);
            return;
        }

        return;
    }
    case DBUS_TYPE_STRUCT:
        DBusSignatureIter recurseIt;
        dbus_signature_iter_recurse(it, &recurseIt);
        structStream(dbus, &recurseIt, arg.toList());
        return;
    }
    qFatal("fallthru");
}

void mapStream(QDBusArgument &dbus, DBusSignatureIter *it, const QVariantMap &data)
{
    {
        DBusSignatureIter typeIt;
        dbus_signature_iter_recurse(it, &typeIt);
        QDBusMetaType::registerCustomType(QMetaType::fromType<FlexKey>(), dbus_signature_iter_get_signature(&typeIt));
        dbus_signature_iter_next(&typeIt);
        QDBusMetaType::registerCustomType(QMetaType::fromType<FlexValue>(), dbus_signature_iter_get_signature(&typeIt));
    }

    dbus.beginMap(QMetaType::fromType<FlexKey>(), QMetaType::fromType<FlexValue>());
    for (const auto &[key, value] : data.asKeyValueRange()) {
        dbus.beginMapEntry();
        {
            DBusSignatureIter entryIt;
            dbus_signature_iter_recurse(it, &entryIt);
            dbus << FlexKey{
                .it = &entryIt,
                .data = key,
            };
            dbus_signature_iter_next(&entryIt);
            dbus << FlexValue{
                .it = &entryIt,
                .data = value,
            };
        }
        dbus.endMapEntry();
    }
    dbus.endMap();
}

void structStream(QDBusArgument &dbus, DBusSignatureIter *it, const QVariantList &data)
{
    auto mutableData = data;
    dbus.beginStructure();
    do {
        const auto arg = [&] {
            if (mutableData.isEmpty()) {
                setDBusError(QDBusError(QDBusError::InvalidArgs, u"Not enough arguments to stream a struct of type '%1'!"_s.arg(dbus.currentSignature())));
                return QVariant();
            }
            return mutableData.takeFirst();
        }();
        typeStream(dbus, it, arg);
    } while ((bool)dbus_signature_iter_next(it));
    dbus.endStructure();
}

} // namespace

[[nodiscard]] QVariant KDBusAddons::pack(const QVariant &arg, const char *signature)
{
    if (auto err = validSignature(signature); err.isValid()) {
        return {};
    }

    clearDBusError();

    DBusSignatureIter it;
    dbus_signature_iter_init(&it, signature);

    QDBusArgument dbusArg;
    typeStream(dbusArg, &it, arg);

    return QVariant::fromValue(dbusArg);
}

[[nodiscard]] QDBusError KDBusAddons::pack(QDBusMessage &method, const QVariantList &args, const char *signature)
{
    if (auto err = validSignature(signature); err.isValid()) {
        return err;
    }

    clearDBusError();

    DBusSignatureIter it;
    dbus_signature_iter_init(&it, signature);

    for (auto &arg : args) {
        QDBusArgument dbusArg;
        typeStream(dbusArg, &it, arg);
        method << QVariant::fromValue(dbusArg);
        if (!dbus_signature_iter_next(&it)) {
            break;
        }
    }

    return s_dbusError;
}

// C++ -> DBus
QDBusArgument &operator<<(QDBusArgument &argument, const FlexList &list)
{
    typeStream(argument, list.it, list.data);
    return argument;
}

// DBus -> C++
const QDBusArgument &operator>>(const QDBusArgument &argument, FlexList &)
{
    qFatal("not implemented");
    return argument;
}

// C++ -> DBus
QDBusArgument &operator<<(QDBusArgument &argument, const FlexKey &key)
{
    typeStream(argument, key.it, key.data);
    return argument;
}

// DBus -> C++
const QDBusArgument &operator>>(const QDBusArgument &argument, FlexKey &)
{
    qFatal("not implemented");
    return argument;
}

// C++ -> DBus
QDBusArgument &operator<<(QDBusArgument &argument, const FlexValue &value)
{
    typeStream(argument, value.it, value.data);
    return argument;
}

// DBus -> C++
const QDBusArgument &operator>>(const QDBusArgument &argument, FlexValue &)
{
    qFatal("not implemented");
    return argument;
}

void KDBusAddons::registerTypes()
{
    qDBusRegisterMetaType<FlexList>();
    qDBusRegisterMetaType<FlexKey>();
    qDBusRegisterMetaType<FlexValue>();
}
