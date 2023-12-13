// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <stdio.h>
#include <stdlib.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtCore/qmetaobject.h>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusVariant>
#include <QtDBus/QDBusArgument>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusUnixFileDescriptor>

QT_BEGIN_NAMESPACE
Q_DBUS_EXPORT extern bool qt_dbus_metaobject_skip_annotations;
QT_END_NAMESPACE

#define DBUS_MAXIMUM_NAME_LENGTH 255

using namespace Qt::StringLiterals;

bool isAsciiDigit(char32_t c) noexcept
{
    return c >= '0' && c <= '9';
}

bool isAsciiUpper(char32_t c) noexcept
{
    return c >= 'A' && c <= 'Z';
}

bool isAsciiLower(char32_t c) noexcept
{
    return c >= 'a' && c <= 'z';
}

bool isAsciiLetterOrNumber(char32_t c) noexcept
{
    return  isAsciiDigit(c) || isAsciiLower(c) || isAsciiUpper(c);
}

bool isValidNumber(QChar c)
{
    return (isAsciiDigit(c.toLatin1()));
}

bool isValidCharacterNoDash(QChar c)
{
    ushort u = c.unicode();
    return isAsciiLetterOrNumber(u) || (u == '_');
}

bool isValidPartOfObjectPath(QStringView part)
{
    if (part.isEmpty())
        return false;       // can't be valid if it's empty

    const QChar *c = part.data();
    for (int i = 0; i < part.size(); ++i)
        if (!isValidCharacterNoDash(c[i]))
            return false;

    return true;
}

bool isValidObjectPath(const QString &path)
{
    if (path == "/"_L1)
        return true;

    if (!path.startsWith(u'/') || path.indexOf("//"_L1) != -1 ||
        path.endsWith(u'/'))
        return false;

    // it starts with /, so we skip the empty first part
    const auto parts = QStringView{path}.mid(1).split(u'/');
    for (QStringView part : parts)
        if (!isValidPartOfObjectPath(part))
            return false;

    return true;
}

bool isValidMemberName(QStringView memberName)
{
    if (memberName.isEmpty() || memberName.size() > DBUS_MAXIMUM_NAME_LENGTH)
        return false;

    const QChar* c = memberName.data();
    if (isValidNumber(c[0]))
        return false;
    for (int j = 0; j < memberName.size(); ++j)
        if (!isValidCharacterNoDash(c[j]))
            return false;
    return true;
}

bool isValidInterfaceName(const QString& ifaceName)
{
    if (ifaceName.isEmpty() || ifaceName.size() > DBUS_MAXIMUM_NAME_LENGTH)
        return false;

    const auto parts = QStringView{ifaceName}.split(u'.');
    if (parts.size() < 2)
        return false;           // at least two parts

    for (auto part : parts)
        if (!isValidMemberName(part))
            return false;

    return true;
}

bool isValidCharacter(QChar c)
{
    ushort u = c.unicode();
    return isAsciiLetterOrNumber(u)
            || (u == '_') || (u == '-');
}

bool isValidUniqueConnectionName(QStringView connName)
{
    if (connName.isEmpty() || connName.size() > DBUS_MAXIMUM_NAME_LENGTH ||
        !connName.startsWith(u':'))
        return false;

    const auto parts = connName.mid(1).split(u'.');
    if (parts.size() < 1)
        return false;

    for (QStringView part : parts) {
        if (part.isEmpty())
                return false;

        const QChar* c = part.data();
        for (int j = 0; j < part.size(); ++j)
            if (!isValidCharacter(c[j]))
                return false;
    }

    return true;
}

