# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

add_executable(kdbusxml2qml main.cpp)
target_link_libraries(kdbusxml2qml Qt::DBusPrivate)
