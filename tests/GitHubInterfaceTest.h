#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class GitHubInterfaceTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void returnsInvalidWhenWgetFails();
    void returnsInvalidWhenResponseIsNotJson();
    void returnsInvalidWhenTagNameMissing();
    void returnsInvalidWhenTagNameEmpty();
    void returnsInvalidWhenAssetsArrayMissing();
    void returnsInvalidWhenNoKoboRootAsset();
    void returnsValidReleaseForWellFormedResponse();
    void selectsKoboRootUrlWhenMultipleAssets();
    void appendsCommitHashToTagNameWhenResolvable();
    void returnsInvalidWhenCommitHashCannotBeResolved();

private:
    void WriteFakeWgetScript(int exitCode, const QByteArray& response,
                             int commitExitCode = 1, const QByteArray& commitResponse = {}) const;

    QString TempRoot;
    QString BinDir;
    QByteArray OriginalPath;
};
