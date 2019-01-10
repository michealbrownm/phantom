/*

 Parser - an expression parser

 Author:  Nick Gammon
 http://www.gammon.com.au/

 (C) Copyright Nick Gammon 2004. Permission to copy, use, modify, sell and
 distribute this software is granted provided this copyright notice appears
 in all copies. This software is provided "as is" without express or implied
 warranty, and with no claim as to its suitability for any purpose.

 Modified 24 October 2005 by Nick Gammon.

 1. Changed use of "abs" to "fabs"
 2. Changed inclues from math.h and time.h to fmath and ftime
 3. Rewrote DoMin and DoMax to inline the computation because of some problems with some libraries.
 4. Removed "using namespace std;" and put "std::" in front of std namespace names where appropriate
 5. Removed MAKE_STRING macro and inlined the functionality where required.
 6. Changed Evaluate function to take its argument by reference.

 Modified 13 January 2010 by Nick Gammon.

 1. Changed getrandom to work more reliably (see page 2 of discussion thread)
 2. Changed recognition of numbers to allow for .5 (eg. "a + .5" where there is no leading 0)
 Also recognises -.5 (so you don't have to write -0.5)
 3. Fixed problem where (2+3)-1 would not parse correctly (- sign directly after parentheses)
 4. Fixed problem where changing a parameter and calling p.Evaluate again would fail because the
 initial token type was not reset to NONE.

 Modified 16 February 2010 by Nick Gammon

 1. Fixed bug where if you called Evaluate () twice, the original expression would not be reprocessed.

 Modified 27 November 2014 by Nick Gammon

 1. Fixed bug where a literal number followed by EOF would throw an error.

 Thanks to various posters on my forum for suggestions. The relevant post is currently at:
 http://www.gammon.com.au/forum/?id=4649

 Modified 23 February 2017 by Zhengyong Zhao
 1. add string support

 */

#include "strings.h"
#include "exprparser.h"

namespace utils {

	/*

	Expression-evaluator
	--------------------

	Author: Nick Gammon
	-------------------


	Example usage:

	Parser p ("2 + 2 * (3 * 5) + nick");

	p.symbols_ ["nick"] = 42;

	double v = p.Evaluate ();

	double v1 = p.Evaluate ("5 + 6");   // supply new expression and evaluate it

	Syntax:

	You can use normal algebraic syntax.

	Multiply and divide has higher precedence than add and subtract.

	You can use parentheses (eg. (2 + 3) * 5 )

	Variables can be assigned, and tested. eg. a=24+a*2

	Variables can be preloaded:

	p.symbols_ ["abc"] = 42;
	p.symbols_ ["def"] = 42;

	Afterwards they can be retrieved:

	x = p.symbols_ ["abc"];

	There are 2 predefined symbols, "pi" and "e".

	You can use the comma operator to load variables and then use them, eg.

	a=42, b=a+6

	You can use predefined functions, see below for examples of writing your own.

	42 + int (64.1)


	Comparisons
	-----------

	Comparisons work by returning 1.0 if true, 0.0 if false.

	Thus, 2 > 3 would return 0.0
	3 > 2 would return 1.0

	Similarly, tests for truth (eg. a && b) test whether the values are 0.0 or not.

	If test
	-------

	There is a ternary function: if (truth-test, true-value, false-value)

	eg.  if (1 < 2, 22, 33)  returns 22


	Precedence
	----------

	( )  =   - nested brackets, including function calls like sqrt (x), and assignment
	* /      - multiply, divide
	+ -      - add and subtract
	< <= > >= == !=  - comparisons
	&& ||    - AND and OR
	,        - comma operator

	Credits:

	Based in part on a simple calculator described in "The C++ Programming Language"
	by Bjarne Stroustrup, however with considerable enhancements by me, and also based
	on my earlier experience in writing Pascal compilers, which had a similar structure.

	*/

	// functions we can call from an expression

	double DoInt(double arg){
		return (int)arg;   // drop fractional part
	}

	const ExprValue DoMin(const ExprValue &arg1, const ExprValue &arg2){
		return (arg1 < arg2 ? arg1 : arg2);
	}

