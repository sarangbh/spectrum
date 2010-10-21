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

#include "spectrumcomponent.h"

#include <iostream>
#include "transport.h"
#include "autoconnectloop.h"
#include "usermanager.h"
#include <boost/bind.hpp>
#include "user.h"

using namespace Swift;
using namespace boost;

SpectrumComponent::SpectrumComponent() : m_timerFactory(&m_boostIOServiceThread.getIOService()) {
	m_reconnectCount = 0;
	m_firstConnection = true;

	m_reconnectTimer = m_timerFactory.createTimer(1000);
	m_reconnectTimer->onTick.connect(bind(&SpectrumComponent::connect, this)); 

	m_component = new Component(Swift::JID(CONFIG().jid), CONFIG().password);
	m_component->onConnected.connect(bind(&SpectrumComponent::handleConnected, this));
	m_component->onError.connect(bind(&SpectrumComponent::handleConnectionError, this, _1));
	m_component->onDataRead.connect(bind(&SpectrumComponent::handleDataRead, this, _1));
	m_component->onDataWritten.connect(bind(&SpectrumComponent::handleDataWritten, this, _1));
	m_component->onPresenceReceived.connect(bind(&SpectrumComponent::handlePresenceReceived, this, _1));

	m_capsMemoryStorage = new CapsMemoryStorage();
	m_capsManager = new CapsManager(m_capsMemoryStorage, m_component->getStanzaChannel(), m_component->getIQRouter());
	m_entityCapsManager = new EntityCapsManager(m_capsManager, m_component->getStanzaChannel());
	m_entityCapsManager->onCapsChanged.connect(boost::bind(&SpectrumComponent::handleCapsChanged, this, _1));
}

SpectrumComponent::~SpectrumComponent() {
	delete m_component;
}

void SpectrumComponent::connect() {
	m_reconnectCount++;
	if (Transport::instance()->sql()->loaded()) {
		m_component->connect(CONFIG().server, CONFIG().port);
		m_reconnectTimer->stop();
	}
	else {
		Log("SpectrumComponent", "Tried to reconnect, but database is not ready yet.");
		Log("SpectrumComponent", "trying to reconnect after 1 second");
		m_reconnectTimer->start();
	}
}

void SpectrumComponent::handleConnected() {
	Log("SpectrumComponent", "CONNECTED!");
	m_reconnectCount = 0;

	if (m_firstConnection) {
// 		j->disco()->setIdentity("gateway", protocol()->gatewayIdentity(), configuration().discoName);
// 		m_discoHandler->setIdentity("client", "pc", "Spectrum");
// 		j->disco()->setVersion(configuration().discoName, VERSION, "");
// 
// 		std::string id = "client";
// 		id += '/';
// 		id += "pc";
// 		id += '/';
// 		id += '/';
// 		id += "Spectrum";
// 		id += '<';
// 		
// 		std::list<std::string> features = protocol()->transportFeatures();
// 		features.sort();
// 		for (std::list<std::string>::iterator it = features.begin(); it != features.end(); it++) {
// 			// these features are default in gloox
// 			if (*it != "http://jabber.org/protocol/disco#items" &&
// 				*it != "http://jabber.org/protocol/disco#info" &&
// 				*it != "http://jabber.org/protocol/commands") {
// 				j->disco()->addFeature(*it);
// 			}
// 		}
// 
// 		std::list<std::string> f = protocol()->buddyFeatures();
// 		if (find(f.begin(), f.end(), "http://jabber.org/protocol/disco#items") == f.end())
// 			f.push_back("http://jabber.org/protocol/disco#items");
// 		if (find(f.begin(), f.end(), "http://jabber.org/protocol/disco#info") == f.end())
// 			f.push_back("http://jabber.org/protocol/disco#info");
// 		f.sort();
// 		for (std::list<std::string>::iterator it = f.begin(); it != f.end(); it++) {
// 			id += (*it);
// 			id += '<';
// 			m_discoHandler->addFeature(*it);
// 		}
// 
// 		SHA sha;
// 		sha.feed( id );
// 		m_configuration.hash = Base64::encode64( sha.binary() );
// 		j->disco()->registerNodeHandler( m_spectrumNodeHandler, "http://spectrum.im/transport#" + m_configuration.hash );
// 		m_discoHandler->registerNodeHandler( m_spectrumNodeHandler, "http://spectrum.im/transport#" + m_configuration.hash );
// 
// 		if (CONFIG().protocol != "irc")
// 			new AutoConnectLoop();
		m_firstConnection = false;
// 		purple_timeout_add_seconds(60, &sendPing, this);
	}
}

