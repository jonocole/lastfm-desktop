/*
   Copyright 2005-2009 Last.fm Ltd. 
      - Primarily authored by Max Howell, Jono Cole and Doug Mansell

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
#ifndef SCROBBLE_SERVICE_H
#define SCROBBLE_SERVICE_H

#include <QObject>
#include <QPointer>
#include <lastfm/Track>

namespace unicorn { class Session; }
class PlayerMediator;
class PlayerConnection;
class Audioscrobbler;
class StopWatch;
class DeviceScrobbler;

class ScrobbleService : public QObject
{
    Q_OBJECT

public:
    ScrobbleService();
    ~ScrobbleService();

    Track currentTrack() const {return m_currentTrack;}
    QPointer<DeviceScrobbler> deviceScrobbler() { return m_deviceScrobbler; }
    QPointer<PlayerConnection> currentConnection() { return m_connection; }
    QPointer<StopWatch> stopWatch() { return m_watch; }

    void handleTwiddlyMessage( const QStringList& message );
    void handleIPodDetectedMessage( const QStringList& message );

    enum State
    {
        Unknown,
        Stopped,
        Paused,
        Playing
    };

public slots:
    void onSessionChanged( unicorn::Session* );
    void onScrobble();

signals:
    void trackStarted( Track, Track );
    void resumed();
    void paused();
    void stopped();
    void scrobblesCached( const QList<lastfm::Track>& tracks );
    void scrobblesSubmitted( const QList<lastfm::Track>& tracks, int numTracks );

    void foundIPodScrobbles( const QList<lastfm::Track>& tracks );
    void bootstrapReady( const QString& playerId );

    void paused( bool );
    void frameChanged( int );
    void timeout();


protected slots:
    void setConnection( PlayerConnection* );
    void onTrackStarted( const Track& );
    void onTrackStarted( const Track&, const Track& );
    void onPaused();
    void onResumed(); 
    void onStopped();

protected:
    State state;

    QPointer <StopWatch> m_watch;
    QPointer <PlayerMediator> m_mediator;
    QPointer <PlayerConnection> m_connection;
    QPointer <Audioscrobbler> m_as;
    QPointer <DeviceScrobbler> m_deviceScrobbler;
    Track m_currentTrack;
    Track m_trackToScrobble;
};

extern ScrobbleService* scrobbleService;

#endif //SCROBBLE_SERVICE_H_