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

#include <pcrecpp.h>
#include "sqlparser.h"

namespace utils{
	SqlParser::SqlParser() :skip_int_(0),limit_int_(0){}

	SqlParser::~SqlParser(){}

	void SqlParser::Clear(){
		skip_int_ = 0;
		limit_int_ = 0;
		query_type_ = "";
		raw_sql_ = "";
		table_ = "";
		fields_ = "";
		condition_ = "";
		limit_ = "";
		orderby_ = "";
		groupby_ = "";
		error_desc_ = "";
		db_name_ = "";

		find_command_ = "";
		mongo_count_ = "";
		mongo_where_ = "";
		mongo_distinct_ = "";
		mongo_field_ = "";
		mongo_skip_ = "";
		mongo_limit_ = "";
		mongo_orderby_ = "";
		mongo_groupby_ = "";
		fields_vec_.clear();
		mongo_statement_ = "";
		null_string = "{}";
		indexes_.clear();
	}
	bool SqlParser::Parse(const std::string &sql){
		Clear();

		std::string result = sql;
		static pcrecpp::RE e("\r|\n|\t");
		e.GlobalReplace(" ", &result);
		static pcrecpp::RE e1("; *$");
		e1.GlobalReplace("", &result);

		bool ret = false;
		do {
			//get query type
			std::string type1, type2;
			static pcrecpp::RE rgx_type("^(\\w+) +(\\w+|.*).*");
			if (!rgx_type.FullMatch(result, &type1, &type2)){
				error_desc_ = "Query type not found, support SELECT|CREATE TABLE|DROP DATABASE";
				ret = false;
				break;
			}
			utils::String::ToUpper(type1);
			utils::String::ToUpper(type2);
			if (type1 == "SELECT"){
				query_type_ = type1;
				ret = ParseSelect(result);
			}
			else if (type1 == "DELETE"){
				query_type_ = type1;
				ret = ParseDelete(result);
			}
			else if (type1 == "UPDATE"){
				query_type_ = type1;
				ret = ParseUpdate(result);
			}
			else if (type1 == "INSERT"){
				query_type_ = type1;
				ret = ParseInsert(result);
			}
			else if (type1 == "CREATE" && type2 == "TABLE"){
				query_type_ = "CREATE_TABLE";
				ret = ParseCreateTable(result);
			}
			else if (type1 == "CREATE" && type2 == "DATABASE"){
				//query_type_ = "CREATE_TABLE";
				//ret = ParseCreate(result);
			}
			else if (type1 == "DROP" && type2 == "DATABASE"){
				query_type_ = "DROP_DATABASE";
				ret = ParseDropDatabase(result);
			}
			else if (type1 == "CREATE" && type2 == "DATABASE"){
				query_type_ = "CREATE_DATABASE";
				ret = ParseCreateDatabase(result);
			}
			else{
				ret = false;
				error_desc_ = "Query type only support SELECT|CREATE TABLE|DROP DATABASE";
			}

		} while (false);
		return ret;
	}

