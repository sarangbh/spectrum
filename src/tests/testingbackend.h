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

#ifndef TESTING_BACKEND_H
#define TESTING_BACKEND_H
#include <iostream>
#include <string>
#include "gloox/tag.h"

#include "../abstractbackend.h"
#include "../parser.h"
#include "../configfile.h"

struct Buddy {
	long id;
	std::string uin;
	std::string subscription;
	std::string group;
	std::string nickname;
};

class TestingBackend : public AbstractBackend {
	public:
		TestingBackend() { m_pInstance = this; m_parser = new GlooxParser(); reset(); }
		~TestingBackend() {}
		static TestingBackend *instance() { return m_pInstance; }
		void addBuddySetting(long userId, long buddyId, const std::string &key, const std::string &value, PurpleType type) {} 
		long addBuddy(long userId, const std::string &uin, const std::string &subscription, const std::string &group = "Buddies", const std::string &nickname = "", int flags = 0) {
			m_buddies[uin].id = 1;
			m_buddies[uin].uin = uin;
			m_buddies[uin].subscription = subscription;
			m_buddies[uin].group = group;
			m_buddies[uin].nickname = nickname;
			return 1;
		}
		
		GHashTable *getBuddies(long userId, PurpleAccount *account) { return NULL; }
		std::list <std::string> getBuddies(long userId) { std::list<std::string> r; return r; }

		std::map <std::string, Buddy> &getBuddies() { return m_buddies; }
		void reset() {
			m_buddies.clear();
			m_users.clear();
			Configuration cfg;
			cfg.jid_escaping = 1;
			cfg.enable_public_registration = 1;
			m_configuration = cfg;
		}
		
		void updateBuddySubscription(long userId, const std::string &uin, const std::string &subscription) {}
		void removeBuddy(long userId, const std::string &uin, long buddy_id) {}
		
		std::vector<std::string> getOnlineUsers() { return m_onlineUsers; }
		void addOnlineUser(const std::string &jid) { m_onlineUsers.push_back(jid); }
		void clearOnlineUsers() { m_onlineUsers.clear(); }
		void setUserOnline(long userId, bool online) {}

		UserRow getUserByJid(const std::string &jid) {
			UserRow row;
			row.id = -1;
			if (m_users.find(jid) == m_users.end())
				return row;
			else
				return m_users[jid];
		}
		void addUser(const UserRow &user) {
			UserRow row = {1, user.jid, user.uin, user.password, user.language, user.encoding, 0};
			m_users[user.jid] = row;
		}
		void removeUser(long userId) {}
		void updateUser(const UserRow &user) {}
		
		GlooxParser *getParser() { return m_parser; }
		Configuration &getConfiguration() { return m_configuration; }
		void setConfiguration(const Configuration &conf) { m_configuration = conf; }
		std::map<std::string, UserRow> getUsersByJid(const std::string &jid) { std::map<std::string, UserRow> test; return test; }
		void updateSetting(long userId, const std::string &key, const std::string &value) {}

	private:
		Configuration m_configuration;
		std::map <std::string, UserRow> m_users;
		std::map <std::string, Buddy> m_buddies;
		std::vector<std::string> m_onlineUsers;
		static TestingBackend *m_pInstance;
		GlooxParser *m_parser;
};

#endif
