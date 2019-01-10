/*
Copyright Bubi Technologies Co., Ltd. 2017 All Rights Reserved.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

#include <3rd/http/server.hpp>
#include <common/general.h>
#include <common/storage.h>
#include <common/pb2json.h>
#include <proto/cpp/chain.pb.h>
#include <utils/singleton.h>
#include <utils/net.h>
#include <main/configure.h>

namespace phantom {

	class WebServer :public utils::Singleton<phantom::WebServer>, public phantom::StatusModule
	{
		friend class utils::Singleton<phantom::WebServer>;
	public:
		WebServer();
		~WebServer();
	private:
		utils::AsyncIo *async_io_ptr_;
		http::server::server *server_ptr_;
		asio::ssl::context *context_;
		bool running;
		size_t thread_count_;
        unsigned short port_;

		void FileNotFound(const http::server::request &request, std::string &reply);
		void Hello(const http::server::request &request, std::string &reply);
		void CreateAccount(const http::server::request &request, std::string &reply);
		void GetAccountBase(const http::server::request &request, std::string &reply);
		void GetAccount(const http::server::request &request, std::string &reply);
		void GetGenesisAccount(const http::server::request &request, std::string &reply);
		void GetAccountMetaData(const http::server::request &request, std::string &reply);
		void GetAccountAssets(const http::server::request &request, std::string &reply);

		void Debug(const http::server::request &request, std::string &reply);

		void CreateTransaction(const http::server::request &request, std::string &reply);
		void GetTransactionBlob(const http::server::request &request, std::string &reply);
		void UpdateLogLevel(const http::server::request &request, std::string &reply);

		void GetTransactionHistory(const http::server::request &request, std::string &reply);
		void GetTransactionCache(const http::server::request &request, std::string &reply);
		void GetContractTx(const http::server::request &request, std::string &reply);

		//void GetRecord(const http::server::request &request, std::string &reply);
		void GetStatus(const http::server::request &request, std::string &reply);
		void GetModulesStatus(const http::server::request &request, std::string &reply);
		void GetLedger(const http::server::request &request, std::string &reply);
		void GetLedgerValidators(const http::server::request &request, std::string &reply);
		void GetAddress(const http::server::request &request, std::string &reply);
		void GetPeerNodeAddress(const http::server::request &request, std::string &reply);
		void GetTransactionFromBlob(const http::server::request &request, std::string &reply);
		void GetPeerAddresses(const http::server::request &request, std::string &reply);

		void GetConsensusInfo(const http::server::request &request, std::string &reply);

		std::string GetCertPassword(std::size_t, asio::ssl::context_base::password_purpose purpose);

		void MultiQuery(const http::server::request &request, std::string &reply);
		void SubmitTransaction(const http::server::request &request, std::string &reply);

		void ContractQuery(const http::server::request &request, std::string &reply);
		void TestContract(const http::server::request &request, std::string &reply);
		void TestTransaction(const http::server::request &request, std::string &reply);
		bool MakeTransactionHelper(const Json::Value &object, protocol::Transaction *tran, Result& result);

        bool EvaluateFee(protocol::Transaction *tran, Result& result);

	public:
		bool Initialize(WebServerConfigure &webserver_configure);
		bool Exit();
		void GetModuleStatus(Json::Value &data);
		uint16_t GetListenPort();
	};
}

#endif