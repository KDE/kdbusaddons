/*
    This file is part of libkdbus

    SPDX-FileCopyrightText: 2011 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QTest>

#include <kdbusinterprocesslock.h>

#include <stdio.h>

static const char *counterFileName = "kdbusinterprocesslocktest.counter";

void writeCounter(int value)
{
    QFile file(counterFileName);
    file.open(QFile::WriteOnly);
    QTextStream stream(&file);
    stream << value;
    file.close();
}

int readCounter()
{
    QFile file(counterFileName);
    file.open(QFile::ReadOnly);
    QTextStream stream(&file);

    int value = 0;
    stream >> value;

    file.close();

    return value;
}

void removeCounter()
{
    QFile::remove(counterFileName);
}

QProcess *executeNewChild()
{
    qDebug() << "executeNewChild";

    // Duplicated from kglobalsettingstest.cpp - make a shared helper method?
    QProcess *proc = new QProcess();
    QString appName = QStringLiteral("kdbusinterprocesslocktest");
#ifdef Q_OS_WIN
    appName += ".exe";
#else
    if (QFile::exists(appName + ".shell")) {
        appName = "./" + appName + ".shell";
    } else {
        Q_ASSERT(QFile::exists(appName));
        appName = "./" + appName;
    }
#endif
    proc->setProcessChannelMode(QProcess::ForwardedChannels);
    proc->start(appName, QStringList() << QStringLiteral("child"));
    return proc;
}

void work(int id, KDBusInterProcessLock &lock)
{
    for (int i = 0; i < 10; i++) {
        qDebug("%d: retrieve lock...", id);
        lock.lock();
        qDebug("%d: waiting...", id);
        lock.waitForLockGranted();
        qDebug("%d: retrieved lock", id);

        int value = readCounter() + 1;
        writeCounter(value);
        qDebug("%d: counter updated to %d", id, value);

        lock.unlock();
        qDebug("%d: sleeping", id);
        QTest::qSleep(20);
    }

    qDebug("%d: done", id);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("kdbusinterprocesslocktest"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));

    QDir::setCurrent(QCoreApplication::applicationDirPath());

    KDBusInterProcessLock lock(QStringLiteral("myfunnylock"));

    if (argc >= 2) {
        work(2, lock);
        return 0;
    }

    writeCounter(0);

    QProcess *proc = executeNewChild();
    work(1, lock);

    proc->waitForFinished();
    delete proc;

    int value = readCounter();
    qDebug("Final value: %d", value);

    const bool ok = (value == 20);

    removeCounter();

    return ok ? 0 : 1;
}
