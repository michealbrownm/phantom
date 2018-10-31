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

#include "serializer.h"

#include <openssl/sha.h>

namespace utils{


Serializer::Serializer (std::string str){
	std::size_t sz = str.length ();
	for (size_t i = 0; i < sz; i++){
		data_.push_back (str[i]);
	}
}

std::vector <char>&
Serializer::peek_data (){
	return data_;
}

uint256
Serializer::get_prefix_hash (const char* ch, int len){
	uint256 j[2];
	SHA512_CTX  ctx;
	SHA512_Init (&ctx);
	SHA512_Update (&ctx, ch, len);
	SHA512_Final (reinterpret_cast <unsigned char *> (&j[0]), &ctx);
	return j[0];
}

bool
Serializer::add_serializer (Serializer &s){
	std::vector <char>& vt = s.peek_data ();
	add_raw (&(*(vt.begin())), vt.size());
	return true;
}

std::size_t
Serializer::peek_data_size (){
	return data_.size ();
}
bool
Serializer::add_raw (const char *ch, int len){
	for (int i=0; i<len; i++){
		data_.push_back (ch[i]);
	}
	return true;
}

bool
Serializer::add256 (uint256 &hash){
	data_.insert (data_.end(), hash.begin(),hash.end());
	return true;
}

uint256
Serializer::get_sha512_half (){
	uint256 j[2];
	SHA512 ( reinterpret_cast<unsigned char*>(&(data_.front())), data_.size(), reinterpret_cast <unsigned char *>(&j[0]) );
	return j[0];
}

}