bool isValidBusName(const QString &busName)
{
    if (busName.isEmpty() || busName.size() > DBUS_MAXIMUM_NAME_LENGTH)
        return false;

    if (busName.startsWith(u':'))
        return isValidUniqueConnectionName(busName);

    const auto parts = QStringView{busName}.split(u'.');
    if (parts.size() < 1)
        return false;

    for (QStringView part : parts) {
        if (part.isEmpty())
            return false;

        const QChar *c = part.data();
        if (isValidNumber(c[0]))
            return false;
        for (int j = 0; j < part.size(); ++j)
            if (!isValidCharacter(c[j]))
                return false;
    }

    return true;
}

static bool argToString(const QDBusArgument &arg, QString &out);

static bool variantToString(const QVariant &arg, QString &out)
{
    int argType = arg.metaType().id();

    if (argType == QMetaType::QStringList) {
        out += u'{';
        const QStringList list = arg.toStringList();
        for (const QString &item : list)
            out += u'\"' + item + "\", "_L1;
        if (!list.isEmpty())
            out.chop(2);
        out += u'}';
    } else if (argType == QMetaType::QByteArray) {
        out += u'{';
        QByteArray list = arg.toByteArray();
        for (int i = 0; i < list.size(); ++i) {
            out += QString::number(list.at(i));
            out += ", "_L1;
        }
        if (!list.isEmpty())
            out.chop(2);
        out += u'}';
    } else if (argType == QMetaType::QVariantList) {
        out += u'{';
        const QList<QVariant> list = arg.toList();
        for (const QVariant &item : list) {
            if (!variantToString(item, out))
                return false;
            out += ", "_L1;
        }
        if (!list.isEmpty())
            out.chop(2);
        out += u'}';
    } else if (argType == QMetaType::Char || argType == QMetaType::Short || argType == QMetaType::Int
               || argType == QMetaType::Long || argType == QMetaType::LongLong) {
        out += QString::number(arg.toLongLong());
    } else if (argType == QMetaType::UChar || argType == QMetaType::UShort || argType == QMetaType::UInt
               || argType == QMetaType::ULong || argType == QMetaType::ULongLong) {
        out += QString::number(arg.toULongLong());
    } else if (argType == QMetaType::Double) {
        out += QString::number(arg.toDouble());
    } else if (argType == QMetaType::Bool) {
        out += arg.toBool() ? "true"_L1 : "false"_L1;
    } else if (argType == qMetaTypeId<QDBusArgument>()) {
        argToString(qvariant_cast<QDBusArgument>(arg), out);
    } else if (argType == qMetaTypeId<QDBusObjectPath>()) {
        const QString path = qvariant_cast<QDBusObjectPath>(arg).path();
        out += "[ObjectPath: "_L1;
        out += path;
        out += u']';
    } else if (argType == qMetaTypeId<QDBusSignature>()) {
        out += "[Signature: "_L1 + qvariant_cast<QDBusSignature>(arg).signature();
        out += u']';
    } else if (argType == qMetaTypeId<QDBusUnixFileDescriptor>()) {
        out += "[Unix FD: "_L1;
        out += qvariant_cast<QDBusUnixFileDescriptor>(arg).isValid() ? "valid"_L1 : "not valid"_L1;
        out += u']';
    } else if (argType == qMetaTypeId<QDBusVariant>()) {
        const QVariant v = qvariant_cast<QDBusVariant>(arg).variant();
        out += "[Variant"_L1;
        QMetaType vUserType = v.metaType();
        if (vUserType != QMetaType::fromType<QDBusVariant>()
                && vUserType != QMetaType::fromType<QDBusSignature>()
                && vUserType != QMetaType::fromType<QDBusObjectPath>()
                && vUserType != QMetaType::fromType<QDBusArgument>())
            out += u'(' + QLatin1StringView(v.typeName()) + u')';
        out += ": "_L1;
        if (!variantToString(v, out))
            return false;
        out += u']';
    } else if (arg.canConvert<QString>()) {
        out += u'\"' + arg.toString() + u'\"';
    } else {
        out += u'[';
        out += QLatin1StringView(arg.typeName());
        out += u']';
    }

    return true;
}

