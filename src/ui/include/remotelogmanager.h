#ifndef KLOGG_REMOTELOGMANAGER_H
#define KLOGG_REMOTELOGMANAGER_H

#include <QHash>
#include <QObject>

#include "remotelogtypes.h"

class RemoteLogSession;

class RemoteLogManager : public QObject {
    Q_OBJECT

  public:
    explicit RemoteLogManager( QString rootTempPath, QObject* parent = nullptr );

    RemoteLogSession* createSession( const RemoteLogLaunchRequest& request,
                                     const QString& preferredToolPath, QString* errorMessage );

    [[nodiscard]] RemoteLogSession* sessionForMirrorPath( const QString& mirrorPath ) const;
    [[nodiscard]] bool isManagedMirrorPath( const QString& mirrorPath ) const;

    void closeSession( const QString& mirrorPath, bool cleanupFiles );
    void closeAllSessions();

  private:
    QString makeFileBaseName( const RemoteLogProfile& profile ) const;

    QString rootTempPath_;
    QHash<QString, RemoteLogSession*> sessionsByMirrorPath_;
};

#endif
