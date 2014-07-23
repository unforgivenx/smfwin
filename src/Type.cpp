/*
 * Node.cpp
 *
 *  Created on: Jul 29, 2012
 *      Author: unforgiven
 */

#include "Type.h"
#include <iostream>
#include <algorithm>

using namespace std;

Type::Type() {
	type = "";
	data = "";
}

Type::Type(std::string ty) {
	type = ty;
	data = "";
}

Type::~Type() {
	// TODO Auto-generated destructor stub
}

std::string Type::getData() {
	return data;
}

int Type::setData(std::string dt) {
	data = dt;
	return 0;
}

std::string Type::getType() {
	return type;
}

int Type::setType(std::string ty) {
	type = ty;
	return 0;
}

bool operator== ( const Type &n1, const Type &n2)
{
		return n1.type == n2.type;
}