bool argToString(const QDBusArgument &busArg, QString &out)
{
    QString busSig = busArg.currentSignature();
    bool doIterate = false;
    QDBusArgument::ElementType elementType = busArg.currentType();

    if (elementType != QDBusArgument::BasicType && elementType != QDBusArgument::VariantType
            && elementType != QDBusArgument::MapEntryType)
        out += "[Argument: "_L1 + busSig + u' ';

    switch (elementType) {
        case QDBusArgument::BasicType:
        case QDBusArgument::VariantType:
            if (!variantToString(busArg.asVariant(), out))
                return false;
            break;
        case QDBusArgument::StructureType:
            busArg.beginStructure();
            doIterate = true;
            break;
        case QDBusArgument::ArrayType:
            busArg.beginArray();
            out += u'{';
            doIterate = true;
            break;
        case QDBusArgument::MapType:
            busArg.beginMap();
            out += u'{';
            doIterate = true;
            break;
        case QDBusArgument::MapEntryType:
            busArg.beginMapEntry();
            if (!variantToString(busArg.asVariant(), out))
                return false;
            out += " = "_L1;
            if (!argToString(busArg, out))
                return false;
            busArg.endMapEntry();
            break;
        case QDBusArgument::UnknownType:
        default:
            out += "<ERROR - Unknown Type>"_L1;
            return false;
    }
    if (doIterate && !busArg.atEnd()) {
        while (!busArg.atEnd()) {
            if (!argToString(busArg, out))
                return false;
            out += ", "_L1;
        }
        out.chop(2);
    }
    switch (elementType) {
        case QDBusArgument::BasicType:
        case QDBusArgument::VariantType:
        case QDBusArgument::UnknownType:
        case QDBusArgument::MapEntryType:
            // nothing to do
            break;
        case QDBusArgument::StructureType:
            busArg.endStructure();
            break;
        case QDBusArgument::ArrayType:
            out += u'}';
            busArg.endArray();
            break;
        case QDBusArgument::MapType:
            out += u'}';
            busArg.endMap();
            break;
    }

    if (elementType != QDBusArgument::BasicType && elementType != QDBusArgument::VariantType
            && elementType != QDBusArgument::MapEntryType)
        out += u']';

    return true;
}

QString argumentToString(const QVariant &arg)
{
    QString out;

#ifndef QT_BOOTSTRAPPED
    variantToString(arg, out);
#else
    Q_UNUSED(arg);
#endif

    return out;
}

static QDBusConnection connection(QLatin1String(""));
static bool printArgumentsLiterally = false;

static void showUsage()
{
    printf("Usage: qdbus [--system] [--bus busaddress] [--literal] [servicename] [path] [method] [args]\n"
           "\n"
           "  servicename       the service to connect to (e.g., org.freedesktop.DBus)\n"
           "  path              the path to the object (e.g., /)\n"
           "  method            the method to call, with or without the interface\n"
           "  args              arguments to pass to the call\n"
           "With 0 arguments, qdbus will list the services available on the bus\n"
           "With just the servicename, qdbus will list the object paths available on the service\n"
           "With service name and object path, qdbus will list the methods, signals and properties available on the object\n"
           "\n"
           "Options:\n"
           "  --system          connect to the system bus\n"
           "  --bus busaddress  connect to a custom bus\n"
           "  --literal         print replies literally\n"
           );
}

