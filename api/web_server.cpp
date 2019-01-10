/*
	phantom is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	phantom is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with phantom.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <utils/headers.h>
#include <common/general.h>
#include <common/private_key.h>
#include <main/configure.h>
#include <glue/glue_manager.h>
#include <ledger/transaction_frm.h>
#include <ledger/ledger_manager.h>
#include "websocket_server.h"
#include "web_server.h"

namespace phantom {

	WebServer::WebServer() :
		async_io_ptr_(NULL),
		server_ptr_(NULL),
		context_(NULL),
		running(NULL),
		thread_count_(0),
        port_(0)
	{
	}

	WebServer::~WebServer() {
	}

	bool WebServer::Initialize(WebServerConfigure &webserver_config) {

		if (webserver_config.listen_addresses_.size() == 0) {
			LOG_INFO("Listen address not set, ignore");
			return true;
		}

		if (webserver_config.ssl_enable_) {
			std::string strHome = utils::File::GetBinHome();
			context_ = new asio::ssl::context(asio::ssl::context::tlsv12);
			context_->set_options(
				asio::ssl::context::default_workarounds
				| asio::ssl::context::no_sslv2
				| asio::ssl::context::single_dh_use);
			context_->set_password_callback(std::bind(&WebServer::GetCertPassword, this, std::placeholders::_1, std::placeholders::_2));
			context_->use_certificate_chain_file(utils::String::Format("%s/%s", strHome.c_str(), webserver_config.ssl_configure_.chain_file_.c_str()));
			asio::error_code ignore_code;
			context_->use_private_key_file(utils::String::Format("%s/%s", strHome.c_str(), webserver_config.ssl_configure_.private_key_file_.c_str()),
				asio::ssl::context::pem,
				ignore_code);
			context_->use_tmp_dh_file(utils::String::Format("%s/%s", strHome.c_str(), webserver_config.ssl_configure_.dhparam_file_.c_str()));
		}

		thread_count_ = webserver_config.thread_count_;
		if (thread_count_ == 0) {
			thread_count_ = utils::System::GetCpuCoreCount() * 4;
		}

		utils::InetAddress address = webserver_config.listen_addresses_.front();
		server_ptr_ = new http::server::server(address.ToIp(), address.GetPort(), context_, thread_count_);
        port_ =server_ptr_->GetServerPort();


		server_ptr_->SetHome(utils::File::GetBinHome() + "/" + webserver_config.directory_);

		server_ptr_->add404(std::bind(&WebServer::FileNotFound, this, std::placeholders::_1, std::placeholders::_2));

		server_ptr_->addRoute("hello", std::bind(&WebServer::Hello, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("createAccount", std::bind(&WebServer::CreateAccount, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getAccount", std::bind(&WebServer::GetAccount, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getAccountBase", std::bind(&WebServer::GetAccountBase, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getGenesisAccount", std::bind(&WebServer::GetGenesisAccount, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getAccountMetaData", std::bind(&WebServer::GetAccountMetaData, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getAccountAssets", std::bind(&WebServer::GetAccountAssets, this, std::placeholders::_1, std::placeholders::_2));

		server_ptr_->addRoute("debug", std::bind(&WebServer::Debug, this, std::placeholders::_1, std::placeholders::_2));


		server_ptr_->addRoute("getTransactionBlob", std::bind(&WebServer::GetTransactionBlob, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getTransactionHistory", std::bind(&WebServer::GetTransactionHistory, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getTransactionCache", std::bind(&WebServer::GetTransactionCache, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getContractTx", std::bind(&WebServer::GetContractTx, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getStatus", std::bind(&WebServer::GetStatus, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getLedger", std::bind(&WebServer::GetLedger, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getModulesStatus", std::bind(&WebServer::GetModulesStatus, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getConsensusInfo", std::bind(&WebServer::GetConsensusInfo, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("updateLogLevel", std::bind(&WebServer::UpdateLogLevel, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getAddress", std::bind(&WebServer::GetAddress, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getTransactionFromBlob", std::bind(&WebServer::GetTransactionFromBlob, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getPeerNodeAddress", std::bind(&WebServer::GetPeerNodeAddress, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getLedgerValidators", std::bind(&WebServer::GetLedgerValidators, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("getPeerAddresses", std::bind(&WebServer::GetPeerAddresses, this, std::placeholders::_1, std::placeholders::_2));
		
		server_ptr_->addRoute("multiQuery", std::bind(&WebServer::MultiQuery, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("submitTransaction", std::bind(&WebServer::SubmitTransaction, this, std::placeholders::_1, std::placeholders::_2));
		//server_ptr_->addRoute("confValidator", std::bind(&WebServer::ConfValidator, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("contractQuery", std::bind(&WebServer::ContractQuery, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("testContract", std::bind(&WebServer::TestContract, this, std::placeholders::_1, std::placeholders::_2));
		server_ptr_->addRoute("testTransaction", std::bind(&WebServer::TestTransaction, this, std::placeholders::_1, std::placeholders::_2));

		server_ptr_->Run();
		running = true;

		StatusModule::RegisterModule(this);
		LOG_INFO("Webserver started, thread count(" FMT_SIZE ") listen at %s", thread_count_, address.ToIpPort().c_str());
		return true;
	}

	bool WebServer::Exit() {
		LOG_INFO("WebServer stoping...");
		running = false;
		if (server_ptr_) {
			server_ptr_->Stop();
			delete server_ptr_;
			server_ptr_ = NULL;
		}

		if (context_) {
			delete context_;
			context_ = NULL;
		}
		LOG_INFO("WebServer stop [OK]");
		return true;
	}

	std::string WebServer::GetCertPassword(std::size_t, asio::ssl::context_base::password_purpose purpose) {
		return phantom::Configure::Instance().webserver_configure_.ssl_configure_.private_password_;
	}

	void WebServer::FileNotFound(const http::server::request &request, std::string &reply) {
		reply = "File not found";
	}

	void WebServer::Hello(const http::server::request &request, std::string &reply) {
		Json::Value reply_json = Json::Value(Json::objectValue);
		reply_json["PHANTOM_VERSION"] = General::PHANTOM_VERSION;
		reply_json["ledger_version"] = utils::String::ToString(General::LEDGER_VERSION);
		reply_json["overlay_version"] = utils::String::ToString(General::OVERLAY_VERSION);
		reply_json["current_time"] = utils::Timestamp::Now().ToFormatString(true);
		reply_json["websocket_address"] =  WebSocketServer::Instance().GetListenPort();
		reply_json["hash_type"] = Configure::Instance().ledger_configure_.hash_type_;
		reply = reply_json.toFastString();
	}

	void WebServer::MultiQuery(const http::server::request &request, std::string &reply){
		WebServerConfigure &web_config = Configure::Instance().webserver_configure_;
		Json::Value reply_json = Json::Value(Json::objectValue);
		Json::Value &results = reply_json["results"];

		do {
			Json::Value req;
			if (!req.fromString(request.body)) {
				LOG_ERROR("Parse request body json failed");
				reply_json["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
				break;
			}

			const Json::Value &items = req["items"];

			if (items.size() > web_config.multiquery_limit_){
				LOG_ERROR("MultiQuery size is too larger than %u", web_config.multiquery_limit_);
				reply_json["error_code"] = protocol::ERRCODE_INVALID_PARAMETER;
				break;
			}

			for (uint32_t i = 0; i < items.size(); i++){
				const Json::Value &item = items[i];
				Json::Value &result = results[i];
				std::string url = item["url"].asString();
				std::string method = item["method"].asString();

				http::server::request request_inner;
				if (item.isMember("jsonData"))
				{
					const Json::Value &nRequestJsonData = item["jsonData"];
					if (nRequestJsonData.isString())
					{
						request_inner.body = nRequestJsonData.asString();
					}
					else
					{
						request_inner.body = nRequestJsonData.toFastString();
					}
				}

				std::string reply_inner;
				request_inner.uri = url;
				request_inner.method = method;
				request_inner.Update();

				http::server::server::routeHandler *handle = server_ptr_->getRoute(request_inner.command);
				if (handle) {
					(*handle)(request_inner, reply_inner);
				}

				result.fromString(reply_inner);
			}

			reply_json["error_code"] = 0;
		} while (false);

		reply = reply_json.toStyledString();
	}

	void WebServer::GetModuleStatus(Json::Value &data) {
		data["name"] = "web server";
		data["context"] = (context_ != NULL);
		data["start_request_count"] = server_ptr_->start_count_;
		data["end_request_count"] = server_ptr_->end_count_;
		data["expire_request_count"] = server_ptr_->expire_count_;
		data["thread_count"] = (Json::Int64)thread_count_;
	}

	uint16_t WebServer::GetListenPort(){
        return port_;
    }
}