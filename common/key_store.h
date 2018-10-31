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

#ifndef KEYSTORE_H_
#define KEYSTORE_H_

namespace phantom {
	class KeyStore {
	public:
		KeyStore();
		~KeyStore();

		bool Generate(const std::string &password, Json::Value &key_store, std::string &new_priv_key);  //If new_private_key is empty, create a new private key.
		bool From(const Json::Value &key_store, const std::string &password, std::string &priv_key);
	};
}

#endif
