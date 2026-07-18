#include "ConfigParseTest.h"
#include "RuntimeUpdateTest.h"
#include "SmokeTest.h"
#include <QtTest>

int main(int argc, char** argv)
{
    int status = 0;

    SmokeTest smokeTest;
    status |= QTest::qExec(&smokeTest, argc, argv);

    ConfigParseTest configParseTest;
    status |= QTest::qExec(&configParseTest, argc, argv);

    RuntimeUpdateTest runtimeUpdateTest;
    status |= QTest::qExec(&runtimeUpdateTest, argc, argv);

    return status;
}
