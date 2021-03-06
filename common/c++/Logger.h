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

#ifndef LOGGER_H
#define LOGGER_H

#include "lib/DllExportMacro.h"

#include "common/c++/string.h"
#include <fstream>
#include <sstream>
#ifdef WIN32
#include <windows.h> //for CRITICAL_SECTION
#pragma warning(disable: 4251)
#endif


class LOGGER_DLLEXPORT Logger
{
public:
    enum Severity
    {
        Critical = 1,
        Warning,
        Info,
        Debug
    };

    /** Sets the Logger instance to this, so only call once, and make it exist
      * as long as the application does */
    explicit Logger( const COMMON_CHAR* filename, Severity severity = Info );
    ~Logger();

    static Logger& the();

    void log( Severity level, const std::string& message, const char* function, int line );
    void log( Severity level, const std::wstring& message, const char* function, int line );
    
    /** plain write + flush, we suggest utf8 */
    void log( const char* message );

    static void truncate( const COMMON_CHAR* path );
    
private:
    const Severity mLevel;
#ifdef WIN32
    CRITICAL_SECTION mMutex;
#else
    pthread_mutex_t mMutex;
#endif
    std::ofstream mFileOut;
};


/** use these to log */
#define LOG( level, msg ) { \
    std::ostringstream ss; \
    ss << msg; \
    Logger::the().log( (Logger::Severity) level, ss.str(), __FUNCTION__, __LINE__ ); }
#define LOGL LOG
#define LOGW( level, msg ) { \
	std::wostringstream ss; \
	ss << msg; \
        Logger::the().log( (Logger::Severity) level, ss.str(), __FUNCTION__, __LINE__ ); }
#define LOGWL LOGW
#endif
