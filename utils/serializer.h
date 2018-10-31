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

#ifndef _BUBI_SERIALIZER_H_
#define 	_BUBI_SERIALIZER_H_

#include <vector>
#include <memory>
#include <string>

#include "utils.h"

namespace utils{

class Serializer{
public:
	typedef std::shared_ptr <Serializer> pointer;
	
	Serializer (){}
	~Serializer (){}
	Serializer (std::string str);

	std::vector <char>& peek_data ();
	static uint256 get_prefix_hash (const char *ch, int len);
	bool add_raw (const char* ch, int len);
	bool add256 (uint256 &hash);
	uint256 get_sha512_half ();
	bool	add_serializer (Serializer &s);
	std::size_t	peek_data_size ();
	
private:
	std::vector <char> data_;
};

}

#endif
