#include "remotelogdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "remotelogsettings.h"

namespace {
int toolKindIndex( RemoteLogToolKind toolKind )
{
    switch ( toolKind ) {
    case RemoteLogToolKind::Auto:
        return 0;
    case RemoteLogToolKind::Plink:
        return 1;
    case RemoteLogToolKind::OpenSsh:
        return 2;
    }

    return 0;
}

RemoteLogToolKind toolKindFromIndex( int index )
{
    switch ( index ) {
    case 1:
        return RemoteLogToolKind::Plink;
    case 2:
        return RemoteLogToolKind::OpenSsh;
    default:
        return RemoteLogToolKind::Auto;
    }
}

int authModeIndex( RemoteLogAuthMode authMode )
{
    return authMode == RemoteLogAuthMode::Password ? 0 : 1;
}

RemoteLogAuthMode authModeFromIndex( int index )
{
    return index == 0 ? RemoteLogAuthMode::Password : RemoteLogAuthMode::KeyAgent;
}
} // namespace

RemoteLogDialog::RemoteLogDialog( const std::vector<RemoteLogProfile>& recentTargets,
                                  int defaultPort, int defaultInitialLines,
                                  const QString& preferredToolPath, QWidget* parent )
    : QDialog( parent )
    , recentTargets_( recentTargets )
{
    setWindowTitle( tr( "Open Remote Log" ) );

    auto* mainLayout = new QVBoxLayout( this );
    auto* introLabel = new QLabel(
        tr( "Open a remote log over SSH and mirror it into a local temp file for live following." ),
        this );
    introLabel->setWordWrap( true );
    mainLayout->addWidget( introLabel );

    auto* formLayout = new QFormLayout();

    recentTargetsCombo_ = new QComboBox( this );
    removeRecentTargetButton_ = new QPushButton( tr( "Remove selected" ), this );
    clearRecentTargetsButton_ = new QPushButton( tr( "Clear all" ), this );

    auto* recentTargetsLayout = new QHBoxLayout();
    recentTargetsLayout->setContentsMargins( 0, 0, 0, 0 );
    recentTargetsLayout->addWidget( recentTargetsCombo_ );
    recentTargetsLayout->addWidget( removeRecentTargetButton_ );
    recentTargetsLayout->addWidget( clearRecentTargetsButton_ );
    formLayout->addRow( tr( "Recent target" ), recentTargetsLayout );

    displayNameEdit_ = new QLineEdit( this );
    formLayout->addRow( tr( "Display name" ), displayNameEdit_ );

    hostEdit_ = new QLineEdit( this );
    formLayout->addRow( tr( "Host" ), hostEdit_ );

    portSpin_ = new QSpinBox( this );
    portSpin_->setRange( 1, 65535 );
    portSpin_->setValue( defaultPort );
    formLayout->addRow( tr( "Port" ), portSpin_ );

    userEdit_ = new QLineEdit( this );
    formLayout->addRow( tr( "User" ), userEdit_ );

    remotePathEdit_ = new QLineEdit( this );
    formLayout->addRow( tr( "Remote log path" ), remotePathEdit_ );

    authModeCombo_ = new QComboBox( this );
    authModeCombo_->addItem( tr( "Password" ) );
    authModeCombo_->addItem( tr( "Key / Agent" ) );
    formLayout->addRow( tr( "Authentication" ), authModeCombo_ );

    passwordEdit_ = new QLineEdit( this );
    passwordEdit_->setEchoMode( QLineEdit::Password );
    formLayout->addRow( tr( "Password" ), passwordEdit_ );

    initialLinesSpin_ = new QSpinBox( this );
    initialLinesSpin_->setRange( 1, 1000000 );
    initialLinesSpin_->setValue( defaultInitialLines );
    formLayout->addRow( tr( "Initial tail lines" ), initialLinesSpin_ );

    toolKindCombo_ = new QComboBox( this );
    toolKindCombo_->addItem( tr( "Auto detect" ) );
    toolKindCombo_->addItem( tr( "PuTTY plink" ) );
    toolKindCombo_->addItem( tr( "OpenSSH ssh" ) );
    formLayout->addRow( tr( "SSH tool" ), toolKindCombo_ );

    toolPathEdit_ = new QLineEdit( preferredToolPath, this );
    formLayout->addRow( tr( "Custom tool path" ), toolPathEdit_ );

    mainLayout->addLayout( formLayout );

    buttonBox_ = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this );
    mainLayout->addWidget( buttonBox_ );

    connect( recentTargetsCombo_, QOverload<int>::of( &QComboBox::currentIndexChanged ), this,
             &RemoteLogDialog::updateFromRecentSelection );
    connect( authModeCombo_, QOverload<int>::of( &QComboBox::currentIndexChanged ), this,
             &RemoteLogDialog::updateAuthUi );
    connect( removeRecentTargetButton_, &QPushButton::clicked, this,
             &RemoteLogDialog::removeSelectedRecentTarget );
    connect( clearRecentTargetsButton_, &QPushButton::clicked, this,
             &RemoteLogDialog::clearRecentTargets );
    connect( buttonBox_, &QDialogButtonBox::accepted, this, &RemoteLogDialog::validateAndAccept );
    connect( buttonBox_, &QDialogButtonBox::rejected, this, &RemoteLogDialog::reject );

    refreshRecentTargets();
    updateAuthUi();
}

