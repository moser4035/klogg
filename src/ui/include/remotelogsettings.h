#ifndef KLOGG_REMOTELOGSETTINGS_H
#define KLOGG_REMOTELOGSETTINGS_H

#include <vector>

#include "persistable.h"
#include "remotelogtypes.h"

class RemoteLogSettings final : public Persistable<RemoteLogSettings> {
  public:
    static const char* persistableName()
    {
        return "RemoteLogSettings";
    }

    [[nodiscard]] int defaultPort() const
    {
        return defaultPort_;
    }

    void setDefaultPort( int port )
    {
        if ( port > 0 ) {
            defaultPort_ = port;
        }
    }

    [[nodiscard]] int defaultInitialLines() const
    {
        return defaultInitialLines_;
    }

    void setDefaultInitialLines( int lines )
    {
        if ( lines > 0 ) {
            defaultInitialLines_ = lines;
        }
    }

    [[nodiscard]] QString preferredToolPath() const
    {
        return preferredToolPath_;
    }

    void setPreferredToolPath( const QString& toolPath )
    {
        preferredToolPath_ = toolPath.trimmed();
    }

    [[nodiscard]] std::vector<RemoteLogProfile> recentTargets() const
    {
        return recentTargets_;
    }

    void addRecentTarget( const RemoteLogProfile& profile );

    void saveToStorage( QSettings& settings ) const;
    void retrieveFromStorage( QSettings& settings );

  private:
    static constexpr int MaxRecentTargets = 10;

    int defaultPort_ = 22;
    int defaultInitialLines_ = 2000;
    QString preferredToolPath_;
    std::vector<RemoteLogProfile> recentTargets_;
};

#endif
