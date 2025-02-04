// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "kdbuspropertieschangedadaptor.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QMetaProperty>

#include "kdbusaddons_debug.h"

using namespace Qt::StringLiterals;

class KDBusPropertiesChangedAdaptorPrivate
{
public:
    QString m_objectPath;
    QDBusConnection m_bus;
    QString m_targetService;
};

KDBusPropertiesChangedAdaptor::KDBusPropertiesChangedAdaptor(const QString &objectPath, QObject *adaptee, const QDBusConnection &bus, QObject *parent)
    : QObject(parent)
    , d(std::make_unique<KDBusPropertiesChangedAdaptorPrivate>(objectPath, bus))
{
    const auto onPropertyChangedIndex = metaObject()->indexOfMethod("_k_onPropertyChanged()"); // of this adaptor
    Q_ASSERT(onPropertyChangedIndex != -1);
    const auto propertyChangedMethod = metaObject()->method(onPropertyChangedIndex);

    auto mo = adaptee->metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        const auto property = mo->property(i);
        if (!property.hasNotifySignal()) {
            continue;
        }
        connect(adaptee, property.notifySignal(), this, propertyChangedMethod);
    }
}

KDBusPropertiesChangedAdaptor::~KDBusPropertiesChangedAdaptor() = default;

void KDBusPropertiesChangedAdaptor::_k_onPropertyChanged()
{
    if (!sender() || senderSignalIndex() == -1) {
        return;
    }

    auto mo = sender()->metaObject();
    for (int i = 0; i < mo->propertyCount(); ++i) {
        // Find the appropriate property to get access to name and value.
        QMetaProperty property = mo->property(i);
        if (!property.hasNotifySignal()) {
            continue;
        }
        if (property.notifySignalIndex() != senderSignalIndex()) {
            continue;
        }
        const int dbusInterfaceClassInfo = mo->indexOfClassInfo("D-Bus Interface");
        if (dbusInterfaceClassInfo == -1) {
            qCWarning(KDBUSADDONS_LOG) << "Object" << sender() << "has no D-Bus interface!";
            continue;
        }

        QDBusMessage signal = [this] {
            constexpr auto iface = "org.freedesktop.DBus.Properties"_L1;
            constexpr auto signal = "PropertiesChanged"_L1;
            if (!d->m_targetService.isEmpty()) {
                return QDBusMessage::createTargetedSignal(d->m_targetService, //
                                                          d->m_objectPath,
                                                          iface,
                                                          signal);
            }
            return QDBusMessage::createSignal(d->m_objectPath, //
                                              iface,
                                              signal);
        }();
        signal << QString::fromLatin1(mo->classInfo(dbusInterfaceClassInfo).value());
        signal << QVariantMap({{QString::fromLatin1(property.name()), property.read(sender())}}); // changed properties DICT<STRING,VARIANT>
        signal << QStringList(); // invalidated_properties is an array of properties that changed but the value is not conveyed
        d->m_bus.send(signal);
    }
}

void KDBusPropertiesChangedAdaptor::setTargetService(const QString &service)
{
    d->m_targetService = service;
}
