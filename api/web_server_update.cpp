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

#include "web_server.h"

namespace phantom {
	void WebServer::UpdateLogLevel(const http::server::request &request, std::string &reply) {
		std::string levelreq = request.GetParamValue("level");
		utils::LogLevel loglevel = utils::LOG_LEVEL_ALL;
		std::string loglevel_info = "LOG_LEVEL_ALL";
		if (levelreq == "1") {
			loglevel = (utils::LogLevel)(utils::LOG_LEVEL_ALL & ~utils::LOG_LEVEL_TRACE);
			loglevel_info = "LOG_LEVEL_ALL & ~utils::LOG_LEVEL_TRACE";
		}

		utils::Logger::Instance().SetLogLevel(loglevel);
		reply = utils::String::Format("set log level to %s", loglevel_info.c_str());
	}
}