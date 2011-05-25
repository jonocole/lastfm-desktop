/*
   Copyright 2005-2010 Last.fm Ltd. 
      - Primarily authored by Jono Cole and Michael Coffey

   This file is part of the Last.fm Desktop Application Suite.

   lastfm-desktop is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   lastfm-desktop is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with lastfm-desktop.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Application.h"
#include "lib/unicorn/QMessageBoxBuilder.h"
#include "Services/RadioService.h"
#include "Services/ScrobbleService.h"
#include "lib/listener/PlayerConnection.h"
#include <QDebug>
#include <QProcess>
#include <QShortcut>

#include "Widgets/PointyArrow.h"
#include "Widgets/RadioWidget.h"
#include "Widgets/Drawer.h"
#include "Dialogs/SettingsDialog.h"

#include "MetadataWindow.h"
#include "ScrobbleInfoFetcher.h"
#include "../Widgets/ScrobbleControls.h"
#include "SkipListener.h"

#include "lib/unicorn/dialogs/AboutDialog.h"
#include "lib/unicorn/dialogs/ShareDialog.h"
#include "lib/unicorn/dialogs/TagDialog.h"
#include "lib/unicorn/QMessageBoxBuilder.h"
#include "lib/unicorn/widgets/UserMenu.h"

#include "AudioscrobblerSettings.h"
#include "Wizard/FirstRunWizard.h"

#include <lastfm/XmlQuery>

#include <QRegExp>
#include <QShortcut>
#include <QFileDialog>
#include <QDesktopServices>
#include <QNetworkDiskCache>
#include <QMenu>
#include <QDebug>

#ifdef Q_OS_WIN32g
#include "windows.h"
#endif

using audioscrobbler::Application;

#define ELLIPSIS QString::fromUtf8("…")
#define CONTROL_KEY_CHAR QString::fromUtf8("⌃")
#define APPLE_KEY_CHAR QString::fromUtf8("⌘")

#ifdef Q_WS_X11
    #define AS_TRAY_ICON ":16x16.png"
#elif defined( Q_WS_WIN )
    #define AS_TRAY_ICON ":22x22.png"
#elif defined( Q_WS_MAC )
    #define AS_TRAY_ICON ":systray_icon_rest_mac.png"
#endif

Application::Application(int& argc, char** argv) 
            : unicorn::Application(argc, argv),
              m_raiseHotKeyId( (void*)-1 )
{
    setQuitOnLastWindowClosed( false );
}

void
Application::initiateLogin() throw( StubbornUserException )
{
    if( !unicorn::Settings().value( "FirstRunWizardCompleted", false ).toBool())
    {
        setWizardRunning( true );
        FirstRunWizard w;
        if( w.exec() != QDialog::Accepted ) {
            setWizardRunning( false );
            throw StubbornUserException();
        }
    }
    setWizardRunning( false );

    //this covers the case where the last user was removed
    //and the main window was closed.
    if ( m_mw )
    {
        m_mw->show();
    }

    if ( m_tray )
    {
        //HACK: turns out when all the windows are closed, the tray stops working
        //unless you call the following methods.
        m_tray->hide();
        m_tray->show();
    }
}

void
Application::init()
{
    // Initialise the unicorn base class first!
    unicorn::Application::init();

    QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
    diskCache->setCacheDirectory( lastfm::dir::cache().path() );
    lastfm::nam()->setCache( diskCache );

/// tray
    m_tray = new QSystemTrayIcon(this);
    QIcon trayIcon( AS_TRAY_ICON );
#ifdef Q_WS_MAC
    trayIcon.addFile( ":systray_icon_pressed_mac.png", QSize(), QIcon::Selected );
#endif

#ifdef Q_WS_WIN
    connect( m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT( onTrayActivated(QSystemTrayIcon::ActivationReason)) );
#endif

#ifdef Q_WS_X11
    connect( m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT( onTrayActivated(QSystemTrayIcon::ActivationReason)) );
#endif
    m_tray->setIcon(trayIcon);
    m_tray->show();
    connect( this, SIGNAL( aboutToQuit()), m_tray, SLOT( hide()));

    /// tray menu
    QMenu* menu = new QMenu;
    (menu->addMenu( new UserMenu()))->setText( "Users");

    m_show_window_action = menu->addAction( tr("Show Scrobbler"));
    m_show_window_action->setShortcut( Qt::CTRL + Qt::META + Qt::Key_S );
    menu->addSeparator();
    m_artist_action = menu->addAction( "" );
    m_title_action = menu->addAction(tr("Ready"));

    m_love_action = menu->addAction(tr("Love"));
    m_love_action->setCheckable( true );
    QIcon loveIcon;
    loveIcon.addFile( ":/love-isloved.png", QSize( 16, 16), QIcon::Normal, QIcon::On );
    loveIcon.addFile( ":/love-rest.png", QSize( 16, 16), QIcon::Normal, QIcon::Off );
    m_love_action->setIcon( loveIcon );
    m_love_action->setEnabled( false );
    connect( m_love_action, SIGNAL(triggered(bool)), SLOT(changeLovedState(bool)));

    {
        m_tag_action = menu->addAction(tr("Tag")+ELLIPSIS);
        m_tag_action->setIcon( QIcon( ":/tag-rest.png" ) );
        m_tag_action->setEnabled( false );
        connect( m_tag_action, SIGNAL(triggered()), SLOT(onTagTriggered()));
    }

    {
        m_share_action = menu->addAction(tr("Share")+ELLIPSIS);
        m_share_action->setIcon( QIcon( ":/share-rest.png" ) );
        m_share_action->setEnabled( false );
        connect( m_share_action, SIGNAL(triggered()), SLOT(onShareTriggered()));
    }

    {
        m_ban_action = new QAction( tr( "Ban" ), this );
        QIcon banIcon;
        banIcon.addFile( ":/taskbar-ban.png" );
        m_ban_action->setIcon( banIcon );

        //connect( m_ban_action, SIGNAL(triggered()), SLOT(onBanTriggered()));
    }
    {
        m_play_action = new QAction( tr( "Play" ), this );
        m_play_action->setCheckable( true );
        QIcon playIcon;
        playIcon.addFile( ":/taskbar-pause.png", QSize(), QIcon::Normal, QIcon::On );
        playIcon.addFile( ":/taskbar-play.png", QSize(), QIcon::Normal, QIcon::Off );
        m_play_action->setIcon( playIcon );

        //connect( m_play_action, SIGNAL(triggered(bool)), SLOT(onPlayTriggered(bool)));
    }
    {
        m_skip_action = new QAction( tr( "Skip" ), this );
        QIcon skipIcon;
        skipIcon.addFile( ":/taskbar-skip.png" );
        m_skip_action->setIcon( skipIcon );

        //connect( m_skip_action, SIGNAL(triggered()), SLOT(onSkipTriggered()));
    }

#ifdef Q_WS_X11
    menu->addSeparator();
    m_scrobble_ipod_action = menu->addAction( tr( "Scrobble iPod..." ) );
    connect( m_scrobble_ipod_action, SIGNAL( triggered() ), scrobbleService->deviceScrobbler(), SLOT( onScrobbleIpodTriggered() ) );
#endif

    menu->addSeparator();

    m_visit_profile_action = menu->addAction( tr( "Visit Last.fm profile" ) );
    connect( m_visit_profile_action, SIGNAL( triggered() ), SLOT( onVisitProfileTriggered() ) );

    menu->addSeparator();

    m_submit_scrobbles_toggle = menu->addAction(tr("Submit Scrobbles"));
#ifdef Q_WS_MAC
    m_prefs_action = menu->addAction(tr("Preferences")+ELLIPSIS);
#else
    m_prefs_action = menu->addAction(tr("Options")+ELLIPSIS);
#endif
    connect( m_prefs_action, SIGNAL( triggered() ), this, SLOT( onPrefsTriggered() ) );
    menu->addSeparator();
    QMenu* helpMenu = menu->addMenu( tr( "Help" ) );

    m_faq_action    = helpMenu->addAction( tr( "FAQ" ) );
    m_forums_action = helpMenu->addAction( tr( "Forums" ) );
    m_about_action  = helpMenu->addAction( tr( "About" ) );

    connect( m_faq_action, SIGNAL( triggered() ), SLOT( onFaqTriggered() ) );
    connect( m_forums_action, SIGNAL( triggered() ), SLOT( onForumsTriggered() ) );
    connect( m_about_action, SIGNAL( triggered() ), SLOT( onAboutTriggered() ) );
    menu->addSeparator();

#ifndef NDEBUG
    QAction* rss = menu->addAction( tr("Refresh Stylesheet"), qApp, SLOT(refreshStyleSheet()) );
    rss->setShortcut( Qt::CTRL + Qt::Key_R );

    menu->addSeparator();
#endif

    QAction* quit = menu->addAction(tr("Quit %1").arg( applicationName()));

    connect(quit, SIGNAL(triggered()), SLOT(quit()));

    m_artist_action->setEnabled( false );
    m_title_action->setEnabled( false );
    m_submit_scrobbles_toggle->setCheckable(true);
    m_submit_scrobbles_toggle->setChecked(true);
    m_tray->setContextMenu(menu);

/// MetadataWindow
    m_mw = new MetadataWindow;
    m_mw->addWinThumbBarButton( m_tag_action );
    m_mw->addWinThumbBarButton( m_share_action );
    m_mw->addWinThumbBarButton( m_love_action );
    m_mw->addWinThumbBarButton( m_ban_action );
    m_mw->addWinThumbBarButton( m_play_action );
    m_mw->addWinThumbBarButton( m_skip_action );

    QVBoxLayout* drawerLayout = new QVBoxLayout( m_drawer = new Drawer( m_mw ) );
    drawerLayout->setContentsMargins( 0, 0, 0, 0 );
    drawerLayout->setSpacing( 0 );

    drawerLayout->addWidget( m_radioWidget = new RadioWidget );

    m_toggle_window_action = new QAction( this ), SLOT( trigger());
#ifndef Q_OS_LINUX
     AudioscrobblerSettings settings;
     setRaiseHotKey( settings.raiseShortcutModifiers(), settings.raiseShortcutKey());
#endif
    //although the shortcuts are actually set on the ScrobbleControls widget,
    //setting it here adds the shortkey text to the trayicon menu
    //and it's no problem since, for some reason, the shortcuts don't trigger the actions.
    m_tag_action->setShortcut( Qt::CTRL + Qt::Key_T );
    m_share_action->setShortcut( Qt::CTRL + Qt::Key_S );
    m_love_action->setShortcut( Qt::CTRL + Qt::Key_L );


    // make the love buttons sychronised
    connect(this, SIGNAL(lovedStateChanged(bool)), m_love_action, SLOT(setChecked(bool)));

    // tell the radio that the scrobbler's love state has changed
    connect(this, SIGNAL(lovedStateChanged(bool)), SLOT(sendBusLovedStateChanged(bool)));

    // update the love buttons if love was pressed in the radio
    connect(this, SIGNAL(busLovedStateChanged(bool)), m_love_action, SLOT(setChecked(bool)));
    connect(this, SIGNAL(busLovedStateChanged(bool)), SLOT(onBusLovedStateChanged(bool)));

    // tell everyone that is interested that data about the current track has been fetched
    connect( m_mw, SIGNAL(trackGotInfo(XmlQuery)), SIGNAL(trackGotInfo(XmlQuery)));
    connect( m_mw, SIGNAL(albumGotInfo(XmlQuery)), SIGNAL(albumGotInfo(XmlQuery)));
    connect( m_mw, SIGNAL(artistGotInfo(XmlQuery)), SIGNAL(artistGotInfo(XmlQuery)));
    connect( m_mw, SIGNAL(artistGotEvents(XmlQuery)), SIGNAL(artistGotEvents(XmlQuery)));
    connect( m_mw, SIGNAL(trackGotTopFans(XmlQuery)), SIGNAL(trackGotTopFans(XmlQuery)));
    connect( m_mw, SIGNAL(trackGotTags(XmlQuery)), SIGNAL(trackGotTags(XmlQuery)));
    connect( m_mw, SIGNAL(finished()), SIGNAL(finished()));

    connect( m_mw, SIGNAL(trackGotInfo(XmlQuery)), this, SLOT(onTrackGotInfo(XmlQuery)));



    // connect the radio up so it scrobbles
#warning This code bypasses the mediator - FIXME
    connect( radio, SIGNAL(trackSpooled(Track)), scrobbleService, SLOT(onTrackStarted(Track)));
    connect( radio, SIGNAL(paused()), scrobbleService, SLOT(onPaused()));
    connect( radio, SIGNAL(resumed()), scrobbleService, SLOT(onResumed()));
    connect( radio, SIGNAL(stopped()), scrobbleService, SLOT(onStopped()));

    connect( m_show_window_action, SIGNAL( triggered()), SLOT( showWindow()), Qt::QueuedConnection );
    connect( m_toggle_window_action, SIGNAL( triggered()), SLOT( toggleWindow()), Qt::QueuedConnection );

    connect( this, SIGNAL(messageReceived(QStringList)), SLOT(onMessageReceived(QStringList)) );
    connect( this, SIGNAL( sessionChanged( unicorn::Session* ) ), scrobbleService, SLOT( onSessionChanged( unicorn::Session* ) ) );

    //We're not going to catch the first session change as it happened in the unicorn application before
    //we could connect to the signal!

    if ( !currentSession() )
    {
        QMap<QString, QString> lastSession = unicorn::Session::lastSessionData();
        if ( lastSession.contains( "username" ) && lastSession.contains( "sessionKey" ) )
        {
            changeSession( lastSession[ "username" ], lastSession[ "sessionKey" ] );
        }
    }

    // clicking on a system tray message should show the scrobbler
    connect( m_tray, SIGNAL(messageClicked()), m_show_window_action, SLOT(trigger()));

    // Do this last so that when the user logs in all the interested widgets find out
    initiateLogin();

    emit messageReceived( arguments() );

#ifdef CLIENT_ROOM_RADIO
    new SkipListener( this );
#endif
}

void
Application::setRaiseHotKey( Qt::KeyboardModifiers mods, int key) {
    if( m_raiseHotKeyId >= 0 ) {
        unInstallHotKey( m_raiseHotKeyId );
    }
    m_raiseHotKeyId = installHotKey( mods, key, m_toggle_window_action, SLOT(trigger()));
}

void
Application::onTrackGotInfo(const XmlQuery& lfm)
{
    //Q_ASSERT(m_connection);
    MutableTrack( scrobbleService->currentConnection()->track() ).setFromLfm( lfm );
}


void
Application::onCorrected(QString /*correction*/)
{
    setTrackInfo();
}


