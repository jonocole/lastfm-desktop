/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef QT_OVERRIDES_HTTP_H
#define QT_OVERRIDES_HTTP_H

#include "lib/DllExportMacro.h"
#include <QtNetwork/QHttp>

//TODO add user agent adjustment

namespace Unicorn
{
    /** @author Max Howell <max@last.fm>
      */
    class UNICORN_DLLEXPORT QHttp : public ::QHttp
    {
        QString m_host;
        
        void init();
        void applyAutoDetectedProxy();
        
    public:
        QHttp( const QString& host, quint16 port = 80, QObject* parent = 0 );
        QHttp( QObject* parent );
        
        QString host() const { return m_host; }
        int setHost( const QString & hostName, quint16 port = 80 ) { m_host = hostName; return ::QHttp::setHost( hostName, port ); }
        
        int get( const QString& path );
        int post( const QString& path, const QByteArray& data );
        int request( QHttpRequestHeader, const QByteArray& data = QByteArray() );
        
        /** all unimplemented */
        int get( const QString & path, QIODevice * to );
        int post( const QString & path, QIODevice * data, QIODevice * to );
        int post( const QString & path, const QByteArray & data, QIODevice * to );
        int request( const QHttpRequestHeader & header, QIODevice * data, QIODevice * to );
        int request( const QHttpRequestHeader & header, const QByteArray & data, QIODevice * to );
        int setHost( const QString & hostName, ConnectionMode mode, quint16 port );
        
        static QString stateString( QHttp::State state )
        {
            switch ( state )
            {
                case QHttp::Unconnected: return "No connection";
                case QHttp::HostLookup: return "Looking up host";
                case QHttp::Connecting: return "Connecting";
                case QHttp::Sending: return "Sending request";
                case QHttp::Reading: return "Downloading";
                case QHttp::Connected: return "Connected";
                case QHttp::Closing: return "Closing connection";
                default: return QString();
            }
        }
        
    };
}


#define QHttp Unicorn::QHttp

#endif
