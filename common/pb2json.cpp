/*

Copyright (c) 2013, EMC Corporation (Isilon Division)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

-- Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

-- Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

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

#ifndef PB2JSON_H
#define PB2JSON_H

#include "pb2json.h"

#include <sstream>
#include <stdexcept>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/repeated_field.h>
#include <utils/strings.h>

using google::protobuf::Message;
using google::protobuf::MessageFactory;
using google::protobuf::Descriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::EnumDescriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::Reflection;

namespace  phantom {
	class j2pb_error : public std::exception {
		std::string _error;
	public:
		j2pb_error(const std::string &e) : _error(e) {}
		j2pb_error(const FieldDescriptor *field, const std::string &e) : _error(field->name() + ": " + e) {}
		virtual ~j2pb_error() throw() {};

		virtual const char *what() const throw () { return _error.c_str(); };
		static std::string error_msg(const FieldDescriptor *field, const std::string &e){
			return field->name() + ": " + e;
		}
	};

	static Json::Value  _field2json(const Message& msg, const FieldDescriptor *field, size_t index) {
		const google::protobuf::Message::Reflection *ref = msg.GetReflection();
		const bool repeated = field->is_repeated();
		Json::Value jf;
		switch (field->cpp_type()) {

			case FieldDescriptor::CPPTYPE_BOOL:
			{
				jf = repeated ? ref->GetRepeatedBool(msg, field, index) : ref->GetBool(msg, field);
				break;
			}
			case FieldDescriptor::CPPTYPE_DOUBLE:
			{
				jf = repeated ? ref->GetRepeatedDouble(msg, field, index) : ref->GetDouble(msg, field);
				break;
			}
			case FieldDescriptor::CPPTYPE_INT32:
			{
				jf = repeated ? ref->GetRepeatedInt32(msg, field, index) : ref->GetInt32(msg, field);
				break;
			}
			case FieldDescriptor::CPPTYPE_UINT32:
			{
				jf = repeated ? ref->GetRepeatedUInt32(msg, field, index) : ref->GetUInt32(msg, field);
				break;
			}
			case FieldDescriptor::CPPTYPE_INT64:
			{
				 int64_t n = repeated ? ref->GetRepeatedInt64(msg, field, index) : ref->GetInt64(msg, field);
				// jf = utils::String::Format(FMT_I64, n);
				 jf = n;
				break;
			}
			case FieldDescriptor::CPPTYPE_UINT64:
			{
				uint64_t n = repeated ? ref->GetRepeatedUInt64(msg, field, index) : ref->GetUInt64(msg, field);
				//jf = utils::String::Format(FMT_U64, n);
				jf = n;
				break;
			}
			case FieldDescriptor::CPPTYPE_FLOAT:
			{
				jf = repeated ? ref->GetRepeatedFloat(msg, field, index) : ref->GetFloat(msg, field);
				break;
			}
			case FieldDescriptor::CPPTYPE_STRING: {
				std::string scratch;
				const std::string &v = (repeated) ?
					ref->GetRepeatedStringReference(msg, field, index, &scratch) :
					ref->GetStringReference(msg, field, &scratch);
				if (field->type() == FieldDescriptor::TYPE_BYTES)
					jf = utils::String::BinToHexString(v);
				else
					jf = v;
				break;
			}
			case FieldDescriptor::CPPTYPE_MESSAGE: {
#ifdef GetMessage
#undef GetMessage
				const Message& mf = (repeated) ? ref->GetRepeatedMessage(msg, field, (int)index) : ref->GetMessage(msg, field);
#else
				const Message& mf = (repeated) ? ref->GetRepeatedMessage(msg, field, (int)index) : ref->GetMessage(msg, field);

#endif
				jf = Proto2Json(mf);
				break;
			}
			case FieldDescriptor::CPPTYPE_ENUM: {
				const EnumValueDescriptor* ef = (repeated) ?
					ref->GetRepeatedEnum(msg, field, index) :
					ref->GetEnum(msg, field);
				//jf = ef->name();
				jf = ef->number();
				break;
			}
			default:
				break;
		}

		return jf;
	}
	static bool _json2field(Message &msg, const FieldDescriptor *field, const Json::Value& jf, std::string& errorMsg) {
		const Reflection *ref = msg.GetReflection();
		const bool repeated = field->is_repeated();
		
		switch (field->cpp_type()) {
#define _SET_OR_ADD(sfunc, afunc, value)	\
		do {	\
			if (repeated)	\
				ref->afunc(&msg, field, value);	\
																																																																																else	\
				ref->sfunc(&msg, field, value);	\
											} while (0)	

			case FieldDescriptor::CPPTYPE_DOUBLE:
			{
				_SET_OR_ADD(SetDouble, AddDouble, jf.asDouble());
				break;
			}
			case FieldDescriptor::CPPTYPE_FLOAT:{
				_SET_OR_ADD(SetFloat, AddFloat, jf.asDouble());
				break;
			}
			
			case FieldDescriptor::CPPTYPE_INT64:{
				_SET_OR_ADD(SetInt64, AddInt64, jf.asInt64());
				break;
			}
			case FieldDescriptor::CPPTYPE_UINT64:{
				_SET_OR_ADD(SetUInt64, AddUInt64, jf.asUInt64());
				break;
			}
			case FieldDescriptor::CPPTYPE_INT32:{
				_SET_OR_ADD(SetInt32, AddInt32, jf.asInt());
				break;
			}
			case FieldDescriptor::CPPTYPE_UINT32:{
				_SET_OR_ADD(SetUInt32, AddUInt32, jf.asUInt());
				break;
			}
			case FieldDescriptor::CPPTYPE_BOOL:{
				_SET_OR_ADD(SetBool, AddBool, jf.asBool());
				break;
			}
			case FieldDescriptor::CPPTYPE_STRING: {
				std::string value = jf.asString();
				if (field->type() == FieldDescriptor::TYPE_BYTES)
				{
					std::string strBin;
					if (utils::String::HexStringToBin(value, strBin))
						_SET_OR_ADD(SetString, AddString, strBin);
					else{
						errorMsg = j2pb_error::error_msg(field, "not a valid hex string");
						return false;
					}
				}
				else
					_SET_OR_ADD(SetString, AddString, value);
				break;

			}
			case FieldDescriptor::CPPTYPE_MESSAGE: {
				Message *mf = (repeated) ?
					ref->AddMessage(&msg, field) :
					ref->MutableMessage(&msg, field);
				if (!Json2Proto(jf, *mf, errorMsg)){
					return false;
				}
				break;
			}
			case FieldDescriptor::CPPTYPE_ENUM: {
				const EnumDescriptor *ed = field->enum_type();
				const EnumValueDescriptor *ev = 0;
				if (jf.isInt() || jf.isInt64() || jf.isUInt() || jf.isUInt64()) {
					ev = ed->FindValueByNumber(jf.asInt());
				}
				else if (jf.isString()) {
					ev = ed->FindValueByName(jf.asString());
				}
				else {
					errorMsg = j2pb_error::error_msg(field, "Not an integer or string");

					return false;
				}
				if (!ev) {
					errorMsg = j2pb_error::error_msg(field, "Enum value not found");
					return false;
				}
				_SET_OR_ADD(SetEnum, AddEnum, ev);
				break;
			}
			default:
				break;
		}
		return true;
	}

	Json::Value Proto2Json(const Message& msg) {
		const Descriptor *d = msg.GetDescriptor();
		const Reflection *ref = msg.GetReflection();
		if (!d || !ref)
		{
			throw j2pb_error("Descriptor or Reflection");
		}
			
		Json::Value va;

		std::vector<const FieldDescriptor *> fields;
		ref->ListFields(msg, &fields);
		
		for (size_t i = 0; i != fields.size(); i++) {
			const FieldDescriptor *field = fields[i];
			const std::string &name = (field->is_extension()) ? field->full_name() : field->name();
			
			Json::Value jf;
			if (field->is_repeated()) {
				int count = ref->FieldSize(msg, field);
				//if (!count) continue;
				for (int j = 0; j < count; j++)
					jf[j] = _field2json(msg, field, j);
			}
			else jf = _field2json(msg, field, 0);
			//else if (ref->HasField(msg, field))
			//	jf = _field2json(msg, field, 0);
			//else
			//	continue;

			//const std::string &name = (field->is_extension()) ? field->full_name() : field->name();
			va[name] = jf;

		}
		return va;
	}


	bool Json2Proto(const Json::Value& root, Message& msg, std::string& errorMsg) {

		const Descriptor *descriptor = msg.GetDescriptor();
		const Reflection *ref = msg.GetReflection();
		if (!descriptor || !ref)/* throw j2pb_error("No descriptor or reflection");*/
			return false;

		bool bok = true;
		auto names = root.getMemberNames();
		for (size_t n = 0; n < names.size(); n++) {
			std::string name = names[n];
			const Json::Value& jf = root[name];
			const FieldDescriptor *field = descriptor->FindFieldByName(name);
			if (!field)
				field = ref->FindKnownExtensionByName(name);
			//field = d->file()->FindExtensionByName(name);
			/*throw j2pb_error("Unknown field: " + std::string(name));*/
			if (!field)
				continue;

			int r = 0;
			if (field->is_repeated()) {
				if (!jf.isArray()){
					errorMsg = j2pb_error::error_msg(field, "Not array");
					return false;
				}

				for (size_t j = 0; j < jf.size(); j++){
					if (!_json2field(msg, field, jf[j], errorMsg))
						return false;
				}
			}
			else {
				if (!_json2field(msg, field, jf, errorMsg)) {
					bok = false;
					break;
				}
			}
		}
		return bok;
	}

} // namespace

#endif