void SpectrumComponent::handleConnectionError(const ComponentError &error) {
	Log("SpectrumComponent", "Disconnected from Jabber server!");
	switch (error.getType()) {
		case ComponentError::UnknownError: Log("SpectrumComponent", "Disconnect reason: UnknownError"); break;
		case ComponentError::ConnectionError: Log("SpectrumComponent", "Disconnect reason: ConnectionError"); break;
		case ComponentError::ConnectionReadError: Log("SpectrumComponent", "Disconnect reason: ConnectionReadError"); break;
		case ComponentError::ConnectionWriteError: Log("SpectrumComponent", "Disconnect reason: ConnectionWriteError"); break;
		case ComponentError::XMLError: Log("SpectrumComponent", "Disconnect reason: XMLError"); break;
		case ComponentError::AuthenticationFailedError: Log("SpectrumComponent", "Disconnect reason: AuthenticationFailedError"); break;
		case ComponentError::UnexpectedElementError: Log("SpectrumComponent", "Disconnect reason: UnexpectedElementError"); break; 
	};

	if (m_reconnectCount == 2)
		Transport::instance()->userManager()->removeAllUsers();

	Log("SpectrumComponent", "Trying to reconnect after 1 second");
	m_reconnectTimer->start();
}

void SpectrumComponent::handlePresenceReceived(Swift::Presence::ref presence) {
	switch(presence->getType()) {
		case Swift::Presence::Subscribe:
		case Swift::Presence::Subscribed:
		case Swift::Presence::Unsubscribe:
		case Swift::Presence::Unsubscribed:
			handleSubscription(presence);
			break;
		case Swift::Presence::Available:
		case Swift::Presence::Unavailable:
			handlePresence(presence);
			break;
		case Swift::Presence::Probe:
			handleProbePresence(presence);
			break;
		default:
			break;
	};
}