	bool SqlParser::ParseInsert(std::string &result){
		do {
			static pcrecpp::RE rgx1("INSERT +?INTO +?(\\w+) *?\\((.+?)\\) *?VALUES *?\\((.+?)\\)$", pcrecpp::RE_Options().set_caseless(true));
			//get table name
			std::string str_field, str_value;
			if (rgx1.FullMatch(result, &table_, &str_field, &str_value)){
				if (!ParseTable(table_)){
					error_desc_ = "Multi table not support";
					break;
				}
			}
			else{
				error_desc_ = "Table name not found";
				break;
			}

			//get all field
			utils::String::Trim(str_field);
			utils::String::Trim(str_value);
			utils::StringVector vec;
			do {
				static pcrecpp::RE rgx2("(\\w+?)(?: *, *|$)", pcrecpp::RE_Options().set_caseless(true));
				std::string field1;
				pcrecpp::StringPiece input(str_field);
				while (rgx2.Consume(&input, &field1)){
					vec.push_back(field1);
				}
			} while (false);

			//get all value
			utils::StringVector vec1;
			do {
				static pcrecpp::RE rgx_value("(.+?)(?: *, *|$)", pcrecpp::RE_Options().set_caseless(true));
				std::string value;
				pcrecpp::StringPiece input(str_value);
				while (rgx_value.Consume(&input, &value)){
					static pcrecpp::RE rer("(^['\"]|['\"]$)");
					rer.GlobalReplace("\"", &value);
					vec1.push_back(value);
				}
			} while (false);

			if (vec.size() == 0 || vec.size() != vec1.size()){
				error_desc_ = utils::String::Format("Field's size(" FMT_SIZE ") is not equal to value's size(" FMT_SIZE ") or size is zero", 
					vec.size(), vec1.size());
				return false;
			}

			mongo_field_ = "{";
			for (size_t i = 0; i < vec.size(); i++){
				if (i == 0){
					mongo_field_ += ("\"" + vec[i] + "\":" + vec1[i]);
				}
				else{
					mongo_field_ += (",\"" + vec[i] + "\":" + vec1[i]);
				}
			}
			mongo_field_ += "}";

			return true;
		} while (false);

		return false;
	}

	bool SqlParser::ParseUpdate(std::string &result){
		do {
			static pcrecpp::RE rgx1("UPDATE +?(\\w+) +?SET (.*)", pcrecpp::RE_Options().set_caseless(true));
			//get table name
			std::string str_field;
			if (rgx1.FullMatch(result, &table_, &str_field)){
				if (!ParseTable(table_)){
					error_desc_ = "Multi table not support";
					break;
				}
			}
			else{
				error_desc_ = "Table name not found";
				break;
			}

			//get all field
			static pcrecpp::RE rgx2("(\\w+?) *= *(\\S+?)(?: *, *|$)", pcrecpp::RE_Options().set_caseless(true));
			std::string field1, field2;
			pcrecpp::StringPiece input(str_field);
			while (rgx2.Consume(&input, &field1, &field2)){
				utils::String::Trim(field2);
				static pcrecpp::RE rer("(^['\"]|['\"]$)");
				rer.GlobalReplace("\"", &field2);
				if (mongo_field_.empty()){
					mongo_field_ += ("{\"" + field1 + "\":" + field2);
				}
				else{
					mongo_field_ += (",\"" + field1 + "\":" + field2);
				}
			}
			if (!mongo_field_.empty()) mongo_field_ += "}";

			//get condition
			static pcrecpp::RE rgx3("UPDATE.*?SET.*?WHERE(.*?)(LIMIT.*|$)", pcrecpp::RE_Options().set_caseless(true));
			rgx3.FullMatch(result, &condition_);

			//get limit
			static pcrecpp::RE rgx4("UPDATE.*?SET.*?(WHERE|GROUP.*?BY|ORDER.*?BY|.*?).*?LIMIT (.*?)$.*", pcrecpp::RE_Options().set_caseless(true));
			std::string s1;
			rgx4.FullMatch(result, &s1, &limit_);

			//parse where
			if (!condition_.empty()){
				mongo_where_ += ParseWhere(condition_);
			}

			//parse limit
			if (!limit_.empty() && !ParseLimit(limit_, mongo_limit_)){
				break;
			}

			return true;
		} while (false);

		return false;
	}

	bool SqlParser::ParseDelete(std::string &result){
		do {
			static pcrecpp::RE rgx1("DELETE +?FROM *(\\w+) ($|WHERE.*)", pcrecpp::RE_Options().set_caseless(true));
			//get table name
			if (rgx1.FullMatch(result, &table_)){
				if (!ParseTable(table_)){
					error_desc_ = "Multi table not support";
					break;
				}
			}
			else{
				error_desc_ = "Table name not found";
				break;
			}

			//get condition
			static pcrecpp::RE rgx3("DELETE.*?FROM.*?WHERE(.*?)(LIMIT.*|$)", pcrecpp::RE_Options().set_caseless(true));
			rgx3.FullMatch(result, &condition_);

			//get limit
			static pcrecpp::RE rgx4("SELECT.*?FROM.*?(WHERE|GROUP.*?BY|ORDER.*?BY|.*?).*?LIMIT (.*?)$.*", pcrecpp::RE_Options().set_caseless(true));
			std::string s1;
			rgx4.FullMatch(result, &s1, &limit_);

			//parse where
			if (!condition_.empty()){
				mongo_where_ += ParseWhere(condition_);
			}

			//parse limit
			if (!limit_.empty() && !ParseLimit(limit_, mongo_limit_)){
				break;
			}

			return true;
		} while (false);

		return false;
	}

