#include "remotelogsession.h"

#include <QDir>
#include <QFileInfo>

#include "sshprocesstransport.h"

RemoteLogSession::RemoteLogSession( RemoteLogLaunchRequest request, QString mirrorPath,
                                    QString stderrPath, QString preferredToolPath,
                                    QObject* parent )
    : QObject( parent )
    , request_( std::move( request ) )
    , mirrorPath_( std::move( mirrorPath ) )
    , stderrPath_( std::move( stderrPath ) )
    , preferredToolPath_( std::move( preferredToolPath ) )
    , mirrorFile_( mirrorPath_ )
    , stderrFile_( stderrPath_ )
{
    connect( &process_, &QProcess::started, this, &RemoteLogSession::handleStarted );
    connect( &process_, &QProcess::readyReadStandardOutput, this,
             &RemoteLogSession::handleReadyReadStandardOutput );
    connect( &process_, &QProcess::readyReadStandardError, this,
             &RemoteLogSession::handleReadyReadStandardError );
    connect( &process_,
             QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), this,
             &RemoteLogSession::handleFinished );
    connect( &process_, &QProcess::errorOccurred, this, &RemoteLogSession::handleErrorOccurred );
}

RemoteLogSession::~RemoteLogSession()
{
    stop( false );
}

bool RemoteLogSession::prepareFiles( QString* errorMessage )
{
    const auto ensureParentDir = []( const QString& filePath ) {
        auto parentDir = QFileInfo( filePath ).dir();
        if ( parentDir.exists() ) {
            return true;
        }
        return parentDir.mkpath( "." );
    };

    if ( !ensureParentDir( mirrorPath_ ) || !ensureParentDir( stderrPath_ ) ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = tr( "Failed to create a temporary directory for the remote log." );
        }
        return false;
    }

    if ( !mirrorFile_.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = tr( "Failed to create the local mirror file." );
        }
        return false;
    }
    mirrorFile_.close();
    mirrorFile_.setFileName( mirrorPath_ );

    if ( !stderrFile_.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = tr( "Failed to create the diagnostics file for the remote log." );
        }
        return false;
    }
    stderrFile_.close();
    stderrFile_.setFileName( stderrPath_ );

    if ( !mirrorFile_.open( QIODevice::WriteOnly | QIODevice::Append ) ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = tr( "Failed to open the local mirror file for appending." );
        }
        return false;
    }

    if ( !stderrFile_.open( QIODevice::WriteOnly | QIODevice::Append ) ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = tr( "Failed to open the diagnostics file for appending." );
        }
        return false;
    }

    return true;
}

bool RemoteLogSession::start( QString* errorMessage )
{
    if ( !prepareFiles( errorMessage ) ) {
        return false;
    }

    QString launchError;
    const auto launchSpec
        = SshProcessTransport::buildLaunchSpec( request_, preferredToolPath_, &launchError );
    if ( !launchSpec.isValid() ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = launchError;
        }
        return false;
    }

    process_.setProgram( launchSpec.program );
    process_.setArguments( launchSpec.arguments );
    process_.start();

    if ( !process_.waitForStarted( 5000 ) ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = process_.errorString();
        }
        return false;
    }

    return true;
}

void RemoteLogSession::stop( bool cleanupFilesOnStop )
{
    explicitStopRequested_ = true;
    cleanupRequested_ = cleanupFilesOnStop;

    if ( process_.state() != QProcess::NotRunning ) {
        process_.terminate();
        if ( !process_.waitForFinished( 2000 ) ) {
            process_.kill();
            process_.waitForFinished( 2000 );
        }
    }

    mirrorFile_.close();
    stderrFile_.close();

    if ( cleanupRequested_ ) {
        cleanupFiles();
    }
}

QString RemoteLogSession::statusText() const
{
    switch ( state_ ) {
    case RemoteLogSessionState::Starting:
        return tr( "Remote: connecting to %1" ).arg( request_.sourceLabel() );
    case RemoteLogSessionState::Running:
        return tr( "Remote: connected to %1" ).arg( request_.sourceLabel() );
    case RemoteLogSessionState::Stopped:
        return tr( "Remote: stopped %1" ).arg( request_.sourceLabel() );
    case RemoteLogSessionState::Failed:
        return tr( "Remote: disconnected %1" ).arg( request_.sourceLabel() );
    }

    return {};
}

void RemoteLogSession::handleStarted()
{
    setState( RemoteLogSessionState::Running );
    Q_EMIT started();
}

void RemoteLogSession::handleReadyReadStandardOutput()
{
    const auto payload = process_.readAllStandardOutput();
    if ( !payload.isEmpty() ) {
        mirrorFile_.write( payload );
        mirrorFile_.flush();
    }
}

void RemoteLogSession::appendDiagnostics( const QString& text )
{
    if ( text.isEmpty() ) {
        return;
    }

    const auto payload = text.toUtf8();
    stderrFile_.write( payload );
    stderrFile_.flush();
    diagnostics_ += text;
    Q_EMIT warningsChanged( diagnostics_ );
}

void RemoteLogSession::handleReadyReadStandardError()
{
    appendDiagnostics( QString::fromUtf8( process_.readAllStandardError() ) );
}

void RemoteLogSession::handleFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    handleReadyReadStandardOutput();
    handleReadyReadStandardError();

    mirrorFile_.flush();
    stderrFile_.flush();

    if ( explicitStopRequested_ ) {
        setState( RemoteLogSessionState::Stopped );
        if ( cleanupRequested_ ) {
            mirrorFile_.close();
            stderrFile_.close();
            cleanupFiles();
        }
    }
    else if ( exitStatus == QProcess::NormalExit && exitCode == 0 ) {
        setState( RemoteLogSessionState::Stopped );
    }
    else {
        appendDiagnostics( tr( "\nProcess exited with code %1.\n" ).arg( exitCode ) );
        setState( RemoteLogSessionState::Failed );
    }

    Q_EMIT finished();
}

void RemoteLogSession::handleErrorOccurred( QProcess::ProcessError error )
{
    if ( explicitStopRequested_ && error == QProcess::Crashed ) {
        return;
    }

    appendDiagnostics( tr( "\nSSH process error: %1\n" ).arg( process_.errorString() ) );
    if ( state_ == RemoteLogSessionState::Starting || state_ == RemoteLogSessionState::Running ) {
        setState( RemoteLogSessionState::Failed );
    }
}

void RemoteLogSession::setState( RemoteLogSessionState state )
{
    if ( state_ == state ) {
        return;
    }

    state_ = state;
    Q_EMIT stateChanged( state_ );
}

void RemoteLogSession::cleanupFiles()
{
    QFile::remove( stderrPath_ );
    QFile::remove( mirrorPath_ );
}
