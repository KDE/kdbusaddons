// SPDX-License-Identifier: GPL-3.0-only WITH Qt-GPL-exception-1.0
// SPDX-FileCopyrightText: Copyright (C) 2021 The Qt Company Ltd.
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include <qbytearray.h>
#include <qcommandlineparser.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qloggingcategory.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qset.h>

#include <qdbusmetatype.h>
#include <QtDBus/private/qdbusintrospection_p.h>

#include <cstdio>
#include <cstdlib>

#define PROGRAMNAME     "kdbusxml2qml"
#define PROGRAMVERSION  "1.0"
#define PROGRAMCOPYRIGHT "KDE"

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"

using namespace Qt::StringLiterals;

class QDBusXmlToCpp final
{
public:
    int run(const QCoreApplication &app);

private:
    // This is required by the QDBusIntrospection parser!
    class DiagnosticsReporter final : public QDBusIntrospection::DiagnosticsReporter
    {
    public:
        void setFileName(const QString &fileName) { m_fileName = fileName; }
        bool hadErrors() const { return m_hadErrors; }

        void warning(const QDBusIntrospection::SourceLocation &location, const char *msg,
                     ...) override;
        void error(const QDBusIntrospection::SourceLocation &location, const char *msg,
                   ...) override;
        void note(const QDBusIntrospection::SourceLocation &location, const char *msg, ...)
                Q_ATTRIBUTE_FORMAT_PRINTF(3, 4);

    private:
        QString m_fileName;
        bool m_hadErrors = false;

        void report(const QDBusIntrospection::SourceLocation &location, const char *msg, va_list ap,
                    const char *severity);
    };

    void writeAdaptor(const QString &filename, const QDBusIntrospection::Interfaces &interfaces);
    void writeProxy(const QString &filename, const QDBusIntrospection::Interfaces &interfaces);

    QDBusIntrospection::Interfaces readInput();
    void cleanInterfaces(QDBusIntrospection::Interfaces &interfaces);
    QTextStream &writeHeader(QTextStream &ts, bool changesWillBeLost);
    QByteArray qtTypeName(const QString &signature);
    void
    writeArgList(QTextStream &ts, const QStringList &argNames,
                 const QDBusIntrospection::Annotations &annotations,
                 const QDBusIntrospection::Arguments &inputArgs,
                 const QDBusIntrospection::Arguments &outputArgs = QDBusIntrospection::Arguments());
    void writeSignalArgList(QTextStream &ts, const QStringList &argNames,
                            const QDBusIntrospection::Annotations &annotations,
                            const QDBusIntrospection::Arguments &outputArgs);
    QString propertyGetter(const QDBusIntrospection::Property &property);
    QString propertySetter(const QDBusIntrospection::Property &property);

    QString globalClassName;
    QString parentClassName;
    QString inputFile;
    bool skipNamespaces = false;
    bool includeMocs = false;
    QString commandLine;
    QStringList includes;
    QStringList globalIncludes;
    QStringList wantedInterfaces;

    DiagnosticsReporter reporter;
};

void QDBusXmlToCpp::DiagnosticsReporter::warning(const QDBusIntrospection::SourceLocation &location,
                                                 const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    report(location, msg, ap, "warning");
    va_end(ap);
}

void QDBusXmlToCpp::DiagnosticsReporter::error(const QDBusIntrospection::SourceLocation &location,
                                               const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    report(location, msg, ap, "error");
    va_end(ap);
    m_hadErrors = true;
}

void QDBusXmlToCpp::DiagnosticsReporter::note(const QDBusIntrospection::SourceLocation &location,
                                              const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    report(location, msg, ap, "note");
    va_end(ap);
    m_hadErrors = true;
}

void QDBusXmlToCpp::DiagnosticsReporter::report(const QDBusIntrospection::SourceLocation &location,
                                                const char *msg, va_list ap, const char *severity)
{
    fprintf(stderr, "%s:%lld:%lld: %s: ", qPrintable(m_fileName),
            (long long int)location.lineNumber, (long long int)location.columnNumber + 1, severity);
    vfprintf(stderr, msg, ap);
}

QDBusIntrospection::Interfaces QDBusXmlToCpp::readInput()
{
    QFile input(inputFile);
    if (inputFile.isEmpty() || inputFile == "-"_L1) {
        reporter.setFileName("<standard input>"_L1);
        if (!input.open(stdin, QIODevice::ReadOnly)) {
            fprintf(stderr, PROGRAMNAME ": could not open standard input: %s\n",
                    qPrintable(input.errorString()));
            exit(1);
        }
    } else {
        reporter.setFileName(inputFile);
        if (!input.open(QIODevice::ReadOnly)) {
            fprintf(stderr, PROGRAMNAME ": could not open input file '%s': %s\n",
                    qPrintable(inputFile), qPrintable(input.errorString()));
            exit(1);
        }
    }

    QByteArray data = input.readAll();
    auto interfaces = QDBusIntrospection::parseInterfaces(QString::fromUtf8(data), &reporter);
    if (reporter.hadErrors())
        exit(1);

    return interfaces;
}