void
Application::setTrackInfo()
{ 
    QFontMetrics fm( font() );
    QString durationString = " [" + m_currentTrack.durationString() + "]";

    int actionOffsets = fm.width( durationString );
    int actionWidth = m_tray->contextMenu()->actionGeometry( m_artist_action ).width() - actionOffsets;

    QString artistActionText = fm.elidedText( m_currentTrack.artist( lastfm::Track::Corrected ), Qt::ElideRight, actionWidth );
    QString titleActionText = fm.elidedText( m_currentTrack.title( lastfm::Track::Corrected), Qt::ElideRight, actionWidth - fm.width( durationString ) );

    m_artist_action->setText( artistActionText );
    m_artist_action->setToolTip( m_currentTrack.artist( lastfm::Track::Corrected ) );
    m_title_action->setText( titleActionText + durationString );
    m_title_action->setToolTip( m_currentTrack.title( lastfm::Track::Corrected ) + " [" + m_currentTrack.durationString() + "]" );

    m_tray->setToolTip( m_currentTrack.toString() );

    m_love_action->setEnabled( true );
    m_tag_action->setEnabled( true );
    m_share_action->setEnabled( true );

    // make sure that if the love state changes we update all the buttons
    connect( m_currentTrack.signalProxy(), SIGNAL(loveToggled(bool)), SIGNAL(lovedStateChanged(bool)) );
}

