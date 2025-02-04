// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QSignalSpy>
#include <QTest>

#include <kdbuspropertieschangedadaptor.h>

using namespace std::chrono_literals;

class AdapteeBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo MEMBER m_foo NOTIFY fooChanged)
public:
    using QObject::QObject;

Q_SIGNALS:
    void fooChanged();

private:
    int m_foo = 0;
};

class NoInterfaceAdaptee : public AdapteeBase
{
};

class Adaptee : public AdapteeBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdbuspropertieschangedadaptortest")
};

class KDBusPropertiesChangedAdaptorTest : public QObject
{
    Q_OBJECT
    unsigned int m_propertiesChangedCount;
    unsigned int m_targetedPropertiesChangedCount;
    QDBusConnection m_bus = QDBusConnection::sessionBus();
    // We use a secondary connection for targeted signal reception. (i.e. m_bus will eventually send a target signal to m_targetBus)
    QDBusConnection m_targetBus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "targetConnection");

public:
    using QObject::QObject;

private Q_SLOTS:

    void initTestCase()
    {
        m_bus.connect(QString(),
                      "/org/kde/someobject",
                      "org.freedesktop.DBus.Properties",
                      "PropertiesChanged",
                      this,
                      SLOT(propertiesChanged(QString, QVariantMap, QStringList)));

        m_targetBus.connect(QString(),
                            "/org/kde/someobject",
                            "org.freedesktop.DBus.Properties",
                            "PropertiesChanged",
                            this,
                            SLOT(targetedPropertiesChanged(QString, QVariantMap, QStringList)));
    }

    void init()
    {
        m_propertiesChangedCount = 0;
        m_targetedPropertiesChangedCount = 0;
    }

    void testNoInterfaceAdaptee()
    {
        NoInterfaceAdaptee adaptee;
        KDBusPropertiesChangedAdaptor adaptor("/org/kde/someobject", &adaptee, m_bus);

        adaptee.setProperty("foo", 32);

        QTest::qWait(2s);
        QCOMPARE(m_propertiesChangedCount, 0);
    }

    void testAdaption()
    {
        Adaptee adaptee;
        KDBusPropertiesChangedAdaptor adaptor("/org/kde/someobject", &adaptee, m_bus);

        adaptee.setProperty("foo", 64);

        // Should hear this signal on our general bus AND the target bus (since it is a broadcast)
        QTRY_COMPARE_WITH_TIMEOUT(m_propertiesChangedCount, 1, 2s);
        QTRY_COMPARE_WITH_TIMEOUT(m_targetedPropertiesChangedCount, 1, 2s);
    }

    void testTargeting()
    {
        Adaptee adaptee;
        KDBusPropertiesChangedAdaptor adaptor("/org/kde/someobject", &adaptee, m_bus);
        adaptor.setTargetService(m_targetBus.baseService());

        adaptee.setProperty("foo", 128);

        // Should hear this signal on our target bus but not the general bus (since that was not targeted)
        QTRY_COMPARE_WITH_TIMEOUT(m_targetedPropertiesChangedCount, 1, 2s);
        QCOMPARE(m_propertiesChangedCount, 0);
    }

    void propertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
    {
        qDebug() << "Properties changed" << interface << changedProperties << invalidatedProperties;
        m_propertiesChangedCount++;
    }

    void targetedPropertiesChanged(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
    {
        qDebug() << "Targeted properties changed" << interface << changedProperties << invalidatedProperties;
        m_targetedPropertiesChangedCount++;
    }
};

QTEST_MAIN(KDBusPropertiesChangedAdaptorTest)

#include "kdbuspropertieschangedadaptortest.moc"
