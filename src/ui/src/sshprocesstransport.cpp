#include "sshprocesstransport.h"

#include <QFileInfo>
#include <QObject>
#include <QStandardPaths>

namespace {
QString shellQuote( const QString& value )
{
    QString escaped = value;
    escaped.replace( "'", "'\"'\"'" );
    return "'" + escaped + "'";
}

QString remoteTailCommand( const RemoteLogLaunchRequest& request )
{
    if ( request.fullLogFile ) {
        return QString( "tail -n +1 -F %1" ).arg( shellQuote( request.remotePath.trimmed() ) );
    }

    return QString( "tail -n %1 -F %2" )
        .arg( request.initialLines )
        .arg( shellQuote( request.remotePath.trimmed() ) );
}

RemoteLogToolKind inferToolKind( const QString& programPath, RemoteLogToolKind requestedKind )
{
    if ( requestedKind != RemoteLogToolKind::Auto ) {
        return requestedKind;
    }

    const auto fileName = QFileInfo( programPath ).fileName().toLower();
    if ( fileName.contains( "plink" ) ) {
        return RemoteLogToolKind::Plink;
    }

    return RemoteLogToolKind::OpenSsh;
}

QString resolveProgram( RemoteLogToolKind toolKind, const QString& preferredToolPath )
{
    if ( !preferredToolPath.trimmed().isEmpty() ) {
        return preferredToolPath.trimmed();
    }

    if ( toolKind == RemoteLogToolKind::Plink ) {
        return QStandardPaths::findExecutable( "plink.exe" );
    }

    if ( toolKind == RemoteLogToolKind::OpenSsh ) {
        auto program = QStandardPaths::findExecutable( "ssh.exe" );
        if ( program.isEmpty() ) {
            program = QStandardPaths::findExecutable( "ssh" );
        }
        return program;
    }

    auto plink = QStandardPaths::findExecutable( "plink.exe" );
    if ( !plink.isEmpty() ) {
        return plink;
    }

    auto ssh = QStandardPaths::findExecutable( "ssh.exe" );
    if ( ssh.isEmpty() ) {
        ssh = QStandardPaths::findExecutable( "ssh" );
    }
    return ssh;
}
} // namespace

SshProcessLaunchSpec SshProcessTransport::buildLaunchSpec( const RemoteLogLaunchRequest& request,
                                                           const QString& preferredToolPath,
                                                           QString* errorMessage )
{
    SshProcessLaunchSpec spec;

    if ( !request.isValid() ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = QObject::tr( "Remote log request is incomplete." );
        }
        return spec;
    }

    const auto program
        = resolveProgram( request.toolKind, request.toolPath.isEmpty() ? preferredToolPath
                                                                       : request.toolPath );
    if ( program.isEmpty() ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = QObject::tr(
                "No SSH client was found. Configure plink.exe or ssh.exe in the remote log dialog." );
        }
        return spec;
    }

    spec.program = program;
    spec.toolKind = inferToolKind( program, request.toolKind );

    const auto remoteCommand = remoteTailCommand( request );

    if ( spec.toolKind == RemoteLogToolKind::Plink ) {
        spec.arguments << "-batch";
        spec.arguments << "-P" << QString::number( request.port );
        spec.arguments << "-l" << request.user.trimmed();
        if ( request.authMode == RemoteLogAuthMode::Password ) {
            if ( request.password.isEmpty() ) {
                if ( errorMessage != nullptr ) {
                    *errorMessage = QObject::tr( "Password authentication requires a password." );
                }
                return {};
            }
            spec.arguments << "-pw" << request.password;
        }
        spec.arguments << request.host.trimmed();
        spec.arguments << remoteCommand;
        return spec;
    }

    if ( request.authMode == RemoteLogAuthMode::Password ) {
        if ( errorMessage != nullptr ) {
            *errorMessage = QObject::tr(
                "Password authentication is only supported with plink.exe in this version." );
        }
        return {};
    }

    spec.arguments << "-o" << "BatchMode=yes";
    spec.arguments << "-p" << QString::number( request.port );
    spec.arguments << ( request.user.trimmed() + "@" + request.host.trimmed() );
    spec.arguments << remoteCommand;

    return spec;
}