void
Application::resetTrackInfo()
{
    m_artist_action->setText( "" );
    m_title_action->setText( tr( "Ready" ));
    m_love_action->setEnabled( false );
    m_tag_action->setEnabled( false );
    m_share_action->setEnabled( false );
}

void 
Application::onTagTriggered()
{
    TagDialog* td = new TagDialog( m_currentTrack, m_mw );
    td->raise();
    td->show();
    td->activateWindow();
}

void 
Application::onShareTriggered()
{
    ShareDialog* sd = new ShareDialog( m_currentTrack, m_mw );
    sd->raise();
    sd->show();
    sd->activateWindow();
}

void
Application::onVisitProfileTriggered()
{
    QDesktopServices::openUrl( User().www() );
}


void
Application::showRadioDrawer( bool show )
{
    if ( show )
        m_drawer->show();
    else
        m_drawer->close();
}

void
Application::onFaqTriggered()
{
    QDesktopServices::openUrl( "http://" + tr( "www.last.fm" ) + "/help/faq/" );
}

void
Application::onForumsTriggered()
{
    QDesktopServices::openUrl( "http://" + tr( "www.last.fm" ) + "/forum/34905/" );
}

void
Application::onAboutTriggered()
{
    if ( m_aboutDialog ) m_aboutDialog = new AboutDialog( m_mw );
    m_aboutDialog->show();
}