	bool SqlParser::ParseSelect(std::string &result){
		do {
			//get fields
			static pcrecpp::RE rgx1("SELECT +(.*?) +FROM.*", pcrecpp::RE_Options().set_caseless(true));
			if (!rgx1.FullMatch(result, &fields_)){
				error_desc_ = "Filed not found";
				break;
			}

			//get table name
			static pcrecpp::RE rgx2("SELECT.*?FROM *(.*?) *($|WHERE|GROUP.*?BY|order.*?BY|LIMIT|$).*", pcrecpp::RE_Options().set_caseless(true));
			if (rgx2.FullMatch(result, &table_)){
				if (!ParseTable(table_)){
					error_desc_ = "Multi table not support";
					break;
				}
			}
			else{
				error_desc_ = "Table name not found";
				break;
			}

			//get condition
			static pcrecpp::RE rgx3("SELECT.*?FROM.*?WHERE(.*?)(GROUP.*?BY|ORDER.*?BY|LIMIT|$).*", pcrecpp::RE_Options().set_caseless(true));
			rgx3.FullMatch(result, &condition_);

			//get limit
			static pcrecpp::RE rgx4("SELECT.*?FROM.*?(WHERE|GROUP.*?BY|ORDER.*?BY|.*?).*?LIMIT (.*?)$.*", pcrecpp::RE_Options().set_caseless(true));
			std::string s1;
			rgx4.FullMatch(result, &s1, &limit_);

			//get order by
			static pcrecpp::RE rgx5("SELECT.*?FROM.*?ORDER.*?BY(.*?)(LIMIT|$).*", pcrecpp::RE_Options().set_caseless(true));
			rgx5.FullMatch(result, &orderby_);

			//get condition
			static pcrecpp::RE rgx6("SELECT.*?FROM.*?GROUP *?BY(.+?)$", pcrecpp::RE_Options().set_caseless(true));
			rgx6.FullMatch(result, &groupby_);

			if (!ParseField(fields_)){
				break;
			}

			//parse where
			if (!condition_.empty()){
				mongo_where_ += ParseWhere(condition_);
			}
			
			//parse goup by
			if (!groupby_.empty()){
				ParseGroupBy(groupby_);
			}
			else if (query_type_ == "GROUP_BY"){
				mongo_groupby_ = "{\"_id\":null," + mongo_groupby_ + "}";
			}

			//parse orderby
			if (!orderby_.empty() && !ParseOrderBy(orderby_, mongo_orderby_)){
				break;
			}

			//parse limit
			if (!limit_.empty() && !ParseLimit(limit_, mongo_limit_)){
				break;
			}

			mongo_statement_ = "db." + table_ + "." +
				find_command_ + "( " + mongo_where_ + mongo_field_ + " )" +
				mongo_distinct_ +
				mongo_count_ +
				mongo_skip_ +
				mongo_orderby_ +
				mongo_limit_;

			return true;
		} while (false);
		return false;
	}