	const ExprValue DoMax(const ExprValue &arg1, const ExprValue &arg2)
	{
		return (arg1 > arg2 ? arg1 : arg2);
	}


	const ExprValue DoPow(const ExprValue &arg1, const ExprValue &arg2){
		if (arg1.type_ != ExprValue::NUMBER || arg2.type_ != ExprValue::NUMBER)
			throw std::runtime_error("Type is not number");
		return pow(arg1.d_value_, arg2.d_value_);
	}

	const ExprValue DoIf(const ExprValue &arg1, const ExprValue &arg2, const ExprValue &arg3){
		if (arg1 != 0.0)
			return arg2;
		else
			return arg3;
	}

	typedef double(*OneArgFunction)  (double arg);
	typedef const ExprValue(*TwoArgFunction)  (const ExprValue &arg1, const ExprValue &arg2);
	typedef const ExprValue(*ThreeArgFunction)  (const ExprValue &arg1, const ExprValue &arg2, const ExprValue &arg3);

	// maps of function names to functions
	static std::map<std::string, OneArgFunction>    OneArgumentFunctions; //for internal use
	std::map<std::string, OneCommonArgFunction>    OneCommonArgumentFunctions; //for custom user
	static std::map<std::string, TwoArgFunction>    TwoArgumentFunctions; //for internal use
	std::map<std::string, TwoCommonArgFunction>    TwoCommonArgumentFunctions; //for custom user
	static std::map<std::string, ThreeArgFunction>  ThreeArgumentFunctions;//for internal use

	// for standard library functions
#define STD_FUNCTION(arg) OneArgumentFunctions [#arg] = arg

	static int LoadOneArgumentFunctions(){
		OneArgumentFunctions["int"] = DoInt;
		return 0;
	} // end of LoadOneArgumentFunctions

	static int LoadOneCommonArgumentFunctions(){
		return 0;
	} // end of LoadTwoArgumentFunctions

	static int LoadTwoArgumentFunctions(){
		TwoArgumentFunctions["min"] = DoMin;
		TwoArgumentFunctions["max"] = DoMax;
		TwoArgumentFunctions["pow"] = DoPow;     //   x to the power y
		return 0;
	} // end of LoadTwoArgumentFunctions

	static int LoadThreeArgumentFunctions(){
		ThreeArgumentFunctions["if"] = DoIf;
		return 0;
	} // end of LoadThreeArgumentFunctions

	ExprValue::ExprValue(enum TokenType type) : type_(type){}

	ExprValue::ExprValue() : d_value_(0), i_value_(0), b_value_(false){
		type_ = ExprValue::INTEGER64;
	}
	ExprValue::~ExprValue(){}

	ExprValue::ExprValue(bool b_value) : d_value_(0), i_value_(0){
		type_ = BOOL;
		b_value_ = b_value;
	}

	ExprValue::ExprValue(int64_t i_value) : d_value_(0), b_value_(false){
		type_ = INTEGER64;
		i_value_ = i_value;
	}

	ExprValue::ExprValue(double d_value) : i_value_(0), b_value_(false){
		type_ = NUMBER;
		d_value_ = d_value;
	}

	ExprValue::ExprValue(const std::string &s_value) : i_value_(0), d_value_(0), b_value_(false){
		type_ = STRING;
		s_value_ = s_value;
	}

	void ExprValue::operator = (bool b_value){
		type_ = BOOL;
		b_value_ = b_value;
	}

	void ExprValue::operator = (int64_t i_value){
		type_ = INTEGER64;
		i_value_ = i_value;
	}

	void ExprValue::operator=(double d_value) {
		type_ = NUMBER;
		d_value_ = d_value;
	}

	void ExprValue::operator=(const std::string &s_value) {
		type_ = STRING;
		s_value_ = s_value;
	}

	ExprValue ExprValue::operator==(const ExprValue &value) const{
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			return d_value_ == value.i_value_;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			return i_value_ == value.d_value_;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}

		switch (type_){
		case NUMBER:{
			return d_value_ == value.d_value_;
		}
		case INTEGER64:{
			return i_value_ == value.i_value_;
		}
		case STRING:{
			return s_value_ == value.s_value_;
		}
		default:
			throw std::runtime_error("type is unknown");
			break;
		}

