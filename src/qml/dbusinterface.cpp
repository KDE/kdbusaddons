// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "dbusinterface.h"

#include <QQmlContext>

#include "org.freedesktop.DBus.Properties.h"
#include "pack.h"
#include "unpack.h"

using namespace Qt::StringLiterals;

namespace
{
// QML properties and signals cannot start with capital characters. To avoid this problem we simply expect everything to
// be prefixed with dbus. A bit awkward but avoids a whole host of complication.
constexpr auto dbusPrefix = "dbus"_L1;
} // namespace

DBusInterface::DBusInterface(const QString &bus, const QString &name, const QString &path, const QString &iface, QObject *parent)
    : QObject(parent)
    , m_busAddress(bus)
    , m_service(name)
    , m_path(path)
    , m_iface(iface)
{
    componentComplete();
}

void DBusInterface::classBegin()
{
    KDBusAddons::registerTypes();
}

void DBusInterface::componentComplete()
{
    m_bus = [this] {
        if (m_busAddress == "session"_L1) {
            return QDBusConnection::sessionBus();
        }
        if (m_busAddress == "system"_L1) {
            return QDBusConnection::sessionBus();
        }
        static unsigned int count = 0;
        return QDBusConnection::connectToBus(m_busAddress, u"kdbusinterface%1"_s.arg(count++));
    }();
    if (m_object) {
        m_objectJSValue = qjsEngine(m_object)->toScriptValue(m_object);
    }

    m_bus.connect(m_service, m_path, m_iface, QString(), this, SLOT(onSignal(QDBusMessage)));

    auto onPropertyWrittenIndex = metaObject()->indexOfMethod("onPropertyWritten()");
    auto onPropertyWritten = metaObject()->method(onPropertyWrittenIndex);
    if (!onPropertyWritten.isValid()) {
        qFatal("Failed to find method onPropertyWritten");
    }

    auto properties = new org::freedesktop::DBus::Properties(m_service, m_path, m_bus, this);
    connect(properties,
            &org::freedesktop::DBus::Properties::PropertiesChanged,
            this,
            [this](const QString &iface, const QVariantMap &changed, [[maybe_unused]] const QStringList &invalidated) {
                if (iface != m_iface) {
                    return;
                }

                QSignalBlocker blocker(m_object);
                for (const auto &[key, value] : changed.asKeyValueRange()) {
                    if (!m_object->setProperty(qUtf8Printable(dbusPrefix + key), value)) {
                        qWarning() << "Property" << key << "not defined in qml";
                    }
                }
            });

    // TODO don't call getall if the interface is not implemented!
    properties->setTimeout(1000);

    QVariantMap changed = properties->GetAll(m_iface);
    QSignalBlocker blocker(m_object);
    for (const auto &[key, value] : changed.asKeyValueRange()) {
        const QString name = dbusPrefix + key;
        const auto propertyIndex = m_object->metaObject()->indexOfProperty(qUtf8Printable(name));
        if (propertyIndex == -1) {
            qWarning() << "Property" << key << "not defined in qml";
            continue;
        }
        const auto property = m_object->metaObject()->property(propertyIndex);
        if (!property.isValid()) {
            qWarning() << "Property" << key << "not found in qml but doesn't resolve?";
            continue;
        }
        if (!property.write(m_object, value)) {
            qWarning() << "Failed to write property" << key;
        }
        connect(m_object, property.notifySignal(), this, onPropertyWritten);
        m_signalToPropertyIndex.insert(property.notifySignalIndex(), propertyIndex);
    }
}

void DBusInterface::asyncCall(const QString &name, const QString &signature, const QVariantList &args, const QJSValue &resolve, const QJSValue &reject)
{
    auto method = QDBusMessage::createMethodCall(m_service, m_path, m_iface, name);
    if (signature == "_implied_"_L1) {
        method.setArguments(args);
    } else if (auto err = KDBusAddons::pack(method, args, qUtf8Printable(signature)); err.isValid()) {
        reject.call({err.message()});
        return;
    }

    auto reply = m_bus.asyncCall(method);
    auto watcher = new QDBusPendingCallWatcher(reply);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, resolve, reject, watcher]() {
        watcher->deleteLater();

        auto engine = qjsEngine(this);
        if (watcher->reply().type() == QDBusMessage::ErrorMessage) {
            qWarning() << "Error calling method" << watcher->reply().errorMessage();
            reject.call({watcher->reply().errorMessage()});
            return;
        }

        QJSValueList args;
        for (const auto &arg : watcher->reply().arguments()) {
            if (arg.metaType() == QMetaType::fromType<QDBusArgument>()) {
                // For convenience reasons we unpack DBusArguments into a JSON object and then convert that to a QJSValue.
                // This way we don't have to faff about with QJSValues all the way down.
                auto dbusArg = qvariant_cast<QDBusArgument>(arg);
                auto value = engine->toScriptValue(KDBusAddons::unpack(dbusArg));
                args << value;
            } else {
                auto value = engine->toScriptValue(arg);
                args << value;
            }
        }

        // This is a bit awkward. Promise.resolve only accepts a single argument. Meaning if we get multiple arguments
        // from DBus we must pack them into a list.
        QJSValue ret;
        if (args.size() > 1) {
            // e.g. .then(([result, args]) => { ... })
            ret = resolve.call({engine->toScriptValue(args)});
        } else {
            // e.g. .then((result) => { ... })
            ret = resolve.call(args);
        }
        if (ret.isError()) {
            qWarning() << ret.toString();
        }
    });
}

QVariant DBusInterface::syncCall(const QString &name, const QString &signature, const QVariantList &args)
{
    auto context = QQmlEngine::contextForObject(this);
    auto engine = context->engine();

    auto method = QDBusMessage::createMethodCall(m_service, m_path, m_iface, name);
    if (auto err = KDBusAddons::pack(method, args, qUtf8Printable(signature)); err.isValid()) {
        qWarning() << "Error packing arguments" << err;
        engine->throwError(err.message());
        return {};
    }

    auto reply = m_bus.call(method);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Error calling method" << reply.errorMessage();
        engine->throwError(reply.errorMessage());
        return {};
    }
    return reply.arguments();
}

void DBusInterface::onSignal(const QDBusMessage &message)
{
    if (message.interface() != m_iface) {
        return;
    }

    const QString name = dbusPrefix + message.member();
    auto value = m_objectJSValue.property(name);
    if (!value.isCallable()) {
        qWarning() << "No signal handler for" << name;
        return;
    }

    QJSValueList args;
    for (const auto &arg : message.arguments()) {
        args << QQmlEngine::contextForObject(this)->engine()->toScriptValue(arg);
    }
    auto ret = value.call(args);
    if (ret.isError()) {
        qWarning() << ret.toString();
    }
}

void DBusInterface::onPropertyWritten()
{
    if (!sender() || senderSignalIndex() == -1) {
        return;
    }

    auto propertyIndex = m_signalToPropertyIndex.value(senderSignalIndex(), -1);
    if (propertyIndex == -1) {
        return;
    }

    const auto property = sender()->metaObject()->property(propertyIndex);

    auto method = QDBusMessage::createMethodCall(m_service, m_path, u"org.freedesktop.DBus.Properties"_s, u"Set"_s);
    method << property.read(m_object);
    m_bus.asyncCall(method);
}

DBusVariantWrapper *DBusInterface::qmlAttachedProperties(QObject *object)
{
    return new DBusVariantWrapper(object);
}
