#ifndef KLOGG_REMOTELOGTYPES_H
#define KLOGG_REMOTELOGTYPES_H

#include <QString>

enum class RemoteLogAuthMode {
    Password,
    KeyAgent,
};

enum class RemoteLogToolKind {
    Auto,
    Plink,
    OpenSsh,
};

enum class RemoteLogSessionState {
    Starting,
    Running,
    Stopped,
    Failed,
};

struct RemoteLogProfile {
    QString displayName;
    QString host;
    int port = 22;
    QString user;
    QString remotePath;
    RemoteLogAuthMode authMode = RemoteLogAuthMode::KeyAgent;
    RemoteLogToolKind toolKind = RemoteLogToolKind::Auto;
    QString toolPath;
    int initialLines = 2000;

    [[nodiscard]] bool isValid() const
    {
        return !host.trimmed().isEmpty() && !user.trimmed().isEmpty()
            && !remotePath.trimmed().isEmpty() && port > 0;
    }

    [[nodiscard]] QString title() const
    {
        if ( !displayName.trimmed().isEmpty() ) {
            return displayName.trimmed();
        }

        const auto fileName = remotePath.section( '/', -1, -1 );
        if ( !fileName.isEmpty() ) {
            return host + ":" + fileName;
        }

        return host + ":" + remotePath;
    }

    [[nodiscard]] QString connectionLabel() const
    {
        return user.trimmed() + "@" + host.trimmed() + ":" + QString::number( port );
    }

    [[nodiscard]] QString sourceLabel() const
    {
        return connectionLabel() + remotePath.trimmed();
    }
};

struct RemoteLogLaunchRequest : public RemoteLogProfile {
    QString password;
};

#endif