void QDBusXmlToCpp::cleanInterfaces(QDBusIntrospection::Interfaces &interfaces)
{
    if (!wantedInterfaces.isEmpty()) {
        QDBusIntrospection::Interfaces::Iterator it = interfaces.begin();
        while (it != interfaces.end())
            if (!wantedInterfaces.contains(it.key()))
                it = interfaces.erase(it);
            else
                ++it;
    }
}

static bool isSupportedSuffix(QStringView suffix)
{
    static auto candidates = {
        "qml"_L1,
    };

    for (auto candidate : candidates)
        if (suffix == candidate)
            return true;

    return false;
}

// produce a header name from the file name
static QString header(const QString &name)
{
    QStringList parts = name.split(u':');
    QString retval = parts.front();

    if (retval.isEmpty() || retval == "-"_L1)
        return retval;

    QFileInfo header{retval};
    if (!isSupportedSuffix(header.suffix()))
        retval.append(".qml"_L1);

    return retval;
}

QTextStream &QDBusXmlToCpp::writeHeader(QTextStream &ts, bool changesWillBeLost)
{
    ts << "/*\n"
          " * This file was generated by " PROGRAMNAME " version " PROGRAMVERSION "\n"
          " * Source file was " << QFileInfo(inputFile).fileName() << "\n"
          " *\n"
          " * " PROGRAMNAME " is " PROGRAMCOPYRIGHT "\n"
          " *\n"
          " * This is an auto-generated file.\n";

    if (changesWillBeLost)
        ts << " * Do not edit! All changes made to it will be lost.\n";
    else
        ts << " * This file may have been hand-edited. Look for HAND-EDIT comments\n"
              " * before re-generating it.\n";

    ts << " */\n\n";

    return ts;
}

QByteArray QDBusXmlToCpp::qtTypeName(const QString &signature)
{
    auto type = QDBusMetaType::signatureToMetaType(qPrintable(signature));
    switch (QMetaType::Type(type.id())) {
    case QMetaType::Bool:
        return "bool"_ba;
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Long:
    case QMetaType::Short:
    case QMetaType::Char:
    case QMetaType::Char16:
    case QMetaType::Char32:
    case QMetaType::ULong:
    case QMetaType::UShort:
    case QMetaType::UChar:
    case QMetaType::SChar:
        return "int"_ba;
    case QMetaType::Float:
        return "real"_ba;
    case QMetaType::Double:
        return "double"_ba;
    case QMetaType::QChar:
    case QMetaType::QString:
        return "string"_ba;
    case QMetaType::QVariantList:
        return "list<var>"_ba;
    case QMetaType::QByteArrayList:
    case QMetaType::QStringList:
        return "list<string>"_ba;
    case QMetaType::Nullptr:
    case QMetaType::QCborSimpleType:
    case QMetaType::Void:
    case QMetaType::VoidStar:
    case QMetaType::QByteArray:
    case QMetaType::QBitArray:
    case QMetaType::QDate:
    case QMetaType::QTime:
    case QMetaType::QDateTime:
    case QMetaType::QUrl:
    case QMetaType::QLocale:
    case QMetaType::QRect:
    case QMetaType::QRectF:
    case QMetaType::QSize:
    case QMetaType::QSizeF:
    case QMetaType::QLine:
    case QMetaType::QLineF:
    case QMetaType::QPoint:
    case QMetaType::QPointF:
    case QMetaType::QEasingCurve:
    case QMetaType::QUuid:
    case QMetaType::QVariant:
    case QMetaType::QRegularExpression:
    case QMetaType::QJsonValue:
    case QMetaType::QJsonObject:
    case QMetaType::QJsonArray:
    case QMetaType::QJsonDocument:
    case QMetaType::QCborValue:
    case QMetaType::QCborArray:
    case QMetaType::QCborMap:
    case QMetaType::Float16:
    case QMetaType::QModelIndex:
    case QMetaType::QPersistentModelIndex:
    case QMetaType::QObjectStar:
    case QMetaType::QVariantMap:
    case QMetaType::QVariantHash:
    case QMetaType::QVariantPair:
    case QMetaType::QFont:
    case QMetaType::QPixmap:
    case QMetaType::QBrush:
    case QMetaType::QColor:
    case QMetaType::QPalette:
    case QMetaType::QIcon:
    case QMetaType::QImage:
    case QMetaType::QPolygon:
    case QMetaType::QRegion:
    case QMetaType::QBitmap:
    case QMetaType::QCursor:
    case QMetaType::QKeySequence:
    case QMetaType::QPen:
    case QMetaType::QTextLength:
    case QMetaType::QTextFormat:
    case QMetaType::QTransform:
    case QMetaType::QMatrix4x4:
    case QMetaType::QVector2D:
    case QMetaType::QVector3D:
    case QMetaType::QVector4D:
    case QMetaType::QQuaternion:
    case QMetaType::QPolygonF:
    case QMetaType::QColorSpace:
    case QMetaType::QSizePolicy:
    case QMetaType::UnknownType:
    case QMetaType::User:
        break;
    }
    return "var"_ba;
}