		return false;
	}

	ExprValue::operator bool() const{
		switch (type_){
		case BOOL:{
			return b_value_;
		}
		case NUMBER:{
			return d_value_ != 0;
		}
		case INTEGER64:{
			return i_value_ != 0;
		}
		case STRING:{
			return !s_value_.empty();
		}
					//case UNSURE:{
					//	return false;
					//}
		default:
			throw std::runtime_error("type is unknown");
			break;
		}

		return false;
	}

	ExprValue ExprValue::operator<(const ExprValue &value) const {
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64 ){
			return d_value_ < value.i_value_;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			return i_value_ < value.d_value_;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}

		switch (type_){
		case NUMBER:{
			return d_value_ < value.d_value_;
		}
		case INTEGER64:{
			return i_value_ < value.i_value_;
		}
		case STRING:{
			return s_value_.compare(value.s_value_) < 0;
		}
		default:
			throw std::runtime_error("type is unknown");
			break;
		}

		return false;
	}

	ExprValue ExprValue::operator>(const ExprValue &value) const{
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			return d_value_ > value.i_value_;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			return i_value_ > value.d_value_;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}

		switch (type_){
		case NUMBER:{
			return d_value_ > value.d_value_;
		}
		case INTEGER64:{
			return i_value_ > value.i_value_;
		}
		case STRING:{
			return s_value_.compare(value.s_value_) > 0;
		}
		default:
			throw std::runtime_error("type is unknown");
			break;
		}

		return false;
	}

	ExprValue ExprValue::operator<=(const ExprValue &value) const{
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			return d_value_ <= value.i_value_;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			return i_value_ <= value.d_value_;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}

		switch (type_){
		case NUMBER:{
			return d_value_ <= value.d_value_;
		}
		case INTEGER64:{
			return i_value_ <= value.i_value_;
		}
		case STRING:{
			return s_value_.compare(value.s_value_) <= 0;
		}
		default:
			throw std::runtime_error("type is unknown");
			break;
		}

		return false;
	}

	ExprValue ExprValue::operator>=(const ExprValue &value) const{
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			return d_value_ >= value.i_value_;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			return i_value_ >= value.d_value_;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}

		switch (type_){
		case NUMBER:{
			return d_value_ >= value.d_value_;
		}
		case INTEGER64:{
			return i_value_ >= value.i_value_;
		}
		case STRING:{
			return s_value_.compare(value.s_value_) >= 0;
		}
		default:
			throw std::runtime_error("type is unknown");
			break;
		}

		return false;
	}

	ExprValue ExprValue::operator!=(const ExprValue &value) const{
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			return d_value_ != value.i_value_;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			return i_value_ != value.d_value_;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}

		switch (type_){
		case NUMBER:{
			return d_value_ != value.d_value_;
		}
		case INTEGER64:{
			return i_value_ != value.i_value_;
		}
		case STRING:{
			return s_value_.compare(value.s_value_) != 0;
		}
		default:
			throw std::runtime_error("type is unknown");
			break;
		}

		return false;
	}

	const ExprValue ExprValue::operator*=(const ExprValue &value){
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			type_ = NUMBER;
			d_value_ *= value.i_value_;
			return *this;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			type_ = NUMBER;
			d_value_ = i_value_ * value.d_value_;
			return *this;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}
		switch (type_){
		case NUMBER:{
			d_value_ *= value.d_value_;
			break;
		}
		case INTEGER64:{
			i_value_ *= value.i_value_;
			break;
		}
		default:
			throw std::runtime_error("type is support");
			break;
		}

		return *this;
	}

	const ExprValue ExprValue::operator/=(const ExprValue &value){
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			type_ = NUMBER;
			d_value_ /= value.i_value_;
			return *this;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			type_ = NUMBER;
			d_value_ = i_value_ / value.d_value_;
			return *this;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}
		switch (type_){
		case NUMBER:{
			d_value_ /= value.d_value_;
			break;
		}
		case INTEGER64:{
			type_ = NUMBER;
			d_value_ = i_value_ /(double)value.i_value_;
			break;
		}
		default:
			throw std::runtime_error("type is support");
			break;
		}

		return *this;
	}

	const ExprValue ExprValue::operator+=(const ExprValue &value){
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			type_ = NUMBER;
			d_value_ += value.i_value_;
			return *this;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			type_ = NUMBER;
			d_value_ = i_value_ + value.d_value_;
			return *this;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}
		switch (type_){
		case NUMBER:{
			d_value_ += value.d_value_;
			break;
		}
		case INTEGER64:{
			i_value_ += value.i_value_;
			break;
		}
		case STRING:{
			s_value_.append(value.s_value_);
			break;
		}
		default:
			throw std::runtime_error("type is support");
			break;
		}

		return *this;
	}

	const ExprValue  ExprValue::operator-=(const ExprValue &value){
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		if (type_ == NUMBER && value.type_ == INTEGER64){
			type_ = NUMBER;
			d_value_ -= value.i_value_;
			return *this;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			type_ = NUMBER;
			d_value_ = i_value_ - value.d_value_;
			return *this;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}
		switch (type_){
		case NUMBER:{
			d_value_ -= value.d_value_;
			break;
		}
		case INTEGER64:{
			i_value_ -= value.i_value_;
			break;
		}
		default:
			throw std::runtime_error("type is support");
			break;
		}

		return *this;
	}

	const ExprValue ExprValue::operator+(const ExprValue &value) const{
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		ExprValue v;
		if (type_ == NUMBER && value.type_ == INTEGER64){
			v.type_ = NUMBER;
			v.d_value_ = d_value_ + value.d_value_;
			return *this;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			v.type_ = NUMBER;
			v.d_value_ = i_value_ + value.d_value_;
			return *this;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}

		v.type_ = type_;
		switch (type_){
		case NUMBER:{
			v.d_value_ = d_value_ + value.d_value_;
			break;
		}
		case INTEGER64:{
			v.i_value_ = i_value_ + value.i_value_;
			break;
		}
		case STRING:{
			v.s_value_ = s_value_ + value.s_value_;
			break;
		}
		default:
			throw std::runtime_error("type is support");
			break;
		}

		return v;
	}


	const ExprValue ExprValue::operator-(const ExprValue &value) const{
		if (type_ == UNSURE || value.type_ == UNSURE){
			return ExprValue(UNSURE);
		}

		ExprValue v;
		if (type_ == NUMBER && value.type_ == INTEGER64){
			v.type_ = NUMBER;
			v.d_value_ = d_value_ - value.d_value_;
			return *this;
		}
		else if (type_ == INTEGER64 && value.type_ == NUMBER){
			v.type_ = NUMBER;
			v.d_value_ = i_value_ - value.d_value_;
			return *this;
		}

		if (type_ != value.type_){
			throw std::runtime_error("type not compatible");
		}

		v.type_ = type_;
		switch (type_){
		case NUMBER:{
			v.d_value_ = d_value_ - value.d_value_;
			break;
		}
		case INTEGER64:{
			v.i_value_ = i_value_ - value.i_value_;
			break;
		}
		default:
			throw std::runtime_error("type is support");
			break;
		}

		return v;
	}

	std::string ExprValue::Print(){
		if (type_ == NUMBER){
			//int prec = 18;// std::numeric_limits::digits10; // 18
			std::ostringstream out;
			out.precision(0);//¸²¸ÇÄ¬ÈÏ¾«¶È
			out << d_value_;
			std::string str = out.str();

			//std::string s;
			//s.resize(1024);
			//s.resize(sprintf_s((char *)s.c_str(), s.size(), "%f", d_value_));
			return str;
		}
		else if (type_ == INTEGER64){
			std::string s = utils::String::ToString(i_value_);
			return s;
		}
		else if (type_ == BOOL) {
			return b_value_ ? "true":"false";
		}
		else{
			return s_value_;
		}
	}

	std::string ExprValue::GetTypeDesc(enum TokenType type){
		std::string ret;
		switch( type ) {
		case NUMBER: ret = "NUMBER"; break;
		case STRING: ret = "STRING"; break;
		case BOOL: ret = "BOOL"; break;
		case INTEGER64: ret = "INTEGER64"; break;
		case UNSURE: ret = "UNSURE"; break;
		default:  ret = "OTHER"; break;
		}

		return ret;
	}

	bool ExprValue::IsNumber() const {
		return type_ == NUMBER;
	}

	bool ExprValue::IsString() const {
		return type_ == STRING;
	}

	bool ExprValue::IsBool() const{
		return type_ == BOOL;
	}

	bool ExprValue::IsInteger64() const{
		return type_ == INTEGER64;
	}

	const std::string &ExprValue::String() const{
		return s_value_;
	}

	double ExprValue::Number() const {
		return d_value_;
	}

	bool ExprValue::Bool() const {
		return b_value_;
	}

	int64_t ExprValue::Integer64() const {
		return i_value_;
	}

	bool ExprValue::IsSuccess() const{
		if (IsBool()){
			return Bool();
		} else if (IsNumber()){
			return Number() > 0.0;
		} else if (IsInteger64()){
			return Integer64() > 0;
		}

		return false;
	}

	const ExprValue::TokenType ExprParser::GetToken(const bool ignoreSign)
	{
		word_.erase(0, std::string::npos);

		// skip spaces
		while (*pWord_ && isspace(*pWord_))
			++pWord_;

		pWordStart_ = pWord_;   // remember where word_ starts *now*

		// look out for unterminated statements and things
		if (*pWord_ == 0 &&  // we have EOF
			type_ == ExprValue::END)  // after already detecting it
			throw std::runtime_error("Unexpected end of expression.");

		unsigned char cFirstCharacter = *pWord_;        // first character in new word_

		if (cFirstCharacter == 0)    // stop at end of file
		{
			word_ = "<end of expression>";
			return type_ = ExprValue::END;
		}

		unsigned char cNextCharacter = *(pWord_ + 1);  // 2nd character in new word_

		// look for number
		// can be: + or - followed by a decimal point
		// or: + or - followed by a digit
		// or: starting with a digit
		// or: decimal point followed by a digit
		if ((!ignoreSign &&
			(cFirstCharacter == '+' || cFirstCharacter == '-') &&
			(isdigit(cNextCharacter) || cNextCharacter == '.')
			)
			|| isdigit(cFirstCharacter)
			// allow decimal numbers without a leading 0. e.g. ".5"
			// Dennis Jones 01-30-2009
			|| (cFirstCharacter == '.' && isdigit(cNextCharacter)))
		{
			// skip sign for now
			if ((cFirstCharacter == '+' || cFirstCharacter == '-'))
				pWord_++;
			while (isdigit(*pWord_) || *pWord_ == '.')
				pWord_++;

			// allow for 1.53158e+15
			if (*pWord_ == 'e' || *pWord_ == 'E')
			{
				pWord_++; // skip 'e'
				if ((*pWord_ == '+' || *pWord_ == '-'))
					pWord_++; // skip sign after e
				while (isdigit(*pWord_))  // now digits after e
					pWord_++;
			}

			word_ = std::string(pWordStart_, pWord_ - pWordStart_);

			if (word_.find("e") == std::string::npos && 
				word_.find(".") == std::string::npos){
#ifdef WIN32
				sscanf_s(word_.c_str(), "%I64d", &i_value_);
#else
				sscanf(word_.c_str(), "%ld", &i_value_);
#endif
				return type_ = ExprValue::INTEGER64;
			}

			std::istringstream is(word_);
			// parse std::string into double value
			is >> value_;

			if (is.fail() && !is.eof())
				throw std::runtime_error("Bad numeric literal: " + word_);
			return type_ = ExprValue::NUMBER;
		}   // end of number found

		// special test for 2-character sequences: <= >= == !=
		// also +=, -=, /=, *=
		if (cNextCharacter == '=')
		{
			switch (cFirstCharacter)
			{
				// comparisons
			case '=': type_ = ExprValue::EQ;   break;
			case '<': type_ = ExprValue::LE;   break;
			case '>': type_ = ExprValue::GE;   break;
			case '!': type_ = ExprValue::NE;   break;
				// assignments
			case '+': type_ = ExprValue::ASSIGN_ADD;   break;
			case '-': type_ = ExprValue::ASSIGN_SUB;   break;
			case '*': type_ = ExprValue::ASSIGN_MUL;   break;
			case '/': type_ = ExprValue::ASSIGN_DIV;   break;
				// none of the above
			default:  type_ = ExprValue::NONE; break;
			} // end of switch on cFirstCharacter

			if (type_ != ExprValue::NONE)
			{
				word_ = std::string(pWordStart_, 2);
				pWord_ += 2;   // skip both characters
				return type_;
			} // end of found one    
		} // end of *=

		switch (cFirstCharacter)
		{
		case '&': if (cNextCharacter == '&')    // &&
		{
			word_ = std::string(pWordStart_, 2);
			pWord_ += 2;   // skip both characters
			return type_ = ExprValue::AND;
		}
				  break;
		case '|': if (cNextCharacter == '|')   // ||
		{
			word_ = std::string(pWordStart_, 2);
			pWord_ += 2;   // skip both characters
			return type_ = ExprValue::OR;
		}
				  break;
				  // single-character symboles
		case '=':
		case '<':
		case '>':
		case '+':
		case '-':
		case '/':
		case '*':
		case '(':
		case ')':
		case ',':
		case '!':
			word_ = std::string(pWordStart_, 1);
			++pWord_;   // skip it
			return type_ = ExprValue::TokenType(cFirstCharacter);
		} // end of switch on cFirstCharacter

		if (!isalpha(cFirstCharacter) && cFirstCharacter != '\"')
		{
			if (cFirstCharacter < ' ')
			{
				std::ostringstream s;
				s << "Unexpected character (decimal " << int(cFirstCharacter) << ")";
				throw std::runtime_error(s.str());
			}
			else
				throw std::runtime_error("Unexpected character: " + std::string(1, cFirstCharacter));
		}

		//add by zhao for string
		if (cFirstCharacter == '\"'){
			do {
				++pWord_;
			} while (*pWord_ != '\"');
			word_ = std::string(pWordStart_ + 1, pWord_ - pWordStart_ - 1);
			++pWord_;
			return type_ = ExprValue::STRING;
		}

		// we have a word (starting with A-Z) - pull it out
		while (isalnum(*pWord_) || *pWord_ == '_')
			++pWord_;

		word_ = std::string(pWordStart_, pWord_ - pWordStart_);
		return type_ = ExprValue::NAME;
	}   // end of Parser::GetToken

	// force load of functions at static initialisation time
	static int doLoadOneArgumentFunctions = LoadOneArgumentFunctions();
	static int doLoadOneCommonArgumentFunctions = LoadOneCommonArgumentFunctions();
	static int doLoadTwoArgumentFunctions = LoadTwoArgumentFunctions();
	static int doLoadThreeArgumentFunctions = LoadThreeArgumentFunctions();

	const ExprValue ExprParser::Primary(const bool get) {  // primary (base) tokens
		if (get)
			GetToken();    // one-token lookahead  

		switch (type_)
		{
		case ExprValue::NUMBER:
		{
			ExprValue v = value_;
			GetToken(true);  // get next one (one-token lookahead)
			return v;
		}
		case ExprValue::INTEGER64:
		{
			ExprValue v = i_value_;
			GetToken(true);  // get next one (one-token lookahead)
			return v;
		}
		case ExprValue::STRING:
		{
			ExprValue v = word_;
			GetToken(true);  // get next one (one-token lookahead)
			return v;
		}

		case ExprValue::NAME:
		{
			std::string word = word_;
			GetToken(true);
			if (type_ == ExprValue::LHPAREN)
			{
				// might be single-argument function (eg. abs (x) )
				std::map<std::string, OneArgFunction>::const_iterator si;
				si = OneArgumentFunctions.find(word);
				if (si != OneArgumentFunctions.end())
				{
					ExprValue v = Expression(true);   // get argument
					CheckToken(ExprValue::RHPAREN);
					GetToken(true);        // get next one (one-token lookahead)
					return si->second(v.d_value_);  // evaluate function
				}

				// might be single-common-argument function (eg. abs (x) )
				std::map<std::string, OneCommonArgFunction>::const_iterator sic;
				sic = OneCommonArgumentFunctions.find(word);
				if (sic != OneCommonArgumentFunctions.end())
				{
					ExprValue v = Expression(true);   // get argument
					CheckToken(ExprValue::RHPAREN);
					GetToken(true);        // get next one (one-token lookahead)
					return detect_ ? ExprValue(ExprValue::UNSURE) : sic->second(v,this);  // evaluate function
				}

				// might be double-argument function (eg. roll (6, 2) )
				std::map<std::string, TwoArgFunction>::const_iterator di;
				di = TwoArgumentFunctions.find(word);
				if (di != TwoArgumentFunctions.end())
				{
					ExprValue v1 = Expression(true);   // get argument 1 (not commalist)
					CheckToken(ExprValue::COMMA);
					ExprValue v2 = Expression(true);   // get argument 2 (not commalist)
					CheckToken(ExprValue::RHPAREN);
					GetToken(true);            // get next one (one-token lookahead)
					return di->second(v1, v2); // evaluate function
				}

				// might be double-common-argument function (eg. roll (6, 2) )
				std::map<std::string, TwoCommonArgFunction>::const_iterator dic;
				dic = TwoCommonArgumentFunctions.find(word);
				if (dic != TwoCommonArgumentFunctions.end())
				{
					ExprValue v1 = Expression(true);   // get argument 1 (not commalist)
					CheckToken(ExprValue::COMMA);
					ExprValue v2 = Expression(true);   // get argument 2 (not commalist)
					CheckToken(ExprValue::RHPAREN);
					GetToken(true);            // get next one (one-token lookahead)
					return detect_ ? ExprValue(ExprValue::UNSURE) : dic->second(v1, v2, this); // evaluate function
				}

				// might be double-argument function (eg. roll (6, 2) )
				std::map<std::string, ThreeArgFunction>::const_iterator ti;
				ti = ThreeArgumentFunctions.find(word);
				if (ti != ThreeArgumentFunctions.end())
				{
					ExprValue v1 = Expression(true);   // get argument 1 (not commalist)
					CheckToken(ExprValue::COMMA);
					ExprValue v2 = Expression(true);   // get argument 2 (not commalist)
					CheckToken(ExprValue::COMMA);
					ExprValue v3 = Expression(true);   // get argument 3 (not commalist)
					CheckToken(ExprValue::RHPAREN);
					GetToken(true);  // get next one (one-token lookahead)
					return ti->second(v1, v2, v3); // evaluate function
				}

				throw std::runtime_error("Function '" + word + "' not implemented.");
			}

			// not a function? must be a symbol in the symbol table
			ExprValue & v = symbols_[word];  // get REFERENCE to symbol table entry
			// change table entry with expression? (eg. a = 22, or a = 22)
			switch (type_)
			{
				// maybe check for NaN or Inf here (see: isinf, isnan functions)
			case ExprValue::ASSIGN:     v = Expression(true); break;
			case ExprValue::ASSIGN_ADD: v += Expression(true); break;
			case ExprValue::ASSIGN_SUB: v -= Expression(true); break;
			case ExprValue::ASSIGN_MUL: v *= Expression(true); break;
			case ExprValue::ASSIGN_DIV:
			{
				ExprValue d = Expression(true);
				if (d.type_ != ExprValue::UNSURE && d == 0.0)
					throw std::runtime_error("Divide by zero");
				v /= d;
				break;   // change table entry with expression
			} // end of ASSIGN_DIV
			default: break;   // do nothing for others
			} // end of switch on type_              
			return v;               // and return new value
		}

		case ExprValue::MINUS:               // unary minus
			return ExprValue((int64_t)0) - Primary(true);

		case ExprValue::NOT:   // unary not
			return (Primary(true) == 0.0) ? 1.0 : 0.0;;

		case ExprValue::LHPAREN:
		{
			ExprValue v = CommaList(true);    // inside parens, you could have commas
			CheckToken(ExprValue::RHPAREN);
			GetToken(true);                // eat the )
			return v;
		}

		default:
			throw std::runtime_error("Unexpected token: " + word_);

		} // end of switch on type

	} // end of Parser::Primary 

	const ExprValue ExprParser::Term(const bool get) {   // multiply and divide

		ExprValue left = Primary(get);
		while (true)
		{
			switch (type_)
			{
			case ExprValue::MULTIPLY:
				left *= Primary(true); break;
			case ExprValue::DIVIDE:
			{
				ExprValue d = Primary(true);
				if (d.type_ != ExprValue::UNSURE && d == 0.0)
					throw std::runtime_error("Divide by zero");
				left /= d;
				break;
			}
			default:    return left;
			} // end of switch on type
		}   // end of loop
	} // end of Parser::Term

	const ExprValue ExprParser::AddSubtract(const bool get){  // add and subtract

		ExprValue left = Term(get);
		while (true)
		{
			switch (type_)
			{
			case ExprValue::PLUS:  left += Term(true); break;
			case ExprValue::MINUS: left -= Term(true); break;
			default:    return left;
			} // end of switch on type
		}   // end of loop
	} // end of Parser::AddSubtract

	const ExprValue ExprParser::Comparison(const bool get){  // LT, GT, LE, EQ etc.

		ExprValue left = AddSubtract(get);
		while (true)
		{
			switch (type_)
			{
			case ExprValue::LT:
			{
				left = (left < AddSubtract(true));
				if (left.type_ != ExprValue::UNSURE) {
					left = left ? 1.0 : 0.0;
				}
				break;
			}
			case ExprValue::GT:
			{
				left = (left > AddSubtract(true));
				if (left.type_ != ExprValue::UNSURE) {
					left = left ? 1.0 : 0.0;
				}
				break;
			}
			case ExprValue::LE:
			{
				left = (left <= AddSubtract(true));
				if (left.type_ != ExprValue::UNSURE) {
					left = left ? 1.0 : 0.0;
				}
				break;
			}
			case ExprValue::GE:
			{
				left = (left >= AddSubtract(true));
				if (left.type_ != ExprValue::UNSURE) {
					left = left ? 1.0 : 0.0;
				}
				break;
			}
			case ExprValue::EQ:
			{
				left = (left == AddSubtract(true));
				if (left.type_ != ExprValue::UNSURE) {
					left = left ? 1.0 : 0.0;
				}
				break;
			}
			case ExprValue::NE:
			{
				left = (left != AddSubtract(true));
				if (left.type_ != ExprValue::UNSURE) {
					left = left ? 1.0 : 0.0;
				}
				break;
			}
			default:    return left;
			} // end of switch on type
		}   // end of loop
	} // end of Parser::Comparison

	const ExprValue ExprParser::Expression(const bool get){  // AND and OR

		ExprValue left = Comparison(get);
		while (true)
		{
			switch (type_)
			{
			case ExprValue::AND:
			{
				ExprValue d = Comparison(true);   // don't want short-circuit evaluation
				if (left.type_ != ExprValue::UNSURE) {
					left = (left != 0.0) && (d != 0.0);
				} 
			}
			break;
			case ExprValue::OR:
			{
				ExprValue d = Comparison(true);   // don't want short-circuit evaluation
				if (left.type_ != ExprValue::UNSURE) {
					left = (left != 0.0) || (d != 0.0);
				}
			}
			break;
			default:    return left;
			} // end of switch on type
		}   // end of loop
	} // end of Parser::Expression

	const ExprValue ExprParser::CommaList(const bool get){// expr1, expr2
		ExprValue left = Expression(get);
		while (true)
		{
			switch (type_)
			{
			case ExprValue::COMMA:  left = Expression(true); break; // discard previous value
			default:    return left;
			} // end of switch on type
		}   // end of loop
	} // end of Parser::CommaList

	const ExprValue ExprParser::Evaluate(){  // get result

		pWord_ = program_.c_str();
		type_ = ExprValue::NONE;
		ExprValue v = CommaList(true);
		if (type_ != ExprValue::END)
			throw std::runtime_error("Unexpected text at end of expression: " + std::string(pWordStart_));
		return v;
	}

	// change program and evaluate it
	const ExprValue ExprParser::Evaluate(const std::string & program) {  // get result
		program_ = program;
		return Evaluate();
	}

	const ExprValue ExprParser::Parse(const std::string & program){
		detect_ = true;
		ExprValue ret = Evaluate(program);
		detect_ = false;
		return ret;
	}

	const ExprValue ExprParser::Parse(){
		detect_ = true;
		ExprValue ret = Evaluate();
		detect_ = false;
		return ret;
	}
}