void
Application::onPrefsTriggered()
{
    SettingsDialog* settingsDialog = new SettingsDialog();
    settingsDialog->exec();
}

void 
Application::changeLovedState(bool loved)
{
    MutableTrack track( m_currentTrack );

    if (loved)
        track.love();
    else
        track.unlove();
}

void
Application::onBusLovedStateChanged( bool loved )
{
    MutableTrack( m_currentTrack ).setLoved( loved );
}

void 
Application::onTrayActivated( QSystemTrayIcon::ActivationReason reason ) 
{
    if( reason == QSystemTrayIcon::Context ) return;
#ifdef Q_WS_WIN
    if( reason != QSystemTrayIcon::DoubleClick ) return;
#endif
    m_show_window_action->trigger();
}

void
Application::showWindow()
{
    m_mw->showNormal();
    m_mw->setFocus();
    m_mw->raise();
    m_mw->activateWindow();
}

void
Application::toggleWindow()
{
    if( activeWindow() )
        m_mw->hide();
    else
        showWindow();
}
  

// lastfmlib invokes this directly, for some errors:
void
Application::onWsError( lastfm::ws::Error e )
{
    switch (e)
    {
        case lastfm::ws::InvalidSessionKey:
            quit();
            break;
        default:
            break;
    }
}

  
Application::Argument Application::argument( const QString& arg )
{
    if (arg == "--pause") return Pause;
    if (arg == "--skip") return Skip;
    if (arg == "--exit") return Exit;

    QUrl url( arg );
    //TODO show error if invalid schema and that
    if (url.isValid() && url.scheme() == "lastfm") return LastFmUrl;

    return ArgUnknown;
}

    
void
Application::onMessageReceived( const QStringList& message )
{
    parseArguments( message );

    qDebug() << "Messages: " << message;

    if ( message.contains( "--twiddly" ))
    {
        scrobbleService->handleTwiddlyMessage( message );
    }
    else if ( message.contains( "--exit" ) )
    {
        exit();
    }
    else if ( message.contains( "--settings" ) )
    {
        // raise the settings window
        m_prefs_action->trigger();
    }
    else if ( message.contains( "--new-ipod-detected" ) ||
              message.contains( "--ipod-detected" ))
    {
        scrobbleService->handleIPodDetectedMessage( message );
    }

    if ( !(message.contains( "--tray" ) || message.contains( "--settings" )))
    {
        // raise the app
        m_show_window_action->trigger();
#ifdef Q_OS_WIN32
        SetForegroundWindow(m_mw->winId());
#endif
    }
}


