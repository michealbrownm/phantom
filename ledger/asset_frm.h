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

#ifndef ASSET_FRM_H
#define ASSET_FRM_H
#include <unordered_map>
#include <utils/common.h>
#include <common/general.h>
#include <common/private_key.h>
namespace bubi {
	class AssetFrm {
	public:
		struct Sel //start ext length
		{
			Sel(int64_t s, int64_t l, const std::string &e) {
				start_ = s;
				length_ = l;
				ext_ = e;
			}
			std::string ext_;
			int64_t start_;
			int64_t length_;
			bool operator < (const Sel &r) const {
				if (length_ < 0) {
					return false;
				}
				else if (length_ == 0) {
					if (r.length_ < 0) {
						return true;
					}
					else if (r.length_ == 0) {
						return ext_ < r.ext_;
					}
					else if (r.length_ > 0) {
						return false;
					}
				}
				else if (length_ > 0) {
					if (r.length_ < 0) {
						return true;
					}
					else if (r.length_ == 0) {
						return true;
					}
					else if (r.length_ > 0) {
						int64_t a = length_ + start_ - r.length_ - r.start_;
						if (a == 0) {
							if (start_ == r.start_) {
								return ext_ < r.ext_;
							}
							else {
								return start_ > r.start_;
							}
						}
						else if (a < 0) {
							return true;
						}
						else if (a > 0) {
							return false;
						}
					}
				}
				return false;
			}
		};

		static bool IsValid(const protocol::Asset &asset) {
			int64_t namount = 0;
			std::set<Sel> tset;

			do {
				if (asset.amount() <= 0) {
					return false;
				}
				if (asset.property().type() != protocol::AssetProperty_Type_IOU) {
					break;
				}

				if (!bubi::PublicKey::IsAddressValid(asset.property().issuer())) {
					break;
				}
				if (asset.property().code().length() == 0 ||
					asset.property().code().length() > 64) {
					break;
				}
				return true;
			} while (false);

			return false;
		}


		static void ToJson(const protocol::Asset &asset, Json::Value  &json) {
			const protocol::AssetProperty pro = asset.property();
			json["type"] = pro.type();
			json["issuer"] = pro.issuer();
			json["code"] = pro.code();
			json["amount"] = asset.amount();
		}


		static bool FromJson(const Json::Value &js, protocol::Asset *asset) {

			if (!js.isMember("code") ||
				!js.isMember("issuer") ||
				!js.isMember("amount")) {
				return false;
			}

			asset->set_amount(js["amount"].asInt64());
			asset->mutable_property()->set_type(protocol::AssetProperty_Type_IOU);
			asset->mutable_property()->set_issuer(js["issuer"].asString());
			asset->mutable_property()->set_code(js["code"].asString());
			return true;
		}
	};
};

#endif