static void printArg(const QVariant &v)
{
    if (printArgumentsLiterally) {
        printf("%s\n", qPrintable(argumentToString(v)));
        return;
    }

    if (v.metaType() == QMetaType::fromType<QStringList>()) {
        const QStringList sl = v.toStringList();
        for (const QString &s : sl)
            printf("%s\n", qPrintable(s));
    } else if (v.metaType() == QMetaType::fromType<QVariantList>()) {
        const QVariantList vl = v.toList();
        for (const QVariant &var : vl)
            printArg(var);
    } else if (v.metaType() == QMetaType::fromType<QVariantMap>()) {
        const QVariantMap map = v.toMap();
        QVariantMap::ConstIterator it = map.constBegin();
        for ( ; it != map.constEnd(); ++it) {
            printf("%s: ", qPrintable(it.key()));
            printArg(it.value());
        }
    } else if (v.metaType() == QMetaType::fromType<QDBusVariant>()) {
        printArg(qvariant_cast<QDBusVariant>(v).variant());
    } else if (v.metaType() == QMetaType::fromType<QDBusArgument>()) {
        QDBusArgument arg = qvariant_cast<QDBusArgument>(v);
        if (arg.currentSignature() == QLatin1String("av"))
            printArg(qdbus_cast<QVariantList>(arg));
        else if (arg.currentSignature() == QLatin1String("a{sv}"))
            printArg(qdbus_cast<QVariantMap>(arg));
        else
            printf("qdbus: I don't know how to display an argument of type '%s', run with --literal.\n",
                   qPrintable(arg.currentSignature()));
    } else if (v.metaType().isValid()) {
        printf("%s\n", qPrintable(v.toString()));
    }
}

static void listObjects(const QString &service, const QString &path)
{
    // make a low-level call, to avoid introspecting the Introspectable interface
    QDBusMessage call = QDBusMessage::createMethodCall(service, path.isEmpty() ? QLatin1String("/") : path,
                                                       QLatin1String("org.freedesktop.DBus.Introspectable"),
                                                       QLatin1String("Introspect"));
    QDBusReply<QString> xml = connection.call(call);

    if (path.isEmpty()) {
        // top-level
        if (xml.isValid()) {
            printf("/\n");
        } else {
            QDBusError err = xml.error();
            if (err.type() == QDBusError::ServiceUnknown)
                fprintf(stderr, "Service '%s' does not exist.\n", qPrintable(service));
            else
                printf("Error: %s\n%s\n", qPrintable(err.name()), qPrintable(err.message()));
            exit(2);
        }
    } else if (!xml.isValid()) {
        // this is not the first object, just fail silently
        return;
    }

    QDomDocument doc;
    doc.setContent(xml.value());
    QDomElement node = doc.documentElement();
    QDomElement child = node.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QLatin1String("node")) {
            QString sub = path + QLatin1Char('/') + child.attribute(QLatin1String("name"));
            printf("%s\n", qPrintable(sub));
            listObjects(service, sub);
        }
        child = child.nextSiblingElement();
    }
}

static void listInterface(const QString &service, const QString &path, const QString &interface)
{
    QDBusInterface iface(service, path, interface, connection);
    if (!iface.isValid()) {
        QDBusError err(iface.lastError());
        fprintf(stderr, "Interface '%s' not available in object %s at %s:\n%s (%s)\n",
                qPrintable(interface), qPrintable(path), qPrintable(service),
                qPrintable(err.name()), qPrintable(err.message()));
        exit(1);
    }
    const QMetaObject *mo = iface.metaObject();

    // properties
    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        QMetaProperty mp = mo->property(i);
        printf("property ");

        if (mp.isReadable() && mp.isWritable())
            printf("readwrite");
        else if (mp.isReadable())
            printf("read");
        else
            printf("write");

        printf(" %s %s.%s\n", mp.typeName(), qPrintable(interface), mp.name());
    }

    // methods (signals and slots)
    for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
        QMetaMethod mm = mo->method(i);

        QByteArray signature = mm.methodSignature();
        signature.truncate(signature.indexOf('('));
        printf("%s %s%s%s %s.%s(",
               mm.methodType() == QMetaMethod::Signal ? "signal" : "method",
               mm.tag(), *mm.tag() ? " " : "",
               *mm.typeName() ? mm.typeName() : "void",
               qPrintable(interface), signature.constData());

        QList<QByteArray> types = mm.parameterTypes();
        QList<QByteArray> names = mm.parameterNames();
        bool first = true;
        for (int i = 0; i < types.size(); ++i) {
            printf("%s%s",
                   first ? "" : ", ",
                   types.at(i).constData());
            if (!names.at(i).isEmpty())
                printf(" %s", names.at(i).constData());
            first = false;
        }
        printf(")\n");
    }
}

