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
#include <common/storage.h>
#include <common/private_key.h>
#include <common/argument.h>
#include <common/daemon.h>
#include <overlay/peer_manager.h>
#include <ledger/ledger_manager.h>
#include <consensus/consensus_manager.h>
#include <glue/glue_manager.h>
#include <api/web_server.h>
#include <api/websocket_server.h>
#include <api/console.h>
#include <ledger/contract_manager.h>
#include <monitor/monitor_manager.h>
#include "configure.h"

void SaveWSPort();
void RunLoop();
int main(int argc, char *argv[]){
	utils::SetExceptionHandle();
	utils::Thread::SetCurrentThreadName("phantom-thread");

	utils::Daemon::InitInstance();
	utils::net::Initialize();
	utils::Timer::InitInstance();
	phantom::Configure::InitInstance();
	phantom::Storage::InitInstance();
	phantom::Global::InitInstance();
	phantom::SlowTimer::InitInstance();
	utils::Logger::InitInstance();
	phantom::Console::InitInstance();
	phantom::PeerManager::InitInstance();
	phantom::LedgerManager::InitInstance();
	phantom::ConsensusManager::InitInstance();
	phantom::GlueManager::InitInstance();
	phantom::WebSocketServer::InitInstance();
	phantom::WebServer::InitInstance();
	phantom::MonitorManager::InitInstance();
	phantom::ContractManager::InitInstance();

	phantom::Argument arg;
	if (arg.Parse(argc, argv)){
		return 1;
	}

	do {
		utils::ObjectExit object_exit;
		phantom::InstallSignal();

		if (arg.console_){
			arg.log_dest_ = utils::LOG_DEST_FILE; //cancel the std output
			phantom::Console &console = phantom::Console::Instance();
			console.Initialize();
			object_exit.Push(std::bind(&phantom::Console::Exit, &console));
		}

		srand((uint32_t)time(NULL));
		phantom::StatusModule::modules_status_ = new Json::Value;
#ifndef OS_MAC
		utils::Daemon &daemon = utils::Daemon::Instance();
		if (!phantom::g_enable_ || !daemon.Initialize((int32_t)1234))
		{
			LOG_STD_ERRNO("Initialize daemon failed", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&utils::Daemon::Exit, &daemon));
#endif

		phantom::Configure &config = phantom::Configure::Instance();
		std::string config_path = phantom::General::CONFIG_FILE;
		if (!utils::File::IsAbsolute(config_path)){
			config_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), config_path.c_str());
		}

		if (!config.Load(config_path)){
			LOG_STD_ERRNO("Load configure failed", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}

		std::string log_path = config.logger_configure_.path_;
		if (!utils::File::IsAbsolute(log_path)){
			log_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), log_path.c_str());
		}
		const phantom::LoggerConfigure &logger_config = phantom::Configure::Instance().logger_configure_;
		utils::Logger &logger = utils::Logger::Instance();
		logger.SetCapacity(logger_config.time_capacity_, logger_config.size_capacity_);
		logger.SetExpireDays(logger_config.expire_days_);
		if (!phantom::g_enable_ || !logger.Initialize((utils::LogDest)(arg.log_dest_ >= 0 ? arg.log_dest_ : logger_config.dest_),
			(utils::LogLevel)logger_config.level_, log_path, true)){
			LOG_STD_ERR("Initialize logger failed");
			break;
		}
		object_exit.Push(std::bind(&utils::Logger::Exit, &logger));
		LOG_INFO("Initialize daemon successful");
		LOG_INFO("Load configure successful");
		LOG_INFO("Initialize logger successful");

		// end run command
		phantom::Storage &storage = phantom::Storage::Instance();
		LOG_INFO("keyvalue(%s),account(%s),ledger(%s)", 
			config.db_configure_.keyvalue_db_path_.c_str(),
			config.db_configure_.account_db_path_.c_str(),
			config.db_configure_.ledger_db_path_.c_str());

		if (!phantom::g_enable_ || !storage.Initialize(config.db_configure_, arg.drop_db_)) {
			LOG_ERROR("Initialize db failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::Storage::Exit, &storage));
		LOG_INFO("Initialize db successful");

		if (arg.drop_db_) {
			LOG_INFO("Drop db successfully");
			return 1;
		} 
		
		if ( arg.clear_consensus_status_ ){
			phantom::Pbft::ClearStatus();
			LOG_INFO("Clear consensus status successfully");
			return 1;
		}

		if (arg.clear_peer_addresses_) {
			phantom::KeyValueDb *db = phantom::Storage::Instance().keyvalue_db();
			db->Put(phantom::General::PEERS_TABLE, "");
			LOG_INFO("Clear peer addresss list successfully");
			return 1;
		} 

		if (arg.create_hardfork_) {
			phantom::LedgerManager &ledgermanger = phantom::LedgerManager::Instance();
			if (!ledgermanger.Initialize()) {
				LOG_ERROR("legder manger init error!!!");
				return -1;
			}
			phantom::LedgerManager::CreateHardforkLedger();
			return 1;
		}

		phantom::Global &global = phantom::Global::Instance();
		if (!phantom::g_enable_ || !global.Initialize()){
			LOG_ERROR_ERRNO("Initialize global variable failed", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&phantom::Global::Exit, &global));
		LOG_INFO("Initialize global variable successful");

		//consensus manager must be initialized before ledger manager and glue manager
		phantom::ConsensusManager &consensus_manager = phantom::ConsensusManager::Instance();
		if (!phantom::g_enable_ || !consensus_manager.Initialize(phantom::Configure::Instance().ledger_configure_.validation_type_)) {
			LOG_ERROR("Initialize consensus manager failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::ConsensusManager::Exit, &consensus_manager));
		LOG_INFO("Initialize consensus manager successful");

		phantom::LedgerManager &ledgermanger = phantom::LedgerManager::Instance();
		if (!phantom::g_enable_ || !ledgermanger.Initialize()) {
			LOG_ERROR("Initialize ledger manager failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::LedgerManager::Exit, &ledgermanger));
		LOG_INFO("Initialize ledger successful");

		phantom::GlueManager &glue = phantom::GlueManager::Instance();
		if (!phantom::g_enable_ || !glue.Initialize()){
			LOG_ERROR("Initialize glue manager failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::GlueManager::Exit, &glue));
		LOG_INFO("Initialize glue manager successful");

		phantom::PeerManager &p2p = phantom::PeerManager::Instance();
		if (!phantom::g_enable_ || !p2p.Initialize(NULL, false)) {
			LOG_ERROR("Initialize peer network failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::PeerManager::Exit, &p2p));
		LOG_INFO("Initialize peer network successful");

		phantom::SlowTimer &slow_timer = phantom::SlowTimer::Instance();
		if (!phantom::g_enable_ || !slow_timer.Initialize(1)){
			LOG_ERROR_ERRNO("Initialize slow timer failed", STD_ERR_CODE, STD_ERR_DESC);
			break;
		}
		object_exit.Push(std::bind(&phantom::SlowTimer::Exit, &slow_timer));
		LOG_INFO("Initialize slow timer with " FMT_SIZE " successful", utils::System::GetCpuCoreCount());

		phantom::WebSocketServer &ws_server = phantom::WebSocketServer::Instance();
		if (!phantom::g_enable_ || !ws_server.Initialize(phantom::Configure::Instance().wsserver_configure_)) {
			LOG_ERROR("Initialize web server failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::WebSocketServer::Exit, &ws_server));
		LOG_INFO("Initialize web server successful");

		phantom::WebServer &web_server = phantom::WebServer::Instance();
		if (!phantom::g_enable_ || !web_server.Initialize(phantom::Configure::Instance().webserver_configure_)) {
			LOG_ERROR("Initialize web server failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::WebServer::Exit, &web_server));
		LOG_INFO("Initialize web server successful");

		SaveWSPort();
		
		phantom::MonitorManager &monitor_manager = phantom::MonitorManager::Instance();
		if (!phantom::g_enable_ || !monitor_manager.Initialize()) {
			LOG_ERROR("Initialize monitor manager failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::MonitorManager::Exit, &monitor_manager));
		LOG_INFO("Initialize monitor manager successful");

		phantom::ContractManager &contract_manager = phantom::ContractManager::Instance();
		if (!contract_manager.Initialize(argc, argv)){
			LOG_ERROR("Initialize contract manager failed");
			break;
		}
		object_exit.Push(std::bind(&phantom::ContractManager::Exit, &contract_manager));
		LOG_INFO("Initialize contract manager successful");

		phantom::g_ready_ = true;

		RunLoop();

		LOG_INFO("Process begin quit...");
		delete phantom::StatusModule::modules_status_;

	} while (false);

	phantom::ContractManager::ExitInstance();
	phantom::SlowTimer::ExitInstance();
	phantom::GlueManager::ExitInstance();
	phantom::LedgerManager::ExitInstance();
	phantom::PeerManager::ExitInstance();
	phantom::WebSocketServer::ExitInstance();
	phantom::WebServer::ExitInstance();
	phantom::MonitorManager::ExitInstance();
	phantom::Configure::ExitInstance();
	phantom::Global::ExitInstance();
	phantom::Storage::ExitInstance();
	utils::Logger::ExitInstance();
	utils::Daemon::ExitInstance();
	
	printf("process exit\n");
}

void RunLoop(){
	int64_t check_module_interval = 5 * utils::MICRO_UNITS_PER_SEC;
	int64_t last_check_module = 0;
	while (phantom::g_enable_){
		int64_t current_time = utils::Timestamp::HighResolution();

		for (auto item : phantom::TimerNotify::notifys_){
			item->TimerWrapper(utils::Timestamp::HighResolution());
			if (item->IsExpire(utils::MICRO_UNITS_PER_SEC)){
				LOG_WARN("The timer(%s) execute time(" FMT_I64 " us) is expire than 1s", item->GetTimerName().c_str(), item->GetLastExecuteTime());
			}
		}

		utils::Timer::Instance().OnTimer(current_time);
		utils::Logger::Instance().CheckExpiredLog();

		if (current_time - last_check_module > check_module_interval){
			utils::WriteLockGuard guard(phantom::StatusModule::status_lock_);
			phantom::StatusModule::GetModulesStatus(*phantom::StatusModule::modules_status_);
			last_check_module = current_time;
		}

		utils::Sleep(1);
	}
}

void SaveWSPort(){    
    std::string tmp_file = utils::File::GetTempDirectory() +"/phantom_listen_port";
	Json::Value json_port = Json::Value(Json::objectValue);
	json_port["webserver_port"] = phantom::WebServer::Instance().GetListenPort();
	json_port["wsserver_port"] = phantom::WebSocketServer::Instance().GetListenPort();
	utils::File file;
	if (file.Open(tmp_file, utils::File::FILE_M_WRITE | utils::File::FILE_M_TEXT))
	{
		std::string line = json_port.toFastString();
		file.Write(line.c_str(), 1, line.length());
		file.Close();
	}
}