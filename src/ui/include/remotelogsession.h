#ifndef KLOGG_REMOTELOGSESSION_H
#define KLOGG_REMOTELOGSESSION_H

#include <QFile>
#include <QObject>
#include <QProcess>
#include <QElapsedTimer>
#include <QTimer>

#include "remotelogtypes.h"

class RemoteLogSession : public QObject {
    Q_OBJECT

  public:
    explicit RemoteLogSession( RemoteLogLaunchRequest request, QString mirrorPath,
                               QString stderrPath, QString preferredToolPath,
                               QObject* parent = nullptr );
    ~RemoteLogSession() override;

    [[nodiscard]] bool start( QString* errorMessage );
    void stop( bool cleanupFiles );

    [[nodiscard]] QString mirrorPath() const
    {
        return mirrorPath_;
    }

    [[nodiscard]] QString stderrPath() const
    {
        return stderrPath_;
    }

    [[nodiscard]] const RemoteLogProfile& profile() const
    {
        return request_;
    }

    [[nodiscard]] QString sourceLabel() const
    {
        return request_.sourceLabel();
    }

    [[nodiscard]] QString tabLabel() const
    {
        return request_.title();
    }

    [[nodiscard]] RemoteLogSessionState state() const
    {
        return state_;
    }

    [[nodiscard]] QString statusText() const;
    [[nodiscard]] QString uiMessage() const
    {
        return uiMessage_;
    }
    [[nodiscard]] QString diagnostics() const
    {
        return diagnostics_;
    }
    [[nodiscard]] bool failedDuringInitialConnect() const;

  Q_SIGNALS:
    void started();
    void stateChanged( RemoteLogSessionState state );
    void warningsChanged( const QString& warnings );
    void finished();

  private Q_SLOTS:
    void handleStarted();
    void handleReadyReadStandardOutput();
    void handleReadyReadStandardError();
    void handleFinished( int exitCode, QProcess::ExitStatus exitStatus );
    void handleErrorOccurred( QProcess::ProcessError error );
    void handleStopTimeout();

  private:
    void setState( RemoteLogSessionState state );
    bool prepareFiles( QString* errorMessage );
    void appendDiagnostics( const QString& text );
    void setUiMessage( const QString& message );
    void cleanupFiles();
    void finalizeExplicitStop();

    RemoteLogLaunchRequest request_;
    QString mirrorPath_;
    QString stderrPath_;
    QString preferredToolPath_;
    QFile mirrorFile_;
    QFile stderrFile_;
    QProcess process_;
    RemoteLogSessionState state_ = RemoteLogSessionState::Starting;
    QString uiMessage_;
    QString diagnostics_;
    bool explicitStopRequested_ = false;
    bool cleanupRequested_ = false;
    bool stopFinalized_ = false;
    bool hasDeliveredRemoteData_ = false;
    QElapsedTimer runningTimer_;
    QTimer stopTimer_;
};

#endif
