#include "remotelogsession.h"

#include <QDir>
#include <QFileInfo>

#include "sshprocesstransport.h"

namespace {
QString normalizeUiMessage( QString message )
{
    message.replace( '\n', ' ' );
    return message.simplified();
}

QString summarizeRemoteIssue( const QString& text )
{
    const auto simplified = normalizeUiMessage( text );
    const auto lowered = simplified.toLower();

    if ( lowered.contains( "permission denied" )
         || lowered.contains( "authentication failed" ) ) {
        return RemoteLogSession::tr( "Authentication failed." );
    }

    if ( lowered.contains( "host key verification failed" ) ) {
        return RemoteLogSession::tr( "Host key verification failed." );
    }

    if ( lowered.contains( "could not resolve hostname" )
         || lowered.contains( "name or service not known" )
         || lowered.contains( "no such host is known" ) ) {
        return RemoteLogSession::tr( "Host could not be resolved." );
    }

    if ( lowered.contains( "connection refused" ) ) {
        return RemoteLogSession::tr( "Connection was refused." );
    }

    if ( lowered.contains( "connection timed out" )
         || lowered.contains( "operation timed out" ) ) {
        return RemoteLogSession::tr( "Connection timed out." );
    }

    if ( lowered.contains( "no route to host" )
         || lowered.contains( "host is unreachable" ) ) {
        return RemoteLogSession::tr( "Host is unreachable." );
    }

    if ( lowered.contains( "connection closed" )
         || lowered.contains( "closed by remote host" ) ) {
        return RemoteLogSession::tr( "Connection was closed by the remote host." );
    }

    if ( lowered.contains( "command not found" )
         || lowered.contains( "tail:" ) ) {
        return RemoteLogSession::tr( "Remote log command failed." );
    }

    if ( lowered.contains( "process exited with code" ) ) {
        return RemoteLogSession::tr( "Remote log process exited unexpectedly." );
    }

    return simplified;
}
} // namespace

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
    stopTimer_.setSingleShot( true );
    connect( &stopTimer_, &QTimer::timeout, this, &RemoteLogSession::handleStopTimeout );
}

RemoteLogSession::~RemoteLogSession()
{
    stopTimer_.stop();

    if ( process_.state() != QProcess::NotRunning ) {
        process_.kill();
        process_.waitForFinished( 100 );
    }

    mirrorFile_.close();
    stderrFile_.close();
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
    cleanupRequested_ = cleanupRequested_ || cleanupFilesOnStop;

    if ( stopFinalized_ ) {
        return;
    }

    if ( process_.state() == QProcess::NotRunning ) {
        setState( RemoteLogSessionState::Stopped );
        finalizeExplicitStop();
        return;
    }

    process_.terminate();
    stopTimer_.start( 250 );
}

bool RemoteLogSession::failedDuringInitialConnect() const
{
    if ( state_ != RemoteLogSessionState::Failed || explicitStopRequested_ ) {
        return false;
    }

    if ( hasDeliveredRemoteData_ ) {
        return false;
    }

    return !runningTimer_.isValid() || runningTimer_.elapsed() < 5000;
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
        if ( uiMessage_.isEmpty() ) {
            return tr( "Remote: disconnected %1" ).arg( request_.sourceLabel() );
        }
        return tr( "Remote: disconnected %1 (%2)" ).arg( request_.sourceLabel(), uiMessage_ );
    }

    return {};
}

void RemoteLogSession::handleStarted()
{
    runningTimer_.start();
    Q_EMIT started();
}

void RemoteLogSession::handleReadyReadStandardOutput()
{
    const auto payload = process_.readAllStandardOutput();
    if ( !payload.isEmpty() ) {
        if ( state_ == RemoteLogSessionState::Starting ) {
            setState( RemoteLogSessionState::Running );
        }
        hasDeliveredRemoteData_ = true;
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
    const auto message = summarizeRemoteIssue( text );
    if ( !message.isEmpty() ) {
        setUiMessage( message );
    }
}

void RemoteLogSession::handleReadyReadStandardError()
{
    appendDiagnostics( QString::fromUtf8( process_.readAllStandardError() ) );
}

void RemoteLogSession::handleFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    handleReadyReadStandardOutput();
    handleReadyReadStandardError();

    if ( mirrorFile_.isOpen() ) {
        mirrorFile_.flush();
    }
    if ( stderrFile_.isOpen() ) {
        stderrFile_.flush();
    }

    if ( explicitStopRequested_ ) {
        setState( RemoteLogSessionState::Stopped );
        finalizeExplicitStop();
    }
    else if ( exitStatus == QProcess::NormalExit && exitCode == 0 ) {
        setState( RemoteLogSessionState::Stopped );
    }
    else {
        appendDiagnostics( tr( "\nProcess exited with code %1.\n" ).arg( exitCode ) );
        if ( uiMessage_.isEmpty() ) {
            setUiMessage( tr( "Remote log process exited with code %1." ).arg( exitCode ) );
        }
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

void RemoteLogSession::handleStopTimeout()
{
    if ( process_.state() != QProcess::NotRunning ) {
        process_.kill();
    }
}

void RemoteLogSession::setUiMessage( const QString& message )
{
    const auto normalized = summarizeRemoteIssue( message );
    if ( normalized.isEmpty() || uiMessage_ == normalized ) {
        return;
    }

    uiMessage_ = normalized;
    Q_EMIT warningsChanged( uiMessage_ );
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

void RemoteLogSession::finalizeExplicitStop()
{
    if ( stopFinalized_ ) {
        return;
    }

    stopFinalized_ = true;
    stopTimer_.stop();
    mirrorFile_.close();
    stderrFile_.close();

    if ( cleanupRequested_ ) {
        cleanupFiles();
    }

    Q_EMIT finished();
}
