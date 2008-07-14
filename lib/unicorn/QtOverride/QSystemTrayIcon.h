/***************************************************************************
 *   Copyright 2008 Last.fm Ltd.                                           *
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

#ifndef QT_OVERRIDES_SYSTEM_TRAY_ICON_H
#define QT_OVERRIDES_SYSTEM_TRAY_ICON_H

#include <QtGui/QSystemTrayIcon>

#ifdef Q_OS_MAC


namespace Moose
{
    /** @author Max Howell <max@last.fm>
      */
    class QSystemTrayIcon : public ::QSystemTrayIcon
    {
    public:
        QSystemTrayIcon( QObject* parent ) : ::QSystemTrayIcon( parent )
        {}

        void showMessage( const QString& title,
                          const QString& text,
                          MessageIcon icon = Information,
                          int millisecondsTimeoutHint = 10000 );
    };
}


#define QSystemTrayIcon Moose::QSystemTrayIcon

#endif
#endif