void
Application::parseArguments( const QStringList& args )
{
    qDebug() << args;

    if (args.size() <= 1)
        return;

    foreach (QString const arg, args.mid( 1 ))
        switch (argument( arg ))
        {
        case LastFmUrl:
            radio->play( RadioStation( arg ) );
            break;

        case Exit:
            exit();
            break;

        case Skip:
            radio->skip();
            break;

        case Pause:
            if ( radio->state() == RadioService::Playing )
                radio->pause();
            else if ( radio->state() == RadioService::Paused )
                radio->resume();
            break;

        case ArgUnknown:
            qDebug() << "Unknown argument:" << arg;
            break;
        }
}

void 
Application::quit()
{
    if( activeWindow())
        activeWindow()->raise();

    if( unicorn::AppSettings().value( "quitDontAsk", false ).toBool()) {
        actuallyQuit();
        return;
    }

    bool dontAsk = false;
    int result = 1;
    if( !unicorn::AppSettings().value( "quitDontAsk", false ).toBool())
      result =
          QMessageBoxBuilder( activeWindow()).setTitle( tr("%1 is about to quit.").arg(applicationName()))
                                             .setText( tr("Tracks played in %1 will not be scrobbled if you continue." )
                                                       .arg( PluginList().availableDescription()) )
                                             .dontAskAgain()
                                             .setIcon( QMessageBox::Question )
                                             .setButtons( QMessageBox::Yes | QMessageBox::No )
                                             .exec(&dontAsk);
    if( result == QMessageBox::Yes )
    {
        unicorn::AppSettings().setValue( "quitDontAsk", dontAsk );
        QCoreApplication::quit();
    }
    
}

void 
Application::actuallyQuit()
{
    QDialog* d = qobject_cast<QDialog*>( sender());
    if( d ) {
        QCheckBox* dontAskCB = d->findChild<QCheckBox*>();
        if( dontAskCB ) {
            unicorn::AppSettings().setValue( "quitDontAsk", ( dontAskCB->checkState() == Qt::Checked ));
        }
    }
    QCoreApplication::quit();
}
