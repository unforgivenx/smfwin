/*
 * Node.h
 *
 *  Created on: Jul 29, 2012
 *      Author: unforgiven
 */

#ifndef TYPE_H_
#define TYPE_H_

#include <vector>
#include <string>



class Type {
public:
	Type();
	Type(std::string);
	virtual ~Type();
	std::string getData();
	int setData(std::string);
	std::string getType();
	int setType(std::string);
	friend bool operator== ( const Type &n1, const Type &n2);

protected:
	std::string type;
	std::string data;

};

#endif /* TYPE_H_ */
