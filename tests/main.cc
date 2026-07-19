#include "ConfigParseTest.h"
#include "GitHubInterfaceTest.h"
#include "PluginReleaseTest.h"
#include "RuntimeUpdateTest.h"
#include <QtTest>

int main(int argc, char** argv)
{
    int status = 0;

    ConfigParseTest configParseTest;
    status |= QTest::qExec(&configParseTest, argc, argv);

    PluginReleaseTest pluginReleaseTest;
    status |= QTest::qExec(&pluginReleaseTest, argc, argv);

    GitHubInterfaceTest gitHubInterfaceTest;
    status |= QTest::qExec(&gitHubInterfaceTest, argc, argv);

    RuntimeUpdateTest runtimeUpdateTest;
    status |= QTest::qExec(&runtimeUpdateTest, argc, argv);

    return status;
}
