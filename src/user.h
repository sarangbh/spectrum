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

#ifndef _HI_USER_H
#define _HI_USER_H

#include <time.h>
#include <gloox/clientbase.h>
#include <gloox/messagesession.h>
#include <gloox/siprofileft.h>
#include <glib.h>
#include "purple.h"
#include "abstractuser.h"
#include "account.h"
class RosterManager;
#include "rostermanager.h"
#include "rosterstorage.h"
#include "spectrummessagehandler.h"

class GlooxMessageHandler;
class FiletransferRepeater;

class RosterRow;

using namespace gloox;

typedef enum { 	CALLER_ADHOC,
				CALLER_SEARCH
				} AdhocDataCallerType;

struct AdhocData {
	std::string id;
	std::string from;
	std::string node;
	AdhocDataCallerType callerType;
};

struct authData{
	PurpleAccount *account;
	std::string who;
};

struct authRequest {
	PurpleAccountRequestAuthorizationCb authorize_cb;
	PurpleAccountRequestAuthorizationCb deny_cb;
	void *user_data;
};

struct subscribeContact {
	std::string uin;
	std::string alias;
	std::string	group;
};

class User;

class User : public AbstractUser, public RosterManager, public RosterStorage, public SpectrumMessageHandler {
	public:
		User(GlooxMessageHandler *parent, JID jid, const std::string &username, const std::string &password, const std::string &userKey, long id);
		~User();

		void connect();
		bool hasTransportFeature(int feature); // TODO: move me to p->hasTransportFeature and rewrite my API

		// Libpurple stuff
		void purpleReauthorizeBuddy(PurpleBuddy *buddy);

		// Gloox callbacks
		void receivedPresence(const Presence &presence);
		void receivedSubscription(const Subscription &subscription);
		void receivedChatState(const std::string &uin, const std::string &state);

		// Libpurple callbacks
		void purpleBuddyRemoved(PurpleBuddy *buddy);
		void purpleBuddyCreated(PurpleBuddy *buddy);
		void purpleAuthorizeReceived(PurpleAccount *account,const char *remote_user,const char *id,const char *alias,const char *message,gboolean on_list,PurpleAccountRequestAuthorizationCb authorize_cb,PurpleAccountRequestAuthorizationCb deny_cb,void *user_data);
		void purpleBuddyTypingStopped(const std::string &uin);
		void purpleBuddyTyping(const std::string &uin);
		void connected();
		void disconnected();

		// Filetransfers
		void addFiletransfer( const JID& to, const std::string& sid, SIProfileFT::StreamType type, const JID& from, long size );
		void addFiletransfer( const JID& from );
		FiletransferRepeater* removeFiletransfer(const std::string &from) { FiletransferRepeater *repeater = (FiletransferRepeater *) g_hash_table_lookup(m_filetransfers, from.c_str()); if (repeater) g_hash_table_remove(m_filetransfers, from.c_str()); return repeater; }
		FiletransferRepeater* getFiletransfer(const std::string &from) { FiletransferRepeater *repeater = (FiletransferRepeater *) g_hash_table_lookup(m_filetransfers, from.c_str()); return repeater; }
		std::string actionData;

		// Connected
		bool isConnected() { return m_connected; }

		// Settings
		PurpleValue *getSetting(const char *key);
		void updateSetting(const std::string &key, PurpleValue *value);

		// Authorization requests
		bool hasAuthRequest(const std::string &name);
		void removeAuthRequest(const std::string &name);

		// bind IP
		void setBindIP(const std::string& bindIP) { m_bindIP = bindIP; }

		// connection start
		time_t connectionStart() { return m_connectionStart; }

		PurpleAccount *account() { return m_account; }
		int reconnectCount() { return m_reconnectCount; }
		bool isVIP() { return m_vip; }
		bool readyForConnect() { return m_readyForConnect; }
		void setReadyForConnect(bool ready) { m_readyForConnect = ready; }
		const std::string & username() { return m_username; }
		const std::string & jid() { return m_jid; }
		AdhocData & adhocData() { return m_adhocData; }
		void setAdhocData(AdhocData data) { m_adhocData = data; }

		const char *getLang() { return m_lang; }
		void setLang(const char *lang) { m_lang = lang; }
		GHashTable *settings() { return m_settings; }

		GlooxMessageHandler *p;
		const std::string & userKey() { return m_userKey; }
		void setProtocolData(void *protocolData) { m_protocolData = protocolData; }
		void *protocolData() { return m_protocolData; }
		void setFeatures(int f) { m_features = f; }
		int getFeatures() { return m_features; }
		long storageId() { return m_userID; }
		bool loadingBuddiesFromDB() { return m_loadingBuddiesFromDB; }

		guint removeTimer;

	private:
		std::string m_userKey;
		PurpleAccount *m_account;	// PurpleAccount to which this user is connected
		void *m_protocolData;
		guint m_syncTimer;			// timer used for syncing purple buddy list and roster
		int m_subscribeLastCount;	// number of buddies which was in subscribeCache in previous iteration of m_syncTimer
		bool m_vip;					// true if the user is VIP
		bool m_readyForConnect;		// true if the user user wants to connect and we're ready to do it
		bool m_rosterXCalled;		// true if we are counting buddies for roster X
		bool m_connected;			// true if this user is connected to legacy account
		bool m_reconnectCount;		// number of passed reconnect tries
		AdhocData m_adhocData;		// name of node for AdhocHandler
		std::string m_bindIP;		// IP address to which libpurple will be binded
		std::string m_password;		// password used to connect to legacy network
		std::string m_username;		// legacy network user name
		std::string m_jid;			// Jabber ID of this user
		const char *m_lang;			// xml:lang
		int m_features;
		time_t m_connectionStart;	// connection start timestamp
		GHashTable *m_filetransfers;
		std::map<std::string,authRequest> m_authRequests;	// list of authorization requests (holds callbacks and user data)
		GHashTable *m_settings;		// user settings
		long m_userID;				// userID for Database
		bool m_loadingBuddiesFromDB;
};

#endif
