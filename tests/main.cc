#include "ConfigParseTest.h"
#include "PluginReleaseTest.h"
#include "SmokeTest.h"
#include <QtTest>

int main(int argc, char** argv)
{
    int status = 0;

    SmokeTest smokeTest;
    status |= QTest::qExec(&smokeTest, argc, argv);

    ConfigParseTest configParseTest;
    status |= QTest::qExec(&configParseTest, argc, argv);

    PluginReleaseTest pluginReleaseTest;
    status |= QTest::qExec(&pluginReleaseTest, argc, argv);

    return status;
}