	bool SqlParser::ParseCreateTable(std::string &result){

		/*"CREATE TABLE ledger("
		"ledger_seq		  INT PRIMARY   KEY NOT NULL,"
			"hash			  VARCHAR(70)   NOT NULL DEFAULT '', "
			"phash			  VARCHAR(70)   NOT NULL DEFAULT '',"
			"txhash			  VARCHAR(70)   NOT NULL DEFAULT '',"
			"account_hash	  VARCHAR(70)   NOT NULL DEFAULT '',"
			"total_coins	  BIGINT        NOT NULL DEFAULT 0,"
			"close_time		  BIGINT        NOT NULL DEFAULT 0,"
			"consensus_value  VARCHAR(1024) NOT NULL DEFAULT '',"
			"base_fee		  INT           NOT NULL DEFAULT 0,"
			"base_reserve	  INT           NOT NULL DEFAULT 0,"
			"ledger_version	  INT           NOT NULL DEFAULT 1,"
			"tx_count         BIGINT        NOT NULL DEFAULT 1,"
			"state            INT    NOT NULL DEFAULT 0"
			");"
			"CREATE INDEX index_ledger_hash ON ledger(hash);"
			"CREATE INDEX index_ledger_closetime ON ledger(close_time);"
			"CREATE INDEX index_ledger_state ON ledger(state);"
			*/

		do {
			//get table name
			static pcrecpp::RE rgx2("CREATE +?TABLE *(\\w+).*", pcrecpp::RE_Options().set_caseless(true));
			if (rgx2.FullMatch(result, &table_)){
				if (!ParseTable(table_)){
					error_desc_ = "Multi table not support";
					break;
				}
			}
			else{
				error_desc_ = "Table name not found";
				break;
			}

			//get index
			static pcrecpp::RE rgx3(".*?; *?CREATE +?INDEX +?(\\w+?) +?ON +?(\\w+?)\\((.+?)\\)", pcrecpp::RE_Options().set_caseless(true));
			std::string rename_field;
			std::string table;
			std::string field;
			pcrecpp::StringPiece input(result);
			while (rgx3.Consume(&input, &rename_field, &table, &field)){

				std::string f1;
				utils::StringVector vec;
				utils::String::Strtok(field, vec, ",", -1, true);
				for (size_t i = 0; i < vec.size(); i++){
					if (f1.empty()){
						f1 = ("{\"" + vec[i]+"\":1");
					}
					else{
						f1 += (",\"" + vec[i] + "\":1");
					}
				}

				if (!f1.empty()){
					f1 += "}";
				}

				std::string f2 = "{\"name\":\"" + rename_field + "\"}";
				indexes_[f1] = f2;
			}

			//primary key
			static pcrecpp::RE rgx4(".*PRIMARY +?KEY *?\\((\\w+)\\).*", pcrecpp::RE_Options().set_caseless(true));
			std::string primary_key;
			if (rgx4.FullMatch(result, &primary_key)){
				rename_field = "primary_key_" + primary_key;
				std::string f1 = "{\"" + primary_key + "\":1}";
				std::string f2 = "{\"name\":\"" + rename_field + "\", \"primary_key\":true}";
				indexes_[f1] = f2;
			}

			return true;
		} while (false);

		return false;
	}

	bool SqlParser::ParseDropDatabase(std::string &result){
		do {
			//get table name
			static pcrecpp::RE rgx1("DROP +?DATABASE *(IF EXISTS| +) *(\\w+)$", pcrecpp::RE_Options().set_caseless(true));
			std::string other;
			if (rgx1.FullMatch(result, &other, &db_name_)){
			}
			else{
				error_desc_ = "Parse DROP DATABASE failed";
				break;
			}

			return true;
		} while (false);

		return false;
	}

	bool SqlParser::ParseCreateDatabase(std::string &result){
		do {
			//get table name
			static pcrecpp::RE rgx1("CREATE +?DATABASE +(\\w+)$", pcrecpp::RE_Options().set_caseless(true));
			if (rgx1.FullMatch(result, &db_name_)){
			}
			else{
				error_desc_ = "Parse CREATE DATABASE failed";
				break;
			}

			return true;
		} while (false);

		return false;
	}