void SpectrumComponent::handlePresence(Swift::Presence::ref presence) {
	bool isMUC = presence->getPayload<MUCPayload>() != NULL;

	// filter out login/logout presence spam
	if (!presence->getTo().getNode().isEmpty() && isMUC == false)
		return;

	// filter out bad presences
	if (!isValidNode(presence->getFrom().getDomain().getUTF8String())) {
		Swift::Presence::ref response = Swift::Presence::create();
		response->setTo(presence->getFrom());
		response->setFrom(presence->getTo());
		response->setType(Swift::Presence::Error);

		response->addPayload(boost::shared_ptr<Payload>(new ErrorPayload(ErrorPayload::JIDMalformed, ErrorPayload::Modify)));

		m_component->sendPresence(response);
		return;
	}

	Log(presence->getFrom().toString().getUTF8String(), "Presence received (" << (int) presence->getShow() << ") for: " << presence->getTo().toString().getUTF8String() << " - isMUC = " << isMUC);

	// check if we have this client's capabilities and ask for them
	bool haveFeatures = false;
	boost::shared_ptr<CapsInfo> capsInfo = presence->getPayload<CapsInfo>();
	if (capsInfo) {
		haveFeatures = m_entityCapsManager->getCaps(presence->getFrom()) != DiscoInfo::ref();
		std::cout << "has capsInfo " << haveFeatures << "\n";
	}
	else {
		GetDiscoInfoRequest::ref discoInfoRequest = GetDiscoInfoRequest::create(presence->getFrom(), m_component->getIQRouter());
		discoInfoRequest->onResponse.connect(boost::bind(&SpectrumComponent::handleDiscoInfoResponse, this, _1, _2, presence->getFrom()));
		discoInfoRequest->send();
	}
	
	AbstractUser *user;
	std::string barejid = presence->getTo().toBare().toString().getUTF8String();
	std::string userkey = presence->getFrom().toBare().toString().getUTF8String();
	if (Transport::instance()->protocol()->tempAccountsAllowed()) {
		std::string server = barejid.substr(barejid.find("%") + 1, barejid.length() - barejid.find("%"));
		userkey += server;
	}
	
	user = Transport::instance()->userManager()->getUserByJID(userkey);

	if (user == NULL) {
		// No user, unavailable presence... nothing to do
			if (presence->getType() == Swift::Presence::Unavailable)
				return;
			UserRow res = Transport::instance()->sql()->getUserByJid(userkey);
			if (res.id == -1 && !Transport::instance()->protocol()->tempAccountsAllowed()) {
				// presence from unregistered user
				Log(presence->getFrom().toString().getUTF8String(), "This user is not registered");
				return;
			}
			else {
				if (res.id == -1 && Transport::instance()->protocol()->tempAccountsAllowed()) {
					res.jid = userkey;
					res.uin = presence->getFrom().toBare().toString().getUTF8String();
					res.password = "";
					res.language = "en";
					res.encoding = CONFIG().encoding;
					res.vip = 0;
					Transport::instance()->sql()->addUser(res);
					res = Transport::instance()->sql()->getUserByJid(userkey);
				}

				bool isVip = res.vip;
				std::list<std::string> const &x = CONFIG().allowedServers;
				if (CONFIG().onlyForVIP && !isVip && std::find(x.begin(), x.end(), presence->getFrom().getDomain().getUTF8String()) == x.end()) {
					Log(presence->getFrom().toString().getUTF8String(), "This user is not VIP, can't login...");
					return;
				}

				Log(presence->getFrom().toString().getUTF8String(), "Creating new User instance");

				if (Transport::instance()->protocol()->tempAccountsAllowed()) {
					std::string server = barejid.substr(barejid.find("%") + 1, barejid.length() - barejid.find("%"));
					res.uin = presence->getTo().getResource().getUTF8String() + "@" + server;
				}
				else {
					if (purple_accounts_find(res.uin.c_str(), Transport::instance()->protocol()->protocol().c_str()) != NULL) {
						PurpleAccount *act = purple_accounts_find(res.uin.c_str(), Transport::instance()->protocol()->protocol().c_str());
						user = Transport::instance()->userManager()->getUserByAccount(act);
						if (user) {
							Log(presence->getFrom().toString().getUTF8String(), "This account is already connected by another jid " << user->jid());
							return;
						}
					}
				}
				user = (AbstractUser *) new User(res, userkey);
				user->setFeatures(isVip ? CONFIG().VIPFeatures : CONFIG().transportFeatures);
// 				if (c != NULL)
// 					if (Transport::instance()->hasClientCapabilities(c->findAttribute("ver")))
// 						user->setResource(stanza.from().resource(), stanza.priority(), Transport::instance()->getCapabilities(c->findAttribute("ver")));
// 
				Transport::instance()->userManager()->addUser(user);
// 				user->receivedPresence(stanza);
// 				if (protocol()->tempAccountsAllowed()) {
// 					std::string server = stanza.to().username().substr(stanza.to().username().find("%") + 1, stanza.to().username().length() - stanza.to().username().find("%"));
// 					server = stanza.from().bare() + server;
// 					purple_timeout_add_seconds(15, &connectUser, g_strdup(server.c_str()));
// 				}
// 				else
// 					purple_timeout_add_seconds(15, &connectUser, g_strdup(stanza.from().bare().c_str()));
// 			}
		}
// 		if (stanza.presence() == Presence::Unavailable && stanza.to().username() == ""){
// 			Log(stanza.from().full(), "User is already logged out => sending unavailable presence");
// 			Tag *tag = new Tag("presence");
// 			tag->addAttribute( "to", stanza.from().bare() );
// 			tag->addAttribute( "type", "unavailable" );
// 			tag->addAttribute( "from", jid() );
// 			j->send( tag );
// 		}
	}
	else {
// 		user->receivedPresence(stanza);
	}
// 	if (stanza.to().username() == "" && user != NULL) {
// 		if(stanza.presence() == Presence::Unavailable && user->isConnected() == true && user->getResources().empty()) {
// 			Log(stanza.from().full(), "Logging out");
// 			m_userManager->removeUser(user);
// 		}
// 		else if (stanza.presence() == Presence::Unavailable && user->isConnected() == false && user->getResources().empty()) {
// 			Log(stanza.from().full(), "Logging out, but he's not connected...");
// 			m_userManager->removeUser(user);
// 		}
// // 		else if (stanza.presence() == Presence::Unavailable && user->isConnected() == false) {
// // 			Log(stanza.from().full(), "Can't logout because we're connecting now...");
// // 		}
// 	}
// 	else if (user != NULL && stanza.presence() == Presence::Unavailable && m_protocol->tempAccountsAllowed() && !user->hasOpenedMUC()) {
// 		m_userManager->removeUser(user);
// 	}
// 	else if (user == NULL && stanza.to().username() == "" && stanza.presence() == Presence::Unavailable) {
// 		UserRow res = sql()->getUserByJid(userkey);
// 		if (res.id != -1) {
// 			sql()->setUserOnline(res.id, false);
// 		}
// 	}
}

