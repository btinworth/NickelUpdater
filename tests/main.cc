#include "SmokeTest.h"
#include <QtTest>

int main(int argc, char** argv)
{
    int status = 0;

    SmokeTest smokeTest;
    status |= QTest::qExec(&smokeTest, argc, argv);

    return status;
}
