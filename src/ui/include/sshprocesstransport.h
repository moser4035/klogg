#ifndef KLOGG_SSHPROCESSTRANSPORT_H
#define KLOGG_SSHPROCESSTRANSPORT_H

#include <QStringList>

#include "remotelogtypes.h"

struct SshProcessLaunchSpec {
    QString program;
    QStringList arguments;
    RemoteLogToolKind toolKind = RemoteLogToolKind::Auto;

    [[nodiscard]] bool isValid() const
    {
        return !program.isEmpty();
    }
};

class SshProcessTransport {
  public:
    static SshProcessLaunchSpec buildLaunchSpec( const RemoteLogLaunchRequest& request,
                                                 const QString& preferredToolPath,
                                                 QString* errorMessage );
};

#endif