	bool SqlParser::ParseField(const std::string &fields) {
		//split field
		utils::StringList vec;
		std::string mongo_field_item;
		utils::String::Strtok(fields, vec, ",", -1, true);
		for (utils::StringList::iterator iter = vec.begin(); iter != vec.end(); iter++){
			std::string field = *iter;

			//if find count
			utils::String::Trim(field);
			static pcrecpp::RE rgx1("(\\w+) *?\\((\\w+?|\\*)\\) +?AS (\\w+?)", pcrecpp::RE_Options().set_caseless(true));
			static pcrecpp::RE rgx2("(\\w+) *?\\((\\w+?|\\*)\\)", pcrecpp::RE_Options().set_caseless(true));

			std::string field1, field2,field3;
			if (rgx1.FullMatch(field, &field1, &field2, &field3) ||
				rgx2.FullMatch(field, &field1, &field2)){
				utils::String::ToUpper(field1);
				if (field3.empty()) field3 =  field1 + "_" + (field2 == "*" ? "ALL" : field2);
				field3 = "\"" + field3 + "\"";
				if (field1 == "COUNT"){
					if (mongo_field_item.empty()) mongo_field_item = field3 + ":" + "{\"$sum\":1}";
					else mongo_field_item += "," + field3 + ":" + "{\"$sum\":1}";
				}
				else if (field1 == "MAX"){
					if (mongo_field_item.empty()) mongo_field_item = field3 + ":" + "{\"$max\":\"$" + field2+"\"}";
					else mongo_field_item += "," + field3 + ":" + "{\"$max\":\"$" + field2 + "\"}";
				}
				else if (field1 == "MIN"){
					if (mongo_field_item.empty()) mongo_field_item = field3 + ":" + "{\"$min\":\"$" + field2 + "\"}";
					else mongo_field_item += "," + field3 + ":" + "{\"$min\":\"$" + field2 + "\"}";
				}
				else if (field1 == "AVG"){
					if (mongo_field_item.empty()) mongo_field_item = field3 + ":" + "{\"$avg\":\"$" + field2 + "\"}";
					else mongo_field_item += "," + field3 + ":" + "{\"$min\":\"$" + field2 + "\"}";
				}
				
				query_type_ = "GROUP_BY";
			}
			else{
				fields_vec_.push_back(utils::String::Trim(field));
			}
		}

		if (query_type_ == "GROUP_BY"){
			mongo_groupby_ = mongo_field_item;
		} 

		if (fields_vec_.size() > 0 && fields_vec_[0] != "*"){
			mongo_field_ = "{";
			for (size_t i = 0; i < fields_vec_.size(); i++){
				if (i == 0){
					mongo_field_ += utils::String::Format("\"%s\":1", fields_vec_[i].c_str());
				}
				else{
					mongo_field_ += utils::String::Format(",\"%s\":1", fields_vec_[i].c_str());
				}
			}
			mongo_field_ += "}";
		}

		return true;
	}

	bool SqlParser::ParseTable(const std::string &table) {
		utils::StringList list_table;
		utils::String::Strtok(table, list_table, ",", -1, true);
		if (list_table.size() > 1){
			return false;
		}

		return true;
	}

