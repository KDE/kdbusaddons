/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDBusInterface>
#include <QDebug>

#include <KAboutData>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    KAboutData aboutData( "kquitapp", 0, QCoreApplication::translate("main", "Command-line application quitter"),
                          "1.0", QCoreApplication::translate("main", "Quit a D-Bus enabled application easily"), KAboutData::License_GPL,
                           "(c) 2006, Aaron Seigo" );
    aboutData.addAuthor("Aaron J. Seigo", QCoreApplication::translate("main", "Current maintainer"), "aseigo@kde.org");

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption("service", QCoreApplication::translate("main", "Full service name, overrides application name provided"), "service"));
    parser.addOption(QCommandLineOption("path", QCoreApplication::translate("main", "Path in the D-Bus interface to use"), "path", "/MainApplication"));
    parser.addPositionalArgument("application", QCoreApplication::translate("main", "The name of the application to quit"));
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    if(parser.positionalArguments().isEmpty())
        parser.showHelp(1);

    QString service;
    if (parser.isSet(QStringLiteral("service")))
    {
        service = parser.value("service");
    }
    else
    {
        service = QStringLiteral("org.kde.%1").arg(parser.positionalArguments()[0]);
    }

    QString path(parser.value("path"));

    QDBusInterface interface(service, path);
    if (!interface.isValid()) {
        qWarning() << QCoreApplication::translate("main", "Application %1 could not be found using service %2 and path %3.").arg(parser.positionalArguments().first()).arg(service).arg(path);
        return 1;
    }
    interface.call("quit");
    QDBusError error = interface.lastError();
    if (error.type() != QDBusError::NoError) {
        qWarning() << QCoreApplication::translate("main", "Quitting application %1 failed. Error reported was:\n\n     %2 : %3").arg(parser.positionalArguments().first()).arg(error.name()).arg(error.message());
        return 1;
    }
    return 0;
}

