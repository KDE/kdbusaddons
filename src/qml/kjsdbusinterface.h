// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <kdbusaddons_export.h>

#include <QObject>

class QJSEngine;
class QJSValue;

namespace KJSDBusInterface
{
KDBUSADDONS_EXPORT void registerMetaObject(QJSEngine &engine, QJSValue object, const QString &name);
} // namespace KJSDBusInterface
