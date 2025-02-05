// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

import QtQuick 2.0
import QtTest 1.2

import org.kde.dbusaddons

TestCase {
    name: "DBusInterfaceTest"

    QtObject {
        id: daemon
        property DBusInterface iface: DBusInterface {
            name: busAddress
            path: "/"
            iface: "org.kde.dbusaddons.dbusinterfacetest"
            proxy: daemon
            bus: "session"
        }

        signal dbusBatSignal(string message)
    }

    function resolvingCall(method, signature, args, onSuccess = (reply) => {}) {
        let success = false
        const promise = new Promise((resolve, reject) => { daemon.iface.asyncCall(method, signature, args, resolve, reject) }).then((reply) => {
            success = true
        }).catch((e) => {
            fail(e)
        })
        // Call me old fashioned but I'd really like an await operator -.-
        tryVerify(() => { return success })
        compare(success, true)
    }

    function rejectingCall(method, signature, args) {
        let failed = false
        const promise = new Promise((resolve, reject) => { daemon.iface.asyncCall(method, signature, args, resolve, reject) }).then(() => {
            fail("This should have failed!")
        }).catch((e) => {
            failed = true
        })
        // Call me old fashioned but I'd really like an await operator -.-
        tryVerify(() => { return failed })
        compare(failed, true)
    }

    function test_method1() {
        resolvingCall("method1", "", [])
    }

    function test_method2() {
        resolvingCall("method2", "as", ["a", "b"])
    }

    function test_method3() {
        resolvingCall("method3", "sahta(sv)", ["foo.service", [1], 42, [["foo", 42]]])
    }

    function test_method4() {
        resolvingCall("method4", "(sis)", [['a', 1, 'a']])
    }

    function test_method5() {
        resolvingCall("method5", "aa{sv}aaa(sis)", [[{'a':1, 'b': 2}], [[[['a', 1, 'c']]]]])
    }

    function test_method5_malformed_input() {
        // input lacks a [] level. shouldn't work. but also not crash.
        rejectingCall("method5", "aaa(sis)", [[[['a', 1, 'a']]]])
    }

    function test_method6() {
        resolvingCall("method6", "(sa(sis)sas)", [['a', [['a', 1, 'a']], 'b', ['c', 'd', 'e']]])
    }

    function test_method7() {
        resolvingCall("method7", "a{sv}", [{"a": 1, "b": 2}])
    }

    function test_method7_invalid_signature() {
        rejectingCall("method7", "{sv}", [{"a": 1, "b": 2}])
    }

    function test_complex_types_inside_variantlist() {
        resolvingCall("method8", "av", [
            [
                DBusInterface.byte(1),
                DBusInterface.uint32(2),
                DBusInterface.container("av", [DBusInterface.uint64(3), DBusInterface.int64(4)]),
                DBusInterface.container("as", [5, 6]),
            ]
        ])
    }

    function test_one_reply() {
        resolvingCall("singleReply", "", [], (reply) => {
            compare(reply, 1)
        })
    }

    function test_many_replies() {
        resolvingCall("manyReplies", "", [], (reply) => {
            compare(reply.length, 2)
            compare(reply[0], 1)
            compare(reply[1], 2)
        })
    }

    function test_no_reply() {
        skip("not implemented")
    }

    SignalSpy {
        id: batSignalSpy
        target: daemon
        signalName: "dbusBatSignal"
    }

    function test_signal() {
        resolvingCall("lightTheBatSignal", "s", ["batman"])
        batSignalSpy.wait(1000)
        compare(batSignalSpy.count, 1)
        compare(batSignalSpy.signalArguments[0][0], "batman")
    }
}