	std::string SqlParser::ParseWhere(const std::string &sql_where) {
		//std::regex e(" *?(() *?| *?()) *?| +(AND) +| +(OR) +| +(NOT) +|(NOT).*");
		std::string mutable_where = sql_where;
		utils::String::Trim(mutable_where);
		static pcrecpp::RE re_where("^WHERE|1=1");
		re_where.GlobalReplace("", &mutable_where);

		static pcrecpp::RE re("(.+?)($|\\(+?|\\)+?| +?AND +?| +?OR +?| +?NOT +?|$)", pcrecpp::RE_Options().set_caseless(true));
		pcrecpp::RE_Options opt;
		std::string field;
		std::string oper;
		utils::StringVector vec;
		pcrecpp::StringPiece input(mutable_where);
		while (re.Consume(&input, &field, &oper)){
			utils::String::Trim(field);
			utils::String::Trim(oper);
			if (!field.empty()){
				vec.push_back(field);
			}
			if (!oper.empty()){
				vec.push_back(oper);
			}
		}

		utils::StringVector stack, polish;
		std::string str_null;

		for (size_t i = 0; i < vec.size(); i++){
			std::string condition_item = vec[i];
			utils::String::Trim(condition_item);

			std::string condition_comp = condition_item;
			utils::String::ToUpper(condition_comp);

				if (condition_comp == "NOT"){
				stack.push_back(condition_item);
			}
				else if (condition_comp == "OR"){
				while (stack.size() > 0 && stack.back() == "AND"){
					polish.push_back(stack.back());
					stack.pop_back();
				}
				stack.push_back(condition_item);
			}
				else if (condition_comp == "AND"){
				stack.push_back(condition_item);
			}
				else if (condition_comp == "("){
				stack.push_back(condition_item);
			}
				else if (condition_comp == ")"){
				while (stack.size() > 0 && stack.back() != "("){
					polish.push_back(stack.back());
					stack.pop_back();
				}
				str_null = stack.back();
				stack.pop_back();
			}
			else{
				polish.push_back(condition_item);
				while (stack.size() > 0 && stack.back() == "NOT"){
					polish.push_back(stack.back());
					stack.pop_back();
				}
			}
		}

		//empty stack
		while (stack.size() > 0){
			polish.push_back(stack.back());
			stack.pop_back();
		}

		//foreach
		//#foreach($polish as $key) { echo $key . " "; }
		//#echo "<br/>";

		//polish stuff to mongo
		utils::StringVector tmpval;
		size_t cnt = 0;
		std::string nextoper;
		size_t popcount = 2;
		utils::StringVector tmparr;
		std::string rs;
		std::string e1;
		for (size_t i = 0; i < polish.size(); i++){
			const std::string &item = polish[i];
			std::string item_comp = item;
			utils::String::ToUpper(item_comp);
			cnt++;
			if (item_comp == "OR" || item_comp == "AND" || item_comp == "NOT"){
				std::string mgoper;
				if (item_comp == "OR"){
					mgoper = "$or";
				}
				else if (item_comp == "AND"){
					mgoper = "$and";
				}
				else if (item_comp == "NOT"){
					mgoper = "not";
					popcount = 1;
				}

				//if (item == polish[cnt] && popcount > 1 ){
				//	popcount++;
				//	//#echo "same oper $val\n";
				//	continue;
				//} 

				utils::StringVector tmparr2;
				for (size_t m = 0; m < popcount; m++){
					tmparr2.push_back(tmparr.back());
					tmparr.pop_back();
				}

				//join
				std::string opestring;
				for (utils::StringVector::reverse_iterator iter = tmparr2.rbegin(); iter != tmparr2.rend(); iter++){
					if (opestring.empty()){
						opestring = *iter;
					}
					else{
						opestring += utils::String::Format(", %s", iter->c_str());
					}
				}

				if (popcount == 1){
					e1 = opestring;
					//### Rewrite 'not' stuff
					if (item_comp == "NOT"){
						//# fx { b : { $ne : 5 } } = { b : 5 }
						static pcrecpp::RE rgx("{ (\\w+) : ([\\w'\"]+) }");
						rgx.GlobalReplace("{ \"$1\" : { \"$ne\" : $2 } }", &e1);
						//# fx:  { a: 5 } = { a: { $ne: 5 } };
						//### if no change
						if (e1 == opestring){
							static pcrecpp::RE rgx1("{ (\\w+) : { $ne : ([\\w'\"]+) } }");
							rgx1.GlobalReplace("{ \"$1\" : $2 }", &e1);
						}
					}
					else{
						e1 = " { \"" + mgoper + "\" : " + opestring + " } ..";
					}
					tmparr.push_back(e1);
					popcount = 2;
					continue;
				}
				else{
					e1 = " { \"" + mgoper + "\" : [ " + opestring + " ] } ";
				}

				popcount = 2;
				if (polish.size() > cnt){
					tmparr.push_back(e1);
				}
				else{
					rs += e1;
				}
			}
			else{
				std::string output;
				if (Equation2Mg(item, output)){
					tmparr.push_back(output);
				}
			}
		}

		for (size_t m = 0; m < tmparr.size(); m++){
			rs += tmparr[m];
		}

		return rs;
	}

