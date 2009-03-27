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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef _WIN32_WINNT
// bring in CoInitializeSecurity from objbase.h
#define _WIN32_WINNT 0x0400
#endif

#include <objbase.h>
#include <atlbase.h>
#include <atlcom.h>


/** @brief WsConnectionMonitor needs Com to work as early as possible so we do this
  * @author <doug@last.fm> 
  */
class ComSetup
{
public:
    ComSetup()
    {
        HRESULT hr = CoInitialize(0);
        m_bComInitialised = SUCCEEDED(hr);
        _ASSERT(m_bComInitialised);
        if (m_bComInitialised) {
            setupSecurity();
        }
    }
    
    void setupSecurity()
    {
        CSecurityDescriptor sd;
        sd.InitializeFromThreadToken();
        HRESULT hr = CoInitializeSecurity(sd, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL); 
        _ASSERT(SUCCEEDED(hr));
    }
    
    ~ComSetup()
    {
        if (m_bComInitialised) {
            CoUninitialize();
        }
    }
    
private:
    bool m_bComInitialised;
};