static void listAllInterfaces(const QString &service, const QString &path)
{
    // make a low-level call, to avoid introspecting the Introspectable interface
    QDBusMessage call = QDBusMessage::createMethodCall(service, path.isEmpty() ? QLatin1String("/") : path,
                                                       QLatin1String("org.freedesktop.DBus.Introspectable"),
                                                       QLatin1String("Introspect"));
    QDBusReply<QString> xml = connection.call(call);

    if (!xml.isValid()) {
        QDBusError err = xml.error();
        if (err.type() == QDBusError::ServiceUnknown)
            fprintf(stderr, "Service '%s' does not exist.\n", qPrintable(service));
        else
            printf("Error: %s\n%s\n", qPrintable(err.name()), qPrintable(err.message()));
        exit(2);
    }

    QDomDocument doc;
    doc.setContent(xml.value());
    QDomElement node = doc.documentElement();
    QDomElement child = node.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QLatin1String("interface")) {
            QString ifaceName = child.attribute(QLatin1String("name"));
            if (isValidInterfaceName(ifaceName))
                listInterface(service, path, ifaceName);
            else {
                qWarning("Invalid D-BUS interface name '%s' found while parsing introspection",
                         qPrintable(ifaceName));
            }
        }
        child = child.nextSiblingElement();
    }
}

static QStringList readList(QStringList &args)
{
    args.takeFirst();

    QStringList retval;
    while (!args.isEmpty() && args.at(0) != QLatin1String(")"))
        retval += args.takeFirst();

    if (args.value(0) == QLatin1String(")"))
        args.takeFirst();

    return retval;
}

