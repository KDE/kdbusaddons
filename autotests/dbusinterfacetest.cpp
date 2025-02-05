// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTest>
#include <QtQuickTest/quicktest.h>

using namespace Qt::StringLiterals;

class Daemon : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.dbusaddons.dbusinterfacetest")
public:
    explicit Daemon(const QDBusConnection &bus, QObject *parent = nullptr)
        : QObject(parent)
        , m_bus(bus)
        , m_unixFdSupport(bus.connectionCapabilities().testFlag(QDBusConnection::UnixFileDescriptorPassing))
    {
    }

public Q_SLOTS:
    void method1()
    {
    }

    void method2(const QDBusMessage &message)
    {
        qWarning() << Q_FUNC_INFO << message;
        Q_ASSERT(message.signature() == "as"_L1);
    }

    void method3(const QDBusMessage &message)
    {
        qWarning() << Q_FUNC_INFO << message;
        if (m_unixFdSupport) {
            Q_ASSERT(message.signature() == "sahta(sv)"_L1);
        } else {
            Q_ASSERT(message.signature() == "sta(sv)"_L1);
        }
    }

    void method4(const QDBusMessage &message)
    {
        qWarning() << Q_FUNC_INFO << message;
        Q_ASSERT(message.signature() == "(sis)"_L1);
    }

    void method5(const QDBusMessage &message)
    {
        qWarning() << Q_FUNC_INFO << message;
        Q_ASSERT(message.signature() == "aa{sv}aaa(sis)"_L1);

        // There is a lot that can go wrong here so we also check the content!
        // It's very verbose, but also makes it very explicit what we expect.
        // Note that QDBusArgument and libdbus are a bit assertive too, so we don't
        // need to assert every little thing here.

        const auto args = message.arguments();
        QCOMPARE(args.size(), 2);

        // An array of maps with a single map and two entries
        {
            const auto &arg = args.at(0);
            const auto dbusArg = qvariant_cast<QDBusArgument>(arg);
            dbusArg.beginArray();
            QCOMPARE(dbusArg.currentSignature(), "a{sv}"_L1);
            dbusArg.beginMap();
            {
                dbusArg.beginMapEntry();
                QString key;
                QVariant value;
                dbusArg >> key >> value;
                QCOMPARE(key, "a"_L1);
                QCOMPARE(value, QVariant(1));
                dbusArg.endMapEntry();
            }
            {
                dbusArg.beginMapEntry();
                QString key;
                QVariant value;
                dbusArg >> key >> value;
                QCOMPARE(key, "b"_L1);
                QCOMPARE(value, QVariant(2));
                dbusArg.endMapEntry();
            }
            dbusArg.endMap();
            dbusArg.endArray();
        }

        // An array of arrays of arrays of structs(string,int,string)
        {
            const auto &arg = args.at(1);
            const auto dbusArg = qvariant_cast<QDBusArgument>(arg);
            dbusArg.beginArray();
            {
                QCOMPARE(dbusArg.currentSignature(), "aa(sis)"_L1);
                dbusArg.beginArray();
                {
                    dbusArg.beginArray();
                    {
                        dbusArg.beginStructure();
                        {
                            QString a;
                            int b;
                            QString c;
                            dbusArg >> a >> b >> c;
                            QCOMPARE(a, "a"_L1);
                            QCOMPARE(b, 1);
                            QCOMPARE(c, "c"_L1);
                        }
                        dbusArg.endStructure();
                    }
                    dbusArg.endArray();
                }
                dbusArg.endArray();
            }
            dbusArg.endArray();
        }
    }

    void method6(const QDBusMessage &message)
    {
        qWarning() << Q_FUNC_INFO << message;
        Q_ASSERT(message.signature() == "(sa(sis)sas)"_L1);
    }

    void method7(const QDBusMessage &message)
    {
        qWarning() << Q_FUNC_INFO << message;
        Q_ASSERT(message.signature() == "a{sv}"_L1);
    }

    void method8(const QDBusMessage &message)
    {
        qWarning() << Q_FUNC_INFO << message;
        QCOMPARE(message.signature(), "av"_L1);

        const auto args = message.arguments();
        QCOMPARE(args.size(), 1);

        const auto &arg = args.at(0);
        const auto dbusArg = qvariant_cast<QDBusArgument>(arg);
        // This contains an array of variants.
        dbusArg.beginArray();
        {
            QVariant value;

            // Variant 1 is a uchar(1)
            dbusArg >> value;
            QCOMPARE(value, QVariant(1));
            QCOMPARE(value.metaType(), QMetaType::fromType<uchar>());

            // Variant 2 is a string("foo")
            dbusArg >> value;
            QCOMPARE(value, QVariant(2));
            QCOMPARE(value.metaType(), QMetaType::fromType<uint>());

            // Variant 3 is a nested variant array. This gets represented as a nested QDBusArgument. Mind you this seems
            // to be the correct behavior. If you stream a nested QVariantList manually it comes out looking the same.
            dbusArg >> value;
            const auto innerArg = qvariant_cast<QDBusArgument>(value);
            innerArg.beginArray();
            {
                // Variant 1 is a qulonglong(3)
                innerArg >> value;
                QCOMPARE(value, QVariant(3));
                QCOMPARE(value.metaType(), QMetaType::fromType<qulonglong>());

                // Variant 2 is a qlonglong(4)
                innerArg >> value;
                QCOMPARE(value, QVariant(4));
                QCOMPARE(value.metaType(), QMetaType::fromType<qlonglong>());
            }

            // Variant 4 is a stringlist(5,6)
            dbusArg >> value;
            QCOMPARE(value, QVariant(QStringList{u"5"_s, u"6"_s}));
            QCOMPARE(value.metaType(), QMetaType::fromType<QStringList>());
        }
        dbusArg.endArray();
    }

    void singleReply(const QDBusMessage &message)
    {
        auto reply = message.createReply(1);
        m_bus.send(reply);
    }

    void manyReplies(const QDBusMessage &message)
    {
        auto reply = message.createReply({1, 2});
        m_bus.send(reply);
    }

    Q_NOREPLY void noReply([[maybe_unused]] const QDBusMessage &message)
    {
    }

    void lightTheBatSignal(const QString &message)
    {
        qWarning() << Q_FUNC_INFO << message;
        Q_EMIT BatSignal(message);
    }

Q_SIGNALS:
    void BatSignal(const QString &message);

private:
    QDBusConnection m_bus;
    bool m_unixFdSupport;
};

class Setup : public QObject
{
    Q_OBJECT
    QDBusConnection bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "testinterface");
    Daemon *daemon = new Daemon(bus, this);

public:
    using QObject::QObject;

public Q_SLOTS:
    void applicationAvailable()
    {
        bus.registerObject("/", daemon, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->rootContext()->setContextProperty("busAddress", bus.baseService());
    }
};

// The macro is being a bit meh in finding our tst_ files. Let's use a manual main for now.
int main(int argc, char **argv)
{
    const auto sourceDir = QFileInfo(QFINDTESTDATA("tst_dbusinterfacetest.qml")).dir().absolutePath();
    QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR);
    Setup setup;
    return quick_test_main_with_setup(argc, argv, "TestQML", qUtf8Printable(sourceDir), &setup);
}

#include "dbusinterfacetest.moc"
