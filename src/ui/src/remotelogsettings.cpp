#include "remotelogsettings.h"

#include <QSettings>

#include <algorithm>

#include "log.h"

namespace {
constexpr int RemoteLogSettingsVersion = 1;

QString authModeToString( RemoteLogAuthMode authMode )
{
    switch ( authMode ) {
    case RemoteLogAuthMode::Password:
        return "password";
    case RemoteLogAuthMode::KeyAgent:
        return "keyAgent";
    }

    return "keyAgent";
}

RemoteLogAuthMode authModeFromString( const QString& value )
{
    if ( value == "password" ) {
        return RemoteLogAuthMode::Password;
    }

    return RemoteLogAuthMode::KeyAgent;
}

QString toolKindToString( RemoteLogToolKind toolKind )
{
    switch ( toolKind ) {
    case RemoteLogToolKind::Auto:
        return "auto";
    case RemoteLogToolKind::Plink:
        return "plink";
    case RemoteLogToolKind::OpenSsh:
        return "openssh";
    }

    return "auto";
}

RemoteLogToolKind toolKindFromString( const QString& value )
{
    if ( value == "plink" ) {
        return RemoteLogToolKind::Plink;
    }
    if ( value == "openssh" ) {
        return RemoteLogToolKind::OpenSsh;
    }

    return RemoteLogToolKind::Auto;
}
} // namespace

void RemoteLogSettings::addRecentTarget( const RemoteLogProfile& profile )
{
    if ( !profile.isValid() ) {
        return;
    }

    recentTargets_.erase(
        std::remove_if( recentTargets_.begin(), recentTargets_.end(),
                        [ &profile ]( const auto& existing ) {
                            return existing.host == profile.host && existing.port == profile.port
                                && existing.user == profile.user
                                && existing.remotePath == profile.remotePath;
                        } ),
        recentTargets_.end() );

    recentTargets_.insert( recentTargets_.begin(), profile );

    if ( recentTargets_.size() > MaxRecentTargets ) {
        recentTargets_.resize( MaxRecentTargets );
    }
}

void RemoteLogSettings::removeRecentTarget( const RemoteLogProfile& profile )
{
    recentTargets_.erase(
        std::remove_if( recentTargets_.begin(), recentTargets_.end(),
                        [ &profile ]( const auto& existing ) {
                            return existing.host == profile.host && existing.port == profile.port
                                && existing.user == profile.user
                                && existing.remotePath == profile.remotePath;
                        } ),
        recentTargets_.end() );
}

void RemoteLogSettings::clearRecentTargets()
{
    recentTargets_.clear();
}

void RemoteLogSettings::saveToStorage( QSettings& settings ) const
{
    LOG_DEBUG << "RemoteLogSettings::saveToStorage";

    settings.beginGroup( "RemoteLogSettings" );
    settings.setValue( "version", RemoteLogSettingsVersion );
    settings.setValue( "defaultPort", defaultPort_ );
    settings.setValue( "defaultInitialLines", defaultInitialLines_ );
    settings.setValue( "preferredToolPath", preferredToolPath_ );
    settings.remove( "recentTargets" );
    settings.beginWriteArray( "recentTargets" );
    for ( auto i = 0u; i < recentTargets_.size(); ++i ) {
        settings.setArrayIndex( static_cast<int>( i ) );
        const auto& target = recentTargets_.at( i );
        settings.setValue( "displayName", target.displayName );
        settings.setValue( "host", target.host );
        settings.setValue( "port", target.port );
        settings.setValue( "user", target.user );
        settings.setValue( "remotePath", target.remotePath );
        settings.setValue( "authMode", authModeToString( target.authMode ) );
        settings.setValue( "toolKind", toolKindToString( target.toolKind ) );
        settings.setValue( "toolPath", target.toolPath );
        settings.setValue( "initialLines", target.initialLines );
    }
    settings.endArray();
    settings.endGroup();
}

void RemoteLogSettings::retrieveFromStorage( QSettings& settings )
{
    LOG_DEBUG << "RemoteLogSettings::retrieveFromStorage";

    recentTargets_.clear();

    if ( !settings.contains( "RemoteLogSettings/version" ) ) {
        return;
    }

    settings.beginGroup( "RemoteLogSettings" );
    if ( settings.value( "version" ).toInt() == RemoteLogSettingsVersion ) {
        defaultPort_ = settings.value( "defaultPort", defaultPort_ ).toInt();
        defaultInitialLines_
            = settings.value( "defaultInitialLines", defaultInitialLines_ ).toInt();
        preferredToolPath_ = settings.value( "preferredToolPath" ).toString();

        auto size = settings.beginReadArray( "recentTargets" );
        for ( int i = 0; i < size; ++i ) {
            settings.setArrayIndex( i );
            RemoteLogProfile profile;
            profile.displayName = settings.value( "displayName" ).toString();
            profile.host = settings.value( "host" ).toString();
            profile.port = settings.value( "port", defaultPort_ ).toInt();
            profile.user = settings.value( "user" ).toString();
            profile.remotePath = settings.value( "remotePath" ).toString();
            profile.authMode = authModeFromString( settings.value( "authMode" ).toString() );
            profile.toolKind = toolKindFromString( settings.value( "toolKind" ).toString() );
            profile.toolPath = settings.value( "toolPath" ).toString();
            profile.initialLines
                = settings.value( "initialLines", defaultInitialLines_ ).toInt();
            if ( profile.isValid() ) {
                recentTargets_.push_back( profile );
            }
        }
        settings.endArray();
    }
    else {
        LOG_ERROR << "Unknown version of remote log settings, ignoring it...";
    }

    settings.endGroup();
}
