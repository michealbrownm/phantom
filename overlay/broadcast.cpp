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

#include <json/value.h>
#include <utils/headers.h>
#include <common/general.h>
#include "broadcast.h"

namespace phantom{
	BroadcastRecord::BroadcastRecord(int64_t type, const std::string &data, int64_t peer_id) {
		type_ = type;
		peers_.insert(peer_id);
		time_stamp_ = utils::Timestamp::HighResolution();
	}

	BroadcastRecord::~BroadcastRecord(){}

	Broadcast::Broadcast(IBroadcastDriver *driver)
		:driver_(driver){}

	Broadcast::~Broadcast(){}

	bool Broadcast::Add(int64_t type, const std::string &data, int64_t peer_id) {
		std::string hash = HashWrapper::Crypto(data);
		utils::MutexGuard guard(mutex_msg_sending_);
		BroadcastRecordMap::iterator result = records_.find(hash);
		if (result == records_.end()){ // we have never seen this message
			BroadcastRecord::pointer record = std::make_shared<BroadcastRecord>(type, data, peer_id);
			records_[hash] = record;
			records_couple_[record->time_stamp_] = hash;
			return true;
		}
		else {
			result->second->peers_.insert(peer_id);
			return false;
		}

		return true;
	}

	void Broadcast::Send(int64_t type, const std::string &data) {
		std::string hash = HashWrapper::Crypto(data);
		utils::MutexGuard guard(mutex_msg_sending_);
		BroadcastRecordMap::iterator result = records_.find(hash);
		if (result == records_.end()){ // no one has sent us this message
			BroadcastRecord::pointer record = std::make_shared<BroadcastRecord>(
				type, data, 0);

			records_[hash] = record;
			records_couple_[record->time_stamp_] = hash;
			std::set<int64_t> peer_ids = driver_->GetActivePeerIds();
			for (const auto peer_id : peer_ids)
			{
				driver_->SendRequest(peer_id, type, data);
				record->peers_.insert(peer_id);
			}
		}
		else{ // send it to people that haven't sent it to us
			std::set<int64_t>& peersTold = result->second->peers_;
			for (const auto peer : driver_->GetActivePeerIds()){
				if (peersTold.find(peer) == peersTold.end())
				{
					driver_->SendRequest(peer, type, data);
					result->second->peers_.insert(peer);
				}
			}
		}
	}

	void Broadcast::OnTimer(){
		utils::MutexGuard guard(mutex_msg_sending_);
		int64_t current_time = utils::Timestamp::HighResolution();

		for (auto it = records_couple_.begin(); it != records_couple_.end();){
			// give one ledger of leeway
			if (it->first + 120 * utils::MICRO_UNITS_PER_SEC < current_time)
			{
				records_.erase(it->second);
				records_couple_.erase(it++);
			}
			else
			{
				break;
			}
		}
	}
}