static QString methodName(const QDBusIntrospection::Method &method)
{
    QString name = method.annotations.value(u"org.qtproject.QtDBus.MethodName"_s).value;
    if (!name.isEmpty())
        return name;

    return method.name;
}

static bool openFile(const QString &fileName, QFile &file)
{
    if (fileName.isEmpty())
        return false;

    bool isOk = false;
    if (fileName == "-"_L1) {
        isOk = file.open(stdout, QIODevice::WriteOnly | QIODevice::Text);
    } else {
        file.setFileName(fileName);
        isOk = file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    }

    if (!isOk)
        fprintf(stderr, "%s: Unable to open '%s': %s\n",
                PROGRAMNAME, qPrintable(fileName), qPrintable(file.errorString()));
    return isOk;
}

void QDBusXmlToCpp::writeProxy(const QString &filename,
                               const QDBusIntrospection::Interfaces &interfaces)
{
    // open the file
    QString headerName = header(filename);
    QByteArray headerData;
    QTextStream hs(&headerData);

    writeHeader(hs, true);

    for (const QDBusIntrospection::Interface *interface : interfaces) {
        hs << "import QtQuick\n"
           << "import org.kde.dbusaddons\n\n";

        hs << "QtObject {\n"
           << "    required property DBusInterface iface\n"
           << "    readonly property string interfaceName: \"" << interface->name << "\"\n";

        hs << "    readonly property var propertySignatures: {\n";
        for (const QDBusIntrospection::Property &property : interface->properties) {
            hs << "        \"" << property.name << "\": \"" << property.type << "\",\n";
        }
        hs << "    }\n";

        hs << "\n";

        for (const QDBusIntrospection::Property &property : interface->properties) {
            hs << "    property " << qtTypeName(property.type) << " dbus" << property.name << "\n";
        }

        hs << "\n";

        for (const QDBusIntrospection::Signal &signal : interface->signals_) {
            bool isDeprecated = signal.annotations.value("org.freedesktop.DBus.Deprecated"_L1).value == "true"_L1;

            if (isDeprecated) {
                hs << "    @Deprecated {}\n";
            }

            hs << "    signal dbus" << signal.name << "(";

            QStringList args;
            for (const auto &arg : signal.outputArgs) {
                args << QString(arg.name) + ": "_L1 + QString::fromLatin1(qtTypeName(arg.type));
            }
            hs << args.join(", "_L1);

            hs << ")\n"; // finished for header
        }

        hs << "\n";

        for (const QDBusIntrospection::Method &method : interface->methods) {
            bool isDeprecated = method.annotations.value("org.freedesktop.DBus.Deprecated"_L1).value == "true"_L1;
            bool isNoReply = method.annotations.value(ANNOTATION_NO_WAIT ""_L1).value == "true"_L1;
            if (isNoReply && !method.outputArgs.isEmpty()) {
                reporter.warning(method.location,
                                 "method %s in interface %s is marked 'no-reply' but has output "
                                 "arguments.\n",
                                 qPrintable(method.name),
                                 qPrintable(interface->name));
                continue;
            }

            if (isDeprecated) {
                hs << "    @Deprecated {}\n";
            }

            hs << "    function dbus" << methodName(method) << "Async(";

            // args with annotation
            QString signature;
            QStringList args;
            qsizetype index = 0;
            for (const auto &arg : method.inputArgs) {
                signature += arg.type;
                args << QString(!arg.name.isEmpty() ? arg.name : "var%1"_L1.arg(QString::number(index))) + ": "_L1 + QString::fromLatin1(qtTypeName(arg.type));
                ++index;
            }
            hs << args.join(", "_L1);

            // The return value is always var since this is a promise
            hs << "): var {\n";
            hs << "        return new Promise((resolve, reject) => { iface.asyncCall(\"" << methodName(method) << "\", \"" << signature
               << "\", [...arguments], resolve, reject) })\n";
            hs << "    }\n";


            hs << "    function dbus" << methodName(method) << "Sync(";
            hs << args.join(", "_L1);
            hs << ")";

            // output annotation
            if (!method.outputArgs.isEmpty()) {
                hs << ": ";
                if (method.outputArgs.size() > 1)
                    hs << "list<var>";
                else
                    hs << qtTypeName(method.outputArgs.first().type);
            }

            hs << " {\n";
            hs << "        return iface.syncCall(\"" << methodName(method) << "\", \"" << signature << "\", [...arguments])\n";
            hs << "    }\n";
        }

        hs << "}\n\n";
    }

    hs.flush();

    QFile file;
    const bool headerOpen = openFile(headerName, file);
    if (headerOpen)
        file.write(headerData);
}

