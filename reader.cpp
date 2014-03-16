#include "reader.hpp"
#include <cctype>
#include <map>
#include <sstream>
#include <cassert>
#include <iostream>

static std::map<std::string, Reader::TokenType> keywords = {
	{"grid", Reader::KWGrid},
	{"capacity", Reader::KWCapacity},
	{"num", Reader::KWNum},
	{"net", Reader::KWNet},
};

template <typename D, typename S>
D lexicalCast(const S &src)
{
	std::stringstream ss;
	ss << src;
	D result;
	if(ss >> result)
		return result;
	else
		throw std::runtime_error("Cannot read value");
}

void Reader::skipSpace()
{
	// skip whitespace manually to get line number
	while(in && std::isspace(in.peek())) {
		if(in.peek() == '\n') {
			lineNum++;
		}
		in.get();
	}
}

void Reader::readNextToken()
{
	if(putback) {
		putback = false;
		return;
	}

	skipSpace();

	tokenType = TInvalid;
	if(in >> token) {
		if(token.empty()) {
			fail("Unexpected empty token.");
		}
		else if(std::isdigit(token[0])) {
			try {
				intValue = lexicalCast<int>(token);
			}
			catch(std::runtime_error &) {
				fail("Invalid integer: '" + token + "'");
			}
			tokenType = TInteger;
		}
		else if(keywords.count(token)) {
			tokenType = keywords[token];
		}
		else if(token[0] == 'n') {
			tokenType = TNetName;
			try {
				intValue = lexicalCast<int>(token.c_str() + 1);
			}
			catch(std::runtime_error &) {
				fail("Invalid net name: '" + token + "'");
			}
		}
		else {
			unexpected();
		}
	}

	skipSpace();
}

void Reader::unexpected()
{
	fail("Unexpected token: '" + token + "'.");
}

void Reader::expectValid()
{
	if(tokenType == TInvalid) fail("Unexpected EOF");
}
void Reader::expect(TokenType ty) 
{
	readNextToken();
	if(tokenType != ty) unexpected();
}

Point Reader::readPoint()
{
	Point result;

	expect(TInteger);
	result.x = intValue;

	expect(TInteger);
	result.y = intValue;

	return result;
}

Net Reader::readNet()
{
	Net result;
	
	expect(TNetName);
	result.id = intValue;
	
	expect(TInteger);
	const int pinCount = intValue;
	if(pinCount < 0) fail("Pin count must be nonnegative.");

	for(int i = 0; i < pinCount; ++i) {
		result.pins.push_back(readPoint());
	}

	return result;
}

void Reader::emitMessage(const std::string &msg)
{
	std::cerr << "Line " << lineNum << ": " << msg << "\n";
}


void Reader::fail(const std::string &msg)
{
	std::stringstream s;
	s << "Line " << lineNum << ": " << msg;
	throw ParseError(s.str());
}


RoutingInst Reader::readRoutingInst()
{
	RoutingInst result;
	readNextToken();

	bool readHdr = false;
	int netCount = 0;

	while(tokenType != TInvalid) {
		switch(tokenType) {
			case KWGrid: 
				expect(TInteger);
				result.gx = intValue;
				expect(TInteger);
				result.gy = intValue;
				break;

			case KWCapacity:
				expect(TInteger);
				result.cap = intValue;
				break;

			case KWNum:
				readHdr = true;
				expect(KWNet);
				expect(TInteger);
				netCount = intValue;
				for(int i = 0; i < netCount; ++i) {
					result.nets.push_back(readNet());
				}
				break;

			case TInteger: {
				if(!readHdr) {
					fail("Unexpected integer.");
				}

				int nBlockages = intValue;

				for(int i = 0; i < nBlockages; ++i) {
					Point p1 = readPoint();
					Point p2 = readPoint();
					expect(TInteger);
					int capacity = intValue;
					result.setEdgeCap(p1, p2, capacity);
				}

			} break;
			default:
				unexpected();
		}
		readNextToken();
	}

	return result;
}