void SpectrumComponent::handleSubscription(Swift::Presence::ref presence) {
	
}

void SpectrumComponent::handleProbePresence(Swift::Presence::ref presence) {
	
}

void SpectrumComponent::handleDataRead(const String &data) {
	Log("XML IN", data);
}

void SpectrumComponent::handleDataWritten(const String &data) {
	Log("XML OUT", data);
}

void SpectrumComponent::handleDiscoInfoResponse(boost::shared_ptr<DiscoInfo> discoInfo, const boost::optional<ErrorPayload>& error, const Swift::JID& jid) {
	AbstractUser *user = Transport::instance()->userManager()->getUserByJID(jid.toBare().toString().getUTF8String());

	std::string resource = jid.getResource().getUTF8String();
	if (user && user->hasResource(resource)) {
		if (user->getResource(resource).caps == 0) {
			int capabilities = 0;

			for (std::vector< String >::const_iterator it = discoInfo->getFeatures().begin(); it != discoInfo->getFeatures().end(); ++it) {
				if (*it == "http://jabber.org/protocol/rosterx") {
					capabilities |= CLIENT_FEATURE_ROSTERX;
				}
				else if (*it == "http://jabber.org/protocol/xhtml-im") {
					capabilities |= CLIENT_FEATURE_XHTML_IM;
				}
				else if (*it == "http://jabber.org/protocol/si/profile/file-transfer") {
					capabilities |= CLIENT_FEATURE_FILETRANSFER;
				}
				else if (*it == "http://jabber.org/protocol/chatstates") {
					capabilities |= CLIENT_FEATURE_CHATSTATES;
				}
			}

			user->setResource(resource, -256, capabilities);
			if (user->readyForConnect()) {
				user->connect();
			}
		}
	}
}

void SpectrumComponent::handleCapsChanged(const Swift::JID& jid) {
	DiscoInfo::ref discoInfo = m_entityCapsManager->getCaps(jid);
	handleDiscoInfoResponse(discoInfo, NULL, jid);
}
