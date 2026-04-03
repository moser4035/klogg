#ifndef KLOGG_REMOTELOGDIALOG_H
#define KLOGG_REMOTELOGDIALOG_H

#include <vector>

#include <QDialog>

#include "remotelogtypes.h"

class QComboBox;
class QDialogButtonBox;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

class RemoteLogDialog : public QDialog {
    Q_OBJECT

  public:
    explicit RemoteLogDialog( const std::vector<RemoteLogProfile>& recentTargets,
                              int defaultPort, int defaultInitialLines,
                              const QString& preferredToolPath, QWidget* parent = nullptr );

    [[nodiscard]] RemoteLogLaunchRequest request() const;

  private Q_SLOTS:
    void updateFromRecentSelection( int index );
    void updateAuthUi();
    void updateInitialLinesUi();
    void removeSelectedRecentTarget();
    void clearRecentTargets();
    void validateAndAccept();

  private:
    void applyProfile( const RemoteLogProfile& profile );
    void refreshRecentTargets();
    void updateRecentTargetActions();

    std::vector<RemoteLogProfile> recentTargets_;
    QComboBox* recentTargetsCombo_ = nullptr;
    QPushButton* removeRecentTargetButton_ = nullptr;
    QPushButton* clearRecentTargetsButton_ = nullptr;
    QLineEdit* displayNameEdit_ = nullptr;
    QLineEdit* hostEdit_ = nullptr;
    QSpinBox* portSpin_ = nullptr;
    QLineEdit* userEdit_ = nullptr;
    QLineEdit* remotePathEdit_ = nullptr;
    QComboBox* authModeCombo_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QCheckBox* fullLogFileCheckBox_ = nullptr;
    QSpinBox* initialLinesSpin_ = nullptr;
    QComboBox* toolKindCombo_ = nullptr;
    QLineEdit* toolPathEdit_ = nullptr;
    QDialogButtonBox* buttonBox_ = nullptr;
};

#endif