static int placeCall(const QString &service, const QString &path, const QString &interface,
               const QString &member, const QStringList& arguments, bool try_prop=true)
{
    QDBusInterface iface(service, path, interface, connection);

    // Don't check whether the interface is valid to allow DBus try to
    // activate the service if possible.

    QList<int> knownIds;
    bool matchFound = false;
    QStringList args = arguments;
    QVariantList params;
    if (!args.isEmpty()) {
        const QMetaObject *mo = iface.metaObject();
        QByteArray match = member.toLatin1();
        match += '(';

        for (int i = mo->methodOffset(); i < mo->methodCount(); ++i) {
            QMetaMethod mm = mo->method(i);
            QByteArray signature = mm.methodSignature();
            if (signature.startsWith(match))
                knownIds += i;
         }


        while (!matchFound) {
            args = arguments; // reset
            params.clear();
            if (knownIds.isEmpty()) {
                // Failed to set property after falling back?
                // Bail out without displaying an error
                if (!try_prop)
                    return 1;
                if (try_prop && args.size() == 1) {
                    QStringList proparg;
                    proparg += interface;
                    proparg += member;
                    proparg += args.first();
                    if (!placeCall(service, path, "org.freedesktop.DBus.Properties", "Set", proparg, false))
                        return 0;
                }
                fprintf(stderr, "Cannot find '%s.%s' in object %s at %s\n",
                        qPrintable(interface), qPrintable(member), qPrintable(path),
                        qPrintable(service));
                return 1;
            }

            QMetaMethod mm = mo->method(knownIds.takeFirst());
            QList<QByteArray> types = mm.parameterTypes();
            for (int i = 0; i < types.size(); ++i) {
                if (types.at(i).endsWith('&')) {
                    // reference (and not a reference to const): output argument
                    // we're done with the inputs
                    while (types.size() > i)
                        types.removeLast();
                    break;
                }
            }

            for (int i = 0; !args.isEmpty() && i < types.size(); ++i) {
                const QMetaType metaType = QMetaType::fromName(types.at(i));
                if (!metaType.isValid()) {
                    fprintf(stderr, "Cannot call method '%s' because type '%s' is unknown to this tool\n",
                            qPrintable(member), types.at(i).constData());
                    return 1;
                }
                const int id = metaType.id();

                QVariant p;
                QString argument;
                if ((id == QMetaType::QVariantList || id == QMetaType::QStringList)
                     && args.at(0) == QLatin1String("("))
                    p = readList(args);
                else
                    p = argument = args.takeFirst();

                if (id == QMetaType::UChar) {
                    // special case: QVariant::convert doesn't convert to/from
                    // UChar because it can't decide if it's a character or a number
                    p = QVariant::fromValue<uchar>(p.toUInt());
                } else if (id < QMetaType::User && id != QMetaType::QVariantMap) {
                    p.convert(metaType);
                    if (!p.isValid()) {
                        fprintf(stderr, "Could not convert '%s' to type '%s'.\n",
                                qPrintable(argument), types.at(i).constData());
                        return 1 ;
                    }
                } else if (id == qMetaTypeId<QDBusVariant>()) {
                    QDBusVariant tmp(p);
                    p = QVariant::fromValue(tmp);
                } else if (id == qMetaTypeId<QDBusObjectPath>()) {
                    QDBusObjectPath path(argument);
                    if (path.path().isNull()) {
                        fprintf(stderr, "Cannot pass argument '%s' because it is not a valid object path.\n",
                                qPrintable(argument));
                        return 1;
                    }
                    p = QVariant::fromValue(path);
                } else if (id == qMetaTypeId<QDBusSignature>()) {
                    QDBusSignature sig(argument);
                    if (sig.signature().isNull()) {
                        fprintf(stderr, "Cannot pass argument '%s' because it is not a valid signature.\n",
                                qPrintable(argument));
                        return 1;
                    }
                    p = QVariant::fromValue(sig);
                } else {
                    fprintf(stderr, "Sorry, can't pass arg of type '%s'.\n",
                            types.at(i).constData());
                    return 1;
                }
                params += p;
            }
            if (params.size() == types.size() && args.isEmpty())
                matchFound = true;
            else if (knownIds.isEmpty()) {
                fprintf(stderr, "Invalid number of parameters\n");
                return 1;
            }
        } // while (!matchFound)
    } // if (!args.isEmpty()

    QDBusMessage reply = iface.callWithArgumentList(QDBus::Block, member, params);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QDBusError err = reply;
        // Failed to retrieve property after falling back?
        // Bail out without displaying an error
        if (!try_prop)
            return 1;
        if (err.type() == QDBusError::UnknownMethod && try_prop) {
            QStringList proparg;
            proparg += interface;
            proparg += member;
            if (!placeCall(service, path, "org.freedesktop.DBus.Properties", "Get", proparg, false))
                return 0;
        }
        if (err.type() == QDBusError::ServiceUnknown)
            fprintf(stderr, "Service '%s' does not exist.\n", qPrintable(service));
        else
            printf("Error: %s\n%s\n", qPrintable(err.name()), qPrintable(err.message()));
        return 2;
    } else if (reply.type() != QDBusMessage::ReplyMessage) {
        fprintf(stderr, "Invalid reply type %d\n", int(reply.type()));
        return 1;
    }

    const QVariantList replyArguments = reply.arguments();
    for (const QVariant &v : replyArguments)
        printArg(v);

    return 0;
}

static bool globServices(QDBusConnectionInterface *bus, const QString &glob)
{
    QRegularExpression pattern(QRegularExpression::wildcardToRegularExpression(glob));
    if (!pattern.isValid())
        return false;

    QStringList names = bus->registeredServiceNames();
    names.sort();
    for (const QString &name : std::as_const(names))
        if (pattern.match(name).hasMatch())
            printf("%s\n", qPrintable(name));

    return true;
}