	bool SqlParser::Equation2Mg(const std::string &item, std::string &equaltion){
		std::string val = item;
		utils::String::Trim(val);
		if (val.empty()){
			return true;
		}

		//check for normal
		static pcrecpp::RE rgx("(\\w+).*?([<>=!]+)(.*).*", pcrecpp::RE_Options().set_caseless(true));
		static pcrecpp::RE rgx1("([^ ]+) +?(is +?null|is +?not +?null|like) *?(.*)", pcrecpp::RE_Options().set_caseless(true));
		std::string ope;
		std::string str1, str2, str3;
		utils::StringVector matches;
		if (rgx.PartialMatch(val, &str1, &str2, &str3)){
			matches.push_back(val);
			matches.push_back(str1);
			matches.push_back(str2);
			matches.push_back(str3);
			ope = str2;
		}
		else if (rgx1.FullMatch(val, &str1, &str2, &str3)){ //### check for is null and such
			matches.push_back(val);
			matches.push_back(str1);
			matches.push_back(str2);
			matches.push_back(str3);
			ope = str2;
		}
		else{
			return false;
		}

		std::string mg_equation;
		utils::String::Trim(matches[3]);
		static pcrecpp::RE re_match1("^'|'$");
		re_match1.GlobalReplace("\"", &matches[3]);
		if (ope == "="){
			mg_equation = "{ \"" + matches[1] + "\" : " + matches[3] + " }";
		}
		else if (ope == "<"){
			mg_equation = "{ \"" + matches[1] + "\": { \"$lt\" : " + matches[3] + " } }";
		}
		else if (ope == ">"){
			mg_equation = "{ \"" + matches[1] + "\": { \"$gt\" : " + matches[3] + " } }";
		}
		else if (ope == "<="){
			mg_equation = "{ \"" + matches[1] + "\": { \"$lte\" : " + matches[3] + " } }";
		}
		else if (ope == ">="){
			mg_equation = "{ \"" + matches[1] + "\": { \"$gte\" : " + matches[3] + " } }";
		}
		else if (ope == "IS NULL"){
			mg_equation = "{ \"" + matches[1] + "\":null}";
		}
		else if (ope == "!="){
			mg_equation = "{ \"" + matches[1] + "\": { \"$ne\" : " + matches[3] + " } }";
		}
		else if (ope == "IS NOT NULL"){
			mg_equation = "{ \"" + matches[1] + "\" : { \"$ne\" : null } }";
		}
		else if (ope == "like"){
			std::string a = matches[1];
			std::string b = matches[3];
			pcrecpp::RE rgx2("[%_]");
			if (!rgx2.FullMatch(b)){
				mg_equation = "{ \"" + a + "\" : " + b + " }";
			}
			else{
				utils::String::Trim(b);
				static pcrecpp::RE rer("(^['\"]|['\"]$)");
				rer.GlobalReplace("", &b); //# 'text' -> text  or "text" -> text
				if (pcrecpp::RE("^%").FullMatch(b)) { b = "^" + b; } //# handles like 'text%' -> /^text/
				if (pcrecpp::RE("%$").FullMatch(b)) { b += "$"; }//# handles like '%text' -> /^text$/
				pcrecpp::RE("%").GlobalReplace(".*", &b);
				pcrecpp::RE("_").GlobalReplace(".", &b);
				mg_equation = "{\"" + a + "\" : " + b + "}";

			}
			//mg_equation = "{ " + matches[1] + " : { '$ne' : null } }";
		}
		else{
			return false;
		}

		equaltion = mg_equation;
		return true;
	}

