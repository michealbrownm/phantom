#ifndef EXPRPARSER_H_
#define EXPRPARSER_H_

#include "common.h"

#include <sstream>
#include <ostream>

namespace utils {

	class ExprValue{
	public:
		enum TokenType{
			NONE,
			NAME,
			NUMBER,
			INTEGER64,
			STRING, //add by zhao,
			BOOL, //add by zhao for detect
			UNSURE, //add by zhao for syntax parsing
			END,
			PLUS = '+',
			MINUS = '-',
			MULTIPLY = '*',
			DIVIDE = '/',
			ASSIGN = '=',
			LHPAREN = '(',
			RHPAREN = ')',
			COMMA = ',',
			NOT = '!',

			// comparisons
			LT = '<',
			GT = '>',
			LE,     // <=
			GE,     // >=
			EQ,     // ==
			NE,     // !=
			AND,    // &&
			OR,      // ||

			// special assignments

			ASSIGN_ADD,  //  +=
			ASSIGN_SUB,  //  +-
			ASSIGN_MUL,  //  +*
			ASSIGN_DIV   //  +/

		};

	public:
		ExprValue();
		~ExprValue();

		enum TokenType type_;
		bool b_value_;
		double d_value_;
		int64_t i_value_;
		std::string s_value_;

		ExprValue(enum TokenType type);
		ExprValue(bool b_value);
		ExprValue(int64_t i_value);
		ExprValue(double d_value);
		ExprValue(const std::string &s_value);
		void operator=(bool b_value);
		void operator=(int64_t i_value);
		void operator=(double d_value);
		void operator=(const std::string &s_value);
		ExprValue operator==(const ExprValue &value) const;
		ExprValue operator<(const ExprValue &value) const;
		ExprValue operator>(const ExprValue &value) const;
		ExprValue operator<=(const ExprValue &value) const;
		ExprValue operator>=(const ExprValue &value) const;
		ExprValue operator!=(const ExprValue &value) const;
		explicit operator bool() const;
		const ExprValue operator*=(const ExprValue &value);
		const ExprValue operator+=(const ExprValue &value);
		const ExprValue operator-=(const ExprValue &value);
		const ExprValue operator/=(const ExprValue &value);
		const ExprValue operator+(const ExprValue &value) const;
		const ExprValue operator-(const ExprValue &value) const;

		bool IsNumber() const;
		bool IsString() const;
		bool IsBool() const;
		bool IsInteger64() const;

		const std::string &String() const;
		double Number() const;
		bool Bool() const;
		int64_t Integer64() const;

		bool IsSuccess() const;

		std::string Print();
		static std::string GetTypeDesc(enum TokenType type);
	};

	class ExprParser;
	typedef const ExprValue(*OneCommonArgFunction)  (const ExprValue &arg, ExprParser *parser);
	typedef const ExprValue(*TwoCommonArgFunction)  (const ExprValue &arg1, const ExprValue &arg2, ExprParser *parser);
	extern std::map<std::string, OneCommonArgFunction>    OneCommonArgumentFunctions;
	extern std::map<std::string, TwoCommonArgFunction>    TwoCommonArgumentFunctions;

	class ExprParser {
	private:
		std::string program_;
		const char * pWord_;
		const char * pWordStart_;
		// last token parsed
		ExprValue::TokenType type_;
		std::string word_;
		double value_;
		int64_t i_value_;
		bool detect_;
	public:

		// ctor
		ExprParser(const std::string & program)
			: program_(program), detect_(false){
			// insert pre-defined names:
			symbols_["pi"] = 3.1415926535897932385;
			symbols_["e"] = 2.7182818284590452354;
		}

		const ExprValue Evaluate();  // get result
		const ExprValue Evaluate(const std::string & program);  // get result
		const ExprValue Parse(const std::string & program);  // parse
		const ExprValue Parse();  // parse

		// access symbols with operator []
		ExprValue & operator[] (const std::string & key) { return symbols_[key]; }

		// symbol table - can be accessed directly (eg. to copy a batch in)
		std::map<std::string, ExprValue> symbols_;

	private:

		const ExprValue::TokenType GetToken(const bool ignoreSign = false);
		const ExprValue CommaList(const bool get);
		const ExprValue Expression(const bool get);
		const ExprValue Comparison(const bool get);
		const ExprValue AddSubtract(const bool get);
		const ExprValue Term(const bool get);      // multiply and divide
		const ExprValue Primary(const bool get);   // primary (base) tokens

		inline void CheckToken(const ExprValue::TokenType wanted){
			if (type_ != wanted)
			{
				std::ostringstream s;
				s << "'" << static_cast <char> (wanted) << "' expected.";
				throw std::runtime_error(s.str());
			}
		}
	};  // end of Parser

}
#endif // EXPRPARSER_H

