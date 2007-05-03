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

#include <QDBusInterface>

#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocale>

static KCmdLineOptions options[] =
{
    { "service", I18N_NOOP("Full service name, overrides application name provided"), 0 },
    { "path <path>", I18N_NOOP("Path in the dbus interface to use"), "/MainApplication" },
    { "+application", I18N_NOOP("The name of the application to quit"), 0 },
    KCmdLineLastOption
};

int main(int argc, char* argv[])
{
    KAboutData aboutData( "kquitapp", I18N_NOOP("Command line application quitter"),
                          "1.0", I18N_NOOP("Quit a DBUS enabled application easily"), KAboutData::License_GPL,
                           I18N_NOOP("(c) 2006, Aaron Seigo") );
    aboutData.addAuthor("Aaron J. Seigo", I18N_NOOP("Current maintainer"), "aseigo@kde.org");
    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

    if (args->count() == 0)
    {
        args->usage(0);
        return 1;
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
    interface.call("quit");
    return 0;
}

