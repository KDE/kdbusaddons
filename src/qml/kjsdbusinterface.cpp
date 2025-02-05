// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include "kjsdbusinterface.h"

#include <QJSEngine>
#include <QJSValue>

#include "dbusinterface.h"

using namespace Qt::StringLiterals;

void KJSDBusInterface::registerMetaObject(QJSEngine &engine, QJSValue object, const QString &name)
{
    QJSValue jsMetaObject = engine.newQMetaObject(&DBusInterface::staticMetaObject);
    object.setProperty(name, jsMetaObject);
}
