// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QQmlEngine>

#include "dbusvariantwrapper.h"

class DBusInterface : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(DBusVariantWrapper)
    Q_PROPERTY(QString bus MEMBER m_busAddress NOTIFY busAddressChanged REQUIRED)
    Q_PROPERTY(QString name MEMBER m_service NOTIFY nameChanged REQUIRED)
    Q_PROPERTY(QString path MEMBER m_path NOTIFY pathChanged REQUIRED)
    Q_PROPERTY(QString iface MEMBER m_iface NOTIFY ifaceChanged REQUIRED)
    Q_PROPERTY(QObject *proxy MEMBER m_object NOTIFY proxyChanged REQUIRED)
    Q_PROPERTY(QHash<QString, QString> propertySignatures MEMBER m_propertySignatures NOTIFY propertySignaturesChanged)
public:
    using QObject::QObject;
    // Constructor for QJSEngine use without QML
    Q_INVOKABLE DBusInterface(const QString &bus, const QString &name, const QString &path, const QString &iface, QObject *parent = nullptr);

    static DBusVariantWrapper *qmlAttachedProperties(QObject *object);

    void classBegin() override;
    void componentComplete() override;

public Q_SLOTS:
    /*!
        Call a method on the DBus interface asynchronously. Best used in combination with a Promise.
        \param name The name of the method to call.
        \param signature The signature of the method arguments. May be "_implied_" in which case the signature is derived from the arguments (99.9% of the time
                         you do not want this!)
        \param args The arguments to pass to the method.
        \param resolve The function to call when the method call completes successfully. The result of the method call is passed as the first argument.
        \param reject The function to call when the method call fails. The error message is passed as the first argument.
    */
    void asyncCall(const QString &name, const QString &signature, const QVariantList &args, const QJSValue &resolve, const QJSValue &reject);

    // Use asyncCall instead.
    QVariant syncCall(const QString &name, const QString &signature, const QVariantList &args);

Q_SIGNALS:
    void nameChanged();
    void pathChanged();
    void ifaceChanged();
    void proxyChanged();
    void busAddressChanged();
    void propertySignaturesChanged();

private Q_SLOTS:
    void onSignal(const QDBusMessage &message);
    void onPropertyWritten();

private:
    QString m_busAddress;
    QDBusConnection m_bus = QDBusConnection::sessionBus();
    QString m_service;
    QString m_path;
    QString m_iface;
    QObject *m_object = nullptr;
    QJSValue m_objectJSValue;
    QHash<QString, QString> m_propertySignatures;
    QHash<int, int> m_signalToPropertyIndex;
};

