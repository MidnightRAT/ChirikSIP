#include <QtTest/QtTest>
#include "mainwindow.h"

class TestParsing : public QObject
{
    Q_OBJECT

private slots:
    void testParseNumber();
    void testParseDisplayName();
    void testFormatDuration();
};

void TestParsing::testParseNumber()
{
    // Plain number
    QCOMPARE(MainWindow::parseNumber("12345"), QString("12345"));

    // SIP URI with number@host
    QCOMPARE(MainWindow::parseNumber("sip:12345@server.example.com"), QString("12345"));

    // SIP URI with display name — extracts just the number
    QCOMPARE(MainWindow::parseNumber("\"John Doe\" <sip:12345@server.example.com>"), QString("12345"));

    // SIP URI without @
    QCOMPARE(MainWindow::parseNumber("sip:12345"), QString("12345"));

    // Empty string
    QCOMPARE(MainWindow::parseNumber(""), QString(""));

    // International format
    QCOMPARE(MainWindow::parseNumber("sip:+380501234567@server.example.com"), QString("+380501234567"));
}

void TestParsing::testParseDisplayName()
{
    // Standard display name
    QCOMPARE(MainWindow::parseDisplayName("\"John Doe\" <sip:12345@server>"), QString("John Doe"));

    // No display name
    QCOMPARE(MainWindow::parseDisplayName("sip:12345@server"), QString());

    // Empty string
    QCOMPARE(MainWindow::parseDisplayName(""), QString());

    // Single character name
    QCOMPARE(MainWindow::parseDisplayName("\"A\" <sip:1@server>"), QString("A"));

    // Name with special characters
    QCOMPARE(MainWindow::parseDisplayName("\"O'Brien & Co.\" <sip:1@server>"), QString("O'Brien & Co."));
}

void TestParsing::testFormatDuration()
{
    QCOMPARE(MainWindow::formatDuration(0), QString("00:00:00"));
    QCOMPARE(MainWindow::formatDuration(1), QString("00:00:01"));
    QCOMPARE(MainWindow::formatDuration(59), QString("00:00:59"));
    QCOMPARE(MainWindow::formatDuration(60), QString("00:01:00"));
    QCOMPARE(MainWindow::formatDuration(3600), QString("01:00:00"));
    QCOMPARE(MainWindow::formatDuration(3661), QString("01:01:01"));
    QCOMPARE(MainWindow::formatDuration(86399), QString("23:59:59"));
    QCOMPARE(MainWindow::formatDuration(125), QString("00:02:05"));
}

QTEST_MAIN(TestParsing)
#include "test_parsing.moc"
