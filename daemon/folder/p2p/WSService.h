/* Copyright (C) 2015 Alexander Shishenko <GamePad64@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <util/Loggable.h>
#include "pch.h"
#pragma once
#include "websocket_config.h"
#include "P2PProvider.h"

namespace librevault {

class P2PFolder;

class Client;

class WSService : public Loggable {
public:
	WSService(Client& client, P2PProvider& provider);

	struct connection {
		// Set immediately after creation
		websocketpp::connection_hdl connection_handle;
		enum role_type {SERVER, CLIENT} role;

		// Set on_tcp_post_init
		blob remote_pubkey;
		tcp_endpoint remote_endpoint;

		// Needs to be set before on_validate and on_open
		blob hash;

		// Set on_open
		std::weak_ptr<P2PFolder> folder;
	};

	/* Actions */
	virtual void send_message(websocketpp::connection_hdl hdl, const blob& message) = 0;

protected:
	struct connection_error : public std::runtime_error {
		connection_error(const char* what) : std::runtime_error(what) {}
	};

	P2PProvider& provider_;
	Client& client_;

	std::map<websocketpp::connection_hdl, connection, std::owner_less<websocketpp::connection_hdl>> ws_assignment_;

	static const char* subprotocol_;

	/* TLS functions */
	std::shared_ptr<ssl_context> make_ssl_ctx();
	blob pubkey_from_cert(X509* x509);

	/* Handlers */
	void on_tcp_pre_init(websocketpp::connection_hdl hdl, connection::role_type role);
	virtual void on_tcp_post_init(websocketpp::connection_hdl hdl) = 0;
	std::shared_ptr<ssl_context> on_tls_init(websocketpp::connection_hdl hdl);
	bool on_tls_verify(websocketpp::connection_hdl hdl, bool preverified, boost::asio::ssl::verify_context& ctx);    // Not WebSockets callback, but asio::ssl
	void on_open(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, const std::string& message);
	void on_disconnect(websocketpp::connection_hdl hdl);

	/* Handler templates */
	template<class WSClass> void on_tcp_post_init(WSClass& c, websocketpp::connection_hdl hdl);

	/* Actions */
	virtual void close(websocketpp::connection_hdl hdl, const std::string& reason) = 0;
	template<class WSClass> void close(WSClass& c, websocketpp::connection_hdl hdl, const std::string& reason) {
		log_->trace() << log_tag() << BOOST_CURRENT_FUNCTION;
		c.get_con_from_hdl(hdl)->close(websocketpp::close::status::internal_endpoint_error, reason);
	}
	//virtual void terminate(websocketpp::connection_hdl hdl);
	virtual std::string errmsg(websocketpp::connection_hdl hdl) = 0;
};

} /* namespace librevault */