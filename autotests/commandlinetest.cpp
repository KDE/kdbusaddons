// SPDX-FileCopyrightText: 2026 Eike Hein <hein@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <KDBusService>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDir>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QVariantMap>

class CommandLineTest : public QObject
{
    Q_OBJECT

public:
    explicit CommandLineTest(KDBusService &service)
        : m_service(service)
    {
        connect(&service, &KDBusService::activateRequested, this, [this] {
            m_service.setExitValue(4);
        });
    }

private Q_SLOTS:
    void forwarding_data()
    {
        QTest::addColumn<QStringList>("arguments");
        QTest::newRow("without-arguments") << QStringList{};
        QTest::newRow("with-arguments") << QStringList{QStringLiteral("first argument"), QStringLiteral("--second")};
    }

    // Checks directory and argument forwarding as well as reception of exit value.
    void forwarding()
    {
        QFETCH(QStringList, arguments);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QSignalSpy activated(&m_service, &KDBusService::activateRequested);
        QProcess process;
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("KDBUSSERVICE_TEST_CHILD"), QStringLiteral("1"));
        process.setProcessEnvironment(environment);
        process.setWorkingDirectory(directory.path());
        process.start(QCoreApplication::applicationFilePath(), arguments);
        QVERIFY(process.waitForStarted());
        QTRY_COMPARE(activated.count(), 1);
        QTRY_COMPARE(process.state(), QProcess::NotRunning);
        const auto forwarded = activated.constFirst().at(0).toStringList();
        QCOMPARE(forwarded.size(), arguments.size() + 1);
        QCOMPARE(forwarded.sliced(1), arguments);
        QCOMPARE(activated.constFirst().at(1).toString(), QDir(directory.path()).canonicalPath());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 4);
    }

    // Desktop activation remains available with or without argument forwarding.
    void activation()
    {
        QSignalSpy activated(&m_service, &KDBusService::activateRequested);
        auto message = QDBusMessage::createMethodCall(m_service.serviceName(),
                                                      QStringLiteral("/org/kde/commandlinetest"),
                                                      QStringLiteral("org.freedesktop.Application"),
                                                      QStringLiteral("Activate"));
        message << QVariantMap{};
        const auto call = QDBusConnection::sessionBus().asyncCall(message);
        QTRY_VERIFY(call.isFinished());
        QCOMPARE(call.reply().type(), QDBusMessage::ReplyMessage);
        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated.constFirst().at(0).toStringList(), QCoreApplication::arguments().sliced(0, 1));
        QCOMPARE(activated.constFirst().at(1).toString(), QDir::currentPath());
    }

private:
    KDBusService &m_service;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("commandlinetest"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    KDBusService service(KDBusService::Unique);

    if (qEnvironmentVariableIsSet("KDBUSSERVICE_TEST_CHILD")) {
        return 1;
    }

    CommandLineTest test(service);
    return QTest::qExec(&test, argc, argv);
}

#include "commandlinetest.moc"
