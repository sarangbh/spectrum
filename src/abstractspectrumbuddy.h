/**
 * XMPP - libpurple transport
 *
 * Copyright (C) 2009, Jan Kaluza <hanzz@soc.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#ifndef ABSTRACT_SPECTRUM_BUDDY_H
#define ABSTRACT_SPECTRUM_BUDDY_H

#include <string>
#include "purple.h"
#include "account.h"
#include "glib.h"
#include "gloox/tag.h"
#include <algorithm>

class User;

using namespace gloox;

typedef enum { 	SUBSCRIPTION_NONE = 0,
				SUBSCRIPTION_TO,
				SUBSCRIPTION_FROM,
				SUBSCRIPTION_BOTH,
				SUBSCRIPTION_ASK
				} SpectrumSubscriptionType;

// Wrapper for PurpleBuddy
class AbstractSpectrumBuddy {
	public:
		AbstractSpectrumBuddy(long id);
		~AbstractSpectrumBuddy();
		
		long getId();
		void setId(long id);
		
		std::string getSafeName();
		std::string getJid();
		
		Tag *generatePresenceStanza(int features);
		
		bool isOnline();
		void setOnline();
		void setOffline();
		
		void setSubscription(const std::string &subscription);
		const std::string &getSubscription();
		
		virtual PurpleBuddy *getBuddy() = 0;
		virtual std::string getName() = 0;
		virtual std::string getAlias() = 0;
		virtual bool getStatus(int &status, std::string &statusMessage) = 0;
		virtual std::string getIconHash() = 0;

	private:
		long m_id;
		bool m_online;
		std::string m_subscription;
};

#endif