RemoteLogLaunchRequest RemoteLogDialog::request() const
{
    RemoteLogLaunchRequest request;
    request.displayName = displayNameEdit_->text().trimmed();
    request.host = hostEdit_->text().trimmed();
    request.port = portSpin_->value();
    request.user = userEdit_->text().trimmed();
    request.remotePath = remotePathEdit_->text().trimmed();
    request.authMode = authModeFromIndex( authModeCombo_->currentIndex() );
    request.password = passwordEdit_->text();
    request.initialLines = initialLinesSpin_->value();
    request.toolKind = toolKindFromIndex( toolKindCombo_->currentIndex() );
    request.toolPath = toolPathEdit_->text().trimmed();
    return request;
}

void RemoteLogDialog::updateFromRecentSelection( int index )
{
    updateRecentTargetActions();

    if ( index <= 0 || index > static_cast<int>( recentTargets_.size() ) ) {
        return;
    }

    applyProfile( recentTargets_.at( static_cast<size_t>( index - 1 ) ) );
}

void RemoteLogDialog::applyProfile( const RemoteLogProfile& profile )
{
    displayNameEdit_->setText( profile.displayName );
    hostEdit_->setText( profile.host );
    portSpin_->setValue( profile.port );
    userEdit_->setText( profile.user );
    remotePathEdit_->setText( profile.remotePath );
    authModeCombo_->setCurrentIndex( authModeIndex( profile.authMode ) );
    initialLinesSpin_->setValue( profile.initialLines );
    toolKindCombo_->setCurrentIndex( toolKindIndex( profile.toolKind ) );
    toolPathEdit_->setText( profile.toolPath );
}

void RemoteLogDialog::updateAuthUi()
{
    const auto passwordMode = authModeFromIndex( authModeCombo_->currentIndex() )
        == RemoteLogAuthMode::Password;
    passwordEdit_->setEnabled( passwordMode );
}

void RemoteLogDialog::removeSelectedRecentTarget()
{
    const auto currentIndex = recentTargetsCombo_->currentIndex();
    if ( currentIndex <= 0 || currentIndex > static_cast<int>( recentTargets_.size() ) ) {
        return;
    }

    const auto profile = recentTargets_.at( static_cast<size_t>( currentIndex - 1 ) );
    auto& remoteLogSettings = RemoteLogSettings::getSynced();
    remoteLogSettings.removeRecentTarget( profile );
    remoteLogSettings.save();
    recentTargets_ = remoteLogSettings.recentTargets();
    refreshRecentTargets();
}

void RemoteLogDialog::clearRecentTargets()
{
    if ( recentTargets_.empty() ) {
        return;
    }

    const auto userAction = QMessageBox::question(
        this, tr( "Open Remote Log" ),
        tr( "Remove all recent remote targets from the list?" ), QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );
    if ( userAction != QMessageBox::Yes ) {
        return;
    }

    auto& remoteLogSettings = RemoteLogSettings::getSynced();
    remoteLogSettings.clearRecentTargets();
    remoteLogSettings.save();
    recentTargets_ = remoteLogSettings.recentTargets();
    refreshRecentTargets();
}

void RemoteLogDialog::refreshRecentTargets()
{
    const auto previousSelection = recentTargetsCombo_->currentIndex();
    recentTargetsCombo_->blockSignals( true );
    recentTargetsCombo_->clear();
    recentTargetsCombo_->addItem( tr( "Custom target" ) );
    for ( const auto& target : recentTargets_ ) {
        recentTargetsCombo_->addItem( target.title() + " (" + target.sourceLabel() + ")" );
    }

    const auto hasPreviousRecentSelection = previousSelection > 0 && previousSelection
        <= static_cast<int>( recentTargets_.size() );
    recentTargetsCombo_->setCurrentIndex( hasPreviousRecentSelection ? previousSelection : 0 );
    recentTargetsCombo_->blockSignals( false );
    updateRecentTargetActions();
}

void RemoteLogDialog::updateRecentTargetActions()
{
    const auto hasRecentTargets = !recentTargets_.empty();
    removeRecentTargetButton_->setEnabled( hasRecentTargets && recentTargetsCombo_->currentIndex() > 0 );
    clearRecentTargetsButton_->setEnabled( hasRecentTargets );
}

void RemoteLogDialog::validateAndAccept()
{
    const auto currentRequest = request();
    if ( currentRequest.host.isEmpty() || currentRequest.user.isEmpty()
         || currentRequest.remotePath.isEmpty() ) {
        QMessageBox::warning( this, tr( "Open Remote Log" ),
                              tr( "Host, user and remote log path are required." ) );
        return;
    }

    if ( currentRequest.authMode == RemoteLogAuthMode::Password
         && currentRequest.password.isEmpty() ) {
        QMessageBox::warning( this, tr( "Open Remote Log" ),
                              tr( "Please enter a password or choose key/agent authentication." ) );
        return;
    }

    accept();
}