int QDBusXmlToCpp::run(const QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
            "Produces the C++ code to implement the interfaces defined in the input file.\n\n"
            "If the file name given to the options -a and -p does not end in .cpp or .h, the\n"
            "program will automatically append the suffixes and produce both files.\n"
            "You can also use a colon (:) to separate the header name from the source file\n"
            "name, as in '-a filename_p.h:filename.cpp'.\n\n"
            "If you pass a dash (-) as the argument to either -p or -a, the output is written\n"
            "to the standard output."_L1);

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(u"xml-or-xml-file"_s, u"XML file to use."_s);
    parser.addPositionalArgument(u"interfaces"_s, u"List of interfaces to use."_s,
                u"[interfaces ...]"_s);

    QCommandLineOption adapterCodeOption(QStringList{u"a"_s, u"adaptor"_s},
                u"Write the adaptor code to <filename>"_s, u"filename"_s);
    parser.addOption(adapterCodeOption);

    QCommandLineOption classNameOption(QStringList{u"c"_s, u"classname"_s},
                u"Use <classname> as the class name for the generated classes. "
                u"This option can only be used when processing a single interface."_s,
                u"classname"_s);
    parser.addOption(classNameOption);

    QCommandLineOption addIncludeOption(QStringList{u"i"_s, u"include"_s},
                u"Add #include \"filename\" to the output"_s, u"filename"_s);
    parser.addOption(addIncludeOption);

    QCommandLineOption addGlobalIncludeOption(QStringList{u"I"_s, u"global-include"_s},
                u"Add #include <filename> to the output"_s, u"filename"_s);
    parser.addOption(addGlobalIncludeOption);

    QCommandLineOption adapterParentOption(u"l"_s,
                u"When generating an adaptor, use <classname> as the parent class"_s, u"classname"_s);
    parser.addOption(adapterParentOption);

    QCommandLineOption mocIncludeOption(QStringList{u"m"_s, u"moc"_s},
                u"Generate #include \"filename.moc\" statements in the .cpp files"_s);
    parser.addOption(mocIncludeOption);

    QCommandLineOption noNamespaceOption(QStringList{u"N"_s, u"no-namespaces"_s},
                u"Don't use namespaces"_s);
    parser.addOption(noNamespaceOption);

    QCommandLineOption proxyCodeOption(QStringList{u"p"_s, u"proxy"_s},
                u"Write the proxy code to <filename>"_s, u"filename"_s);
    parser.addOption(proxyCodeOption);

    QCommandLineOption verboseOption(QStringList{u"V"_s, u"verbose"_s},
                u"Be verbose."_s);
    parser.addOption(verboseOption);

    parser.process(app);

    QString adaptorFile = parser.value(adapterCodeOption);
    globalClassName = parser.value(classNameOption);
    includes = parser.values(addIncludeOption);
    globalIncludes = parser.values(addGlobalIncludeOption);
    parentClassName = parser.value(adapterParentOption);
    includeMocs = parser.isSet(mocIncludeOption);
    skipNamespaces = parser.isSet(noNamespaceOption);
    QString proxyFile = parser.value(proxyCodeOption);
    bool verbose = parser.isSet(verboseOption);

    wantedInterfaces = parser.positionalArguments();
    if (!wantedInterfaces.isEmpty()) {
        inputFile = wantedInterfaces.takeFirst();

        QFileInfo inputInfo(inputFile);
        if (!inputInfo.exists() || !inputInfo.isFile() || !inputInfo.isReadable()) {
            qCritical("Error: Input %s is not a file or cannot be accessed\n", qPrintable(inputFile));
            return 1;
        }
    }

    if (verbose)
        QLoggingCategory::setFilterRules(u"dbus.parser.debug=true"_s);

    QDBusIntrospection::Interfaces interfaces = readInput();
    cleanInterfaces(interfaces);

    if (!globalClassName.isEmpty() && interfaces.count() != 1) {
        qCritical("Option -c/--classname can only be used with a single interface.\n");
        return 1;
    }

    QStringList args = app.arguments();
    args.removeFirst();
    commandLine = PROGRAMNAME " "_L1 + args.join(u' ');

    if (!proxyFile.isEmpty() || adaptorFile.isEmpty())
        writeProxy(proxyFile, interfaces);

    return 0;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral(PROGRAMNAME));
    QCoreApplication::setApplicationVersion(QStringLiteral(PROGRAMVERSION));

    return QDBusXmlToCpp().run(app);
}
