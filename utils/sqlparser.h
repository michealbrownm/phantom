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

#ifndef TEST_SQLPARSER_H_
#define TEST_SQLPARSER_H_

#include "strings.h"

namespace utils{
	class SqlParser{
		std::string query_type_;
		std::string raw_sql_;
		std::string table_;
		std::string fields_;
		std::string condition_;
		std::string limit_;
		std::string orderby_;
		std::string groupby_;
		//utils::StringMap fields_;
		std::string error_desc_;
		std::string db_name_;

		std::string find_command_;
		std::string mongo_count_;
		std::string mongo_where_;
		std::string mongo_distinct_;
		std::string mongo_field_;
		std::string mongo_skip_;
		std::string mongo_limit_;
		std::string mongo_orderby_;
		std::string mongo_groupby_;
		utils::StringVector fields_vec_;
		std::string mongo_statement_;
		uint32_t limit_int_;
		uint32_t skip_int_;

		utils::StringMap indexes_;
		std::string null_string;

		void Clear();
	public:
		SqlParser();
		~SqlParser();

	public:
		bool Parse(const std::string &sql);
		bool ParseCreateTable(std::string &result);
		bool ParseCreateDatabase(std::string &result);
		bool ParseDropDatabase(std::string &result);
		bool ParseDelete(std::string &result);
		bool ParseSelect(std::string &result);
		bool ParseUpdate(std::string &result);
		bool ParseInsert(std::string &result);

		bool ParseGroupBy(const std::string &groupby);
		bool ParseField(const std::string &field);
		bool ParseTable(const std::string &table);
		std::string  ParseWhere(const std::string &sql_where);
		bool Equation2Mg(const std::string &item, std::string &equaltion);
		bool ParseOrderBy(const std::string &item, std::string &mongo_sort);
		bool ParseLimit(const std::string &item, std::string &mongo_limit);
		const std::string &mg_statement() const;
		const std::string &error_desc() const;
		const std::string &mg_field() const;
		const std::string &mg_groupby() const;
		const std::string &mg_condition() const;
		const std::string &mg_table() const;
		const std::string &mg_orderby() const;
		uint32_t limit() const;
		uint32_t skip() const;
		const std::string &query_type() const;
		const utils::StringMap &indexes() const;
		const std::string &db_name() const;
	};
}
#endif