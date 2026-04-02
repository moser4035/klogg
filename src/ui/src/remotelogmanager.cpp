#include "remotelogmanager.h"

#include <QDir>
#include <QUuid>

#include "remotelogsession.h"

namespace {
QString sanitizedName( QString value )
{
    if ( value.isEmpty() ) {
        value = "remote-log";
    }

    for ( auto& ch : value ) {
        if ( !ch.isLetterOrNumber() ) {
            ch = '_';
        }
    }

    return value.left( 40 );
}
} // namespace

RemoteLogManager::RemoteLogManager( QString rootTempPath, QObject* parent )
    : QObject( parent )
    , rootTempPath_( std::move( rootTempPath ) )
{
}

RemoteLogSession* RemoteLogManager::createSession( const RemoteLogLaunchRequest& request,
                                                   const QString& preferredToolPath,
                                                   QString* errorMessage )
{
    QDir rootDir( rootTempPath_ );
    if ( !rootDir.exists() ) {
        rootDir.mkpath( "." );
    }

    auto remoteDirPath = rootDir.filePath( "remote_logs" );
    QDir remoteDir( remoteDirPath );
    if ( !remoteDir.exists() ) {
        remoteDir.mkpath( "." );
    }

    const auto baseName = makeFileBaseName( request );
    const auto mirrorPath = remoteDir.filePath( baseName + ".log" );
    const auto stderrPath = remoteDir.filePath( baseName + ".stderr.log" );

    auto* session
        = new RemoteLogSession( request, mirrorPath, stderrPath, preferredToolPath, this );
    if ( !session->start( errorMessage ) ) {
        session->stop( true );
        session->deleteLater();
        return nullptr;
    }

    sessionsByMirrorPath_.insert( mirrorPath, session );
    return session;
}

RemoteLogSession* RemoteLogManager::sessionForMirrorPath( const QString& mirrorPath ) const
{
    return sessionsByMirrorPath_.value( mirrorPath, nullptr );
}

bool RemoteLogManager::isManagedMirrorPath( const QString& mirrorPath ) const
{
    return sessionsByMirrorPath_.contains( mirrorPath );
}

void RemoteLogManager::closeSession( const QString& mirrorPath, bool cleanupFiles )
{
    auto* session = sessionsByMirrorPath_.take( mirrorPath );
    if ( session == nullptr ) {
        return;
    }

    session->stop( cleanupFiles );
    session->deleteLater();
}

void RemoteLogManager::closeAllSessions()
{
    const auto mirrorPaths = sessionsByMirrorPath_.keys();
    for ( const auto& mirrorPath : mirrorPaths ) {
        closeSession( mirrorPath, true );
    }
}

QString RemoteLogManager::makeFileBaseName( const RemoteLogProfile& profile ) const
{
    const auto readableName = sanitizedName( profile.title() );
    const auto suffix = QUuid::createUuid().toString( QUuid::WithoutBraces );
    return readableName + "_" + suffix;
}