	bool SqlParser::ParseOrderBy(const std::string &item, std::string &mongo_sort){
		utils::StringVector vec;
		std::string mutable_item = item;
		utils::String::Trim(mutable_item);
		static pcrecpp::RE rgx2("(\\w+?) +?(\\w+?)(?: *, *|$)", pcrecpp::RE_Options().set_caseless(true));
		std::string field1, field2;
		pcrecpp::StringPiece input(mutable_item);
		while (rgx2.Consume(&input, &field1, &field2)){
			utils::String::Trim(field2);
			utils::String::ToUpper(field2);

			if (mongo_sort.empty()){
				mongo_sort += "{";
			}
			else{
				mongo_sort += ",";
			}

			mongo_sort += ("\"" + field1 + "\":");

			if (field2 == "ASC"){
				mongo_sort += "1";
			}
			else if (field2 == "DESC"){
				mongo_sort += "-1";
			}
			else{
				error_desc_ = utils::String::Format("DESC or ASC missing (%s)", item.c_str());
				return false;
			}
		}

		if (!mongo_sort.empty()){
			mongo_sort += "}";
		}

		return true;
	}

	bool SqlParser::ParseGroupBy(const std::string &groupby){
		std::string val = groupby;
		utils::String::Trim(val);
		utils::StringVector vec;
		utils::String::Strtok(val, vec, ",", -1, true);

		if (vec.size() < 1){
			return false;
		}

		std::string groupby1 = "\"_id\": {";
		for (size_t i = 0; i < vec.size(); i++){
			const std::string &field = vec[i];
			if (i== 0){
				groupby1 += ("\"" + field + "\":\"$" + field + "\"");
			}
			else{
				groupby1 += (",\"" + field + "\":\"$" + field + "\"");
			}
		}

		groupby1 += "}";

		mongo_groupby_ = "{" + groupby1 + "," + mongo_groupby_ + "}";

		return true;

	}

	bool SqlParser::ParseLimit(const std::string &item, std::string &mongo_limit){
		std::string val = item;
		utils::String::Trim(val);
		utils::StringVector vec;
		utils::String::Strtok(item, vec, ",", -1, true);

		size_t limitcnt = 0;
		std::string rowstofind;
		std::string skip_value;
		for (size_t i = 0; i < vec.size(); i++){
			std::string value = utils::String::Trim(vec[i]);
			limitcnt++;
			if (limitcnt == 2){
				mongo_skip_ = ".skip(" + value + ")";
				mongo_limit = ".limit(" + skip_value + ")";
				rowstofind = skip_value;

				skip_int_ = utils::String::Stoui(value);
				limit_int_ = utils::String::Stoui(skip_value);
			}
			else{
				mongo_limit = ".limit(" + value + ")";
				skip_value = value;
				rowstofind = skip_value;
				skip_int_ = 0;
				limit_int_ = utils::String::Stoui(value);
			}
		}

		if (rowstofind == "1"){
			find_command_ = "findOne";
		}
		else{
			find_command_ = "find";
		}

		return true;
	}

	const std::string &SqlParser::mg_statement() const{
		return mongo_statement_;
	}

	const std::string &SqlParser::error_desc() const{
		return error_desc_;
	}

	const std::string &SqlParser::mg_field() const{
		if (mongo_field_.empty()){
			return null_string;
		}
		return mongo_field_;
	}

	const std::string &SqlParser::mg_groupby() const{
		if (mongo_groupby_.empty()){
			return null_string;
		}
		return mongo_groupby_;
	}
	
	const std::string &SqlParser::mg_condition() const{
		if (mongo_where_.empty()){
			return null_string;
		} 
		return mongo_where_;
	}

	const std::string &SqlParser::mg_table() const{
		return table_;
	}

	uint32_t SqlParser::limit() const{
		return limit_int_;
	}

	const std::string &SqlParser::mg_orderby() const{
		if (mongo_orderby_.empty()){
			return null_string;
		}
		return mongo_orderby_;
	}

	uint32_t SqlParser::skip() const{
		return skip_int_;
	}

	const std::string &SqlParser::query_type() const{
		return query_type_;
	}

	const utils::StringMap &SqlParser::indexes() const{
		return indexes_;
	}

	const std::string &SqlParser::db_name() const{
		return db_name_;
	}
}