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
#include <QDBusInterface>

#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocale>
#include <KDebug>

int main(int argc, char* argv[])
{
    KAboutData aboutData( "kquitapp", 0, ki18n("Command-line application quitter"),
                          "1.0", ki18n("Quit a D-Bus enabled application easily"), KAboutData::License_GPL,
                           ki18n("(c) 2006, Aaron Seigo") );
    aboutData.addAuthor(ki18n("Aaron J. Seigo"), ki18n("Current maintainer"), "aseigo@kde.org");
    KCmdLineArgs::init(argc, argv, &aboutData);
    QCoreApplication app(argc, argv);

    KCmdLineOptions options;
    options.add("service <service>", ki18n("Full service name, overrides application name provided"));
    options.add("path <path>", ki18n("Path in the D-Bus interface to use"), "/MainApplication");
    options.add("+application", ki18n("The name of the application to quit"));
    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    if (args->count() == 0)
    {
        args->usage(0);
    }

    QString service;
    if (args->isSet("service"))
    {
        service = args->getOption("service");
    }
    else
    {
        service = QString("org.kde.%1").arg(QString(args->arg(0)));
    }

    QString path("/MainApplication");
    if (args->isSet("path"))
    {
        path = args->getOption("path");
    }

    QDBusInterface interface(service, path);
    if (!interface.isValid()) {
        kError() << i18n("Application %1 could not be found using service %2 and path %3.",args->arg(0),service,path);
        return 1;
    }
    interface.call("quit");
    QDBusError error = interface.lastError();
    if (error.type() != QDBusError::NoError) {
        kError() << i18n("Quitting application %1 failed. Error reported was:\n\n     %2 : %3",args->arg(0),error.name(), error.message());
        return 1;
    }
    return 0;
}