static void printAllServices(QDBusConnectionInterface *bus)
{
    const QStringList services = bus->registeredServiceNames();
    QMap<QString, QStringList> servicesWithAliases;

    for (const QString &serviceName : services) {
        QDBusReply<QString> reply = bus->serviceOwner(serviceName);
        QString owner = reply;
        if (owner.isEmpty())
            owner = serviceName;
        servicesWithAliases[owner].append(serviceName);
    }

    for (QMap<QString,QStringList>::const_iterator it = servicesWithAliases.constBegin();
         it != servicesWithAliases.constEnd(); ++it) {
        QStringList names = it.value();
        names.sort();
        printf("%s\n", qPrintable(names.join(QLatin1String("\n "))));
    }
}

int main(int argc, char **argv)
{
    QT_PREPEND_NAMESPACE(qt_dbus_metaobject_skip_annotations) = true;
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.takeFirst();

    bool connectionOpened = false;
    while (!args.isEmpty() && args.at(0).startsWith(QLatin1Char('-'))) {
        QString arg = args.takeFirst();
        if (arg == QLatin1String("--system")) {
            connection = QDBusConnection::systemBus();
            connectionOpened = true;
        } else if (arg == QLatin1String("--bus")) {
            if (!args.isEmpty()) {
                connection = QDBusConnection::connectToBus(args.takeFirst(), "QDBus");
                connectionOpened = true;
            }
        } else if (arg == QLatin1String("--literal")) {
            printArgumentsLiterally = true;
        } else if (arg == QLatin1String("--help")) {
            showUsage();
            return 0;
        }
    }

    if (!connectionOpened)
        connection = QDBusConnection::sessionBus();

    if (!connection.isConnected()) {
        const QDBusError lastError = connection.lastError();
        if (lastError.isValid()) {
            fprintf(stderr, "Could not connect to D-Bus server: %s: %s\n",
                    qPrintable(lastError.name()),
                    qPrintable(lastError.message()));
        } else {
            // an invalid last error means that we were not able to even load the D-Bus library
            fprintf(stderr, "Could not connect to D-Bus server: Unable to load dbus libraries\n");
        }
        return 1;
    }

    QDBusConnectionInterface *bus = connection.interface();
    if (args.isEmpty()) {
        printAllServices(bus);
        return 0;
    }

    QString service = args.takeFirst();
    if (!isValidBusName(service)) {
        if (service.contains(QLatin1Char('*'))) {
            if (globServices(bus, service))
                return 0;
        }
        fprintf(stderr, "Service '%s' is not a valid name.\n", qPrintable(service));
        return 1;
    }

    if (args.isEmpty()) {
        listObjects(service, QString());
        return 0;
    }

    QString path = args.takeFirst();
    if (!isValidObjectPath(path)) {
        fprintf(stderr, "Path '%s' is not a valid path name.\n", qPrintable(path));
        return 1;
    }
    if (args.isEmpty()) {
        listAllInterfaces(service, path);
        return 0;
    }

    QString interface = args.takeFirst();
    QString member;
    int pos = interface.lastIndexOf(QLatin1Char('.'));
    if (pos == -1) {
        member = interface;
        interface.clear();
    } else {
        member = interface.mid(pos + 1);
        interface.truncate(pos);
    }
    if (!interface.isEmpty() && !isValidInterfaceName(interface)) {
        fprintf(stderr, "Interface '%s' is not a valid interface name.\n", qPrintable(interface));
        exit(1);
    }
    if (!isValidMemberName(member)) {
        fprintf(stderr, "Method name '%s' is not a valid member name.\n", qPrintable(member));
        return 1;
    }

    int ret = placeCall(service, path, interface, member, args);
    return ret;
}

