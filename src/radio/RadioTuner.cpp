/***************************************************************************
 *   Copyright 2005-2009 Last.fm Ltd.                                      *
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

#include "RadioTuner.h"
#include "../types/Xspf.h"
#include "../ws/WsRequestBuilder.h"
#include "../ws/WsReply.h"
#include <QBuffer>
#include <QDebug>
#include <QtXml>

using lastfm::Track;
using lastfm::Xspf;

//TODO discovery mode
//TODO skips left
//TODO multiple locations for the same track
//TODO set rtp flag in getPlaylist (whether user is scrobbling this radio session or not)


lastfm::RadioTuner::RadioTuner( const RadioStation& station )
     : m_retry_counter( 0 )
{
    WsReply* reply = WsRequestBuilder( "radio.tune" )
			.add( "station", station.url() )
			.post();
	connect( reply, SIGNAL(finished( WsReply* )), SLOT(onTuneReturn( WsReply* )) );
}


void
lastfm::RadioTuner::onTuneReturn( WsReply* reply )
{
    qDebug() << reply;
    
    if (reply->error() != Ws::NoError) {
		emit error( reply->error() );
		return;
	}

	try {
		emit title( reply->lfm()["station"]["name"].text() );
	}
	catch (std::runtime_error&)
	{}
	
	fetchFiveMoreTracks();
}


bool
lastfm::RadioTuner::fetchFiveMoreTracks()
{
    WsReply* reply = WsRequestBuilder( "radio.getPlaylist" ).add( "rtp", "1" ).get();
	connect( reply, SIGNAL(finished( WsReply* )), SLOT(onGetPlaylistReturn( WsReply* )) );
    return true;
}


bool
lastfm::RadioTuner::tryAgain()
{
    qDebug() << "Bad response count" << m_retry_counter;
    
	if (++m_retry_counter > 5)
		return false;
	fetchFiveMoreTracks();
	return true;
}


void
lastfm::RadioTuner::onGetPlaylistReturn( WsReply* reply )
{   
	switch (reply->error())
	{
		case Ws::NoError:
			break;

		case Ws::TryAgainLater:
			if (!tryAgain())
				emit error( Ws::TryAgainLater );
			return;

		default:
			emit error( reply->error() );
			return;
	}
    
    try
    {
        Xspf xspf( reply->lfm()["playlist"] );

        QList<Track> tracks( xspf.tracks() );
        if (tracks.isEmpty())
        {
            // sometimes the recs service craps out and gives us a blank playlist
            
            if (!tryAgain())
            {
                // an empty playlist is a bug, if there is no content
                // NotEnoughContent should have been returned with the WsReply
                emit error( Ws::MalformedResponse );
            }
        }
        else {
            m_retry_counter = 0;
            m_queue += tracks;
            emit trackAvailable();
        }
    }
    catch (std::runtime_error& e)
    {
        qWarning() << e.what();
        if (!tryAgain())
            emit error( Ws::TryAgainLater );
    }
}


Track
lastfm::RadioTuner::takeNextTrack()
{
    //TODO presumably, we should check if fetchMoreTracks is working?
    if (m_queue.isEmpty())
        return Track();
    
    Track result = m_queue.takeFirst();
    if (m_queue.isEmpty())
        fetchFiveMoreTracks();

    return result;
}
