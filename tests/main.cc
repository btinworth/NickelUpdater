#include "ConfigParseTest.h"
#include "PluginReleaseTest.h"
#include <QCoreApplication>
#include <QtTest>

int main(int argc, char** argv)
{
    // QNetworkAccessManager (used via Utilities::HttpGet) requires a QCoreApplication instance to
    // be alive for its event dispatcher; without it, requests silently fail and the nested
    // QEventLoop used to wait for them blocks forever.
    QCoreApplication app(argc, argv);

    int status = 0;

    ConfigParseTest configParseTest;
    status |= QTest::qExec(&configParseTest, argc, argv);

    PluginReleaseTest pluginReleaseTest;
    status |= QTest::qExec(&pluginReleaseTest, argc, argv);

    return status;
}

