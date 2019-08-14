/* This file is part of libkdbus

   Copyright (c) 2019 Harald Sitter <sitter@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDebug>
#include <QProcess>
#include <QTest>

#include <signal.h>
#include <unistd.h>

static const QString s_serviceName = QStringLiteral("org.kde.kdbussimpleservice");

class TestObject : public QObject
{
    Q_OBJECT

    QList<int> m_danglingPids;
private Q_SLOTS:
    void cleanupTestCase()
    {
        // Make sure we don't leave dangling processes even when we had an
        // error and the process is stopped.
        for (int pid : m_danglingPids) {
            kill(pid, SIGKILL);
        }
    }

    void testDeadService()
    {
        QVERIFY(!QDBusConnection::sessionBus().interface()->isServiceRegistered(s_serviceName).value());

        QProcess proc1;
        proc1.setProgram(QFINDTESTDATA("kdbussimpleservice"));
        proc1.setProcessChannelMode(QProcess::ForwardedChannels);
        proc1.start();
        QVERIFY(proc1.waitForStarted());
        m_danglingPids << proc1.pid();

        // Spy isn't very suitable here because we'd be racing with proc1 or
        // signal blocking since we'd need to unblock before spying but then
        // there is an ɛ between unblock and spy.
        Q_PID pid1 = proc1.pid(); // store local, in case the proc disappears
        QVERIFY(pid1 >= 0);
        bool proc1Registered = QTest::qWaitFor([&]() {
            QTest::qSleep(1000);
            return QDBusConnection::sessionBus().interface()->servicePid(s_serviceName).value() == pid1;
        }, 8000);
        QVERIFY(proc1Registered);

        // suspend proc1, we don't want it responding on dbus anymore, but still
        // be running so it holds the name.
        QCOMPARE(kill(proc1.pid(), SIGSTOP), 0);

        // start second instance
        QProcess proc2;
        proc2.setProgram(QFINDTESTDATA("kdbussimpleservice"));
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("KCRASH_AUTO_RESTARTED", "1");
        proc2.setProcessEnvironment(env);
        proc2.setProcessChannelMode(QProcess::ForwardedChannels);
        proc2.start();
        QVERIFY(proc2.waitForStarted());
        m_danglingPids << proc2.pid();

        // sleep a bit. fairly awkward. we need proc2 to be waiting on the name
        // but we can't easily determine when it started waiting. in lieu of
        // better instrumentation let's just sleep a bit.
        qDebug() << "sleeping";
        QTest::qSleep(4000);

        // Let proc1 go up in flames so that dbus-daemon reclaims the name and
        // gives it to proc2.
        qDebug() << "murder on the orient express";
        QCOMPARE(0, kill(proc1.pid(), SIGUSR1));
        QCOMPARE(0, kill(proc1.pid(), SIGCONT));

        Q_PID pid2 = proc2.pid(); // store local, in case the proc disappears
        QVERIFY(pid2 >= 0);
        // Wait for service to be owned by proc2.
        bool proc2Registered = QTest::qWaitFor([&]() {
            QTest::qSleep(1000);
            return QDBusConnection::sessionBus().interface()->servicePid(s_serviceName).value() == pid2;
        }, 8000);
        QVERIFY(proc2Registered);

        proc1.kill();
        m_danglingPids.removeAll(pid1);
        proc2.kill();
        m_danglingPids.removeAll(pid2);
    }
};

QTEST_MAIN(TestObject)

#include "deadservicetest.moc"
