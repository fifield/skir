#ifndef _FECONTEXT_HPP_
#define _FECONTEXT_HPP_

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
using std::string;
using std::vector;

namespace streamit {

class FEContext
{
public:
    FEContext() :
	fileName(), lineNumber(-1), columnNumber(-1) {}
    FEContext(string file) :
	fileName(file), lineNumber(-1), columnNumber(-1) {}
    FEContext(string file, int line) :
	fileName(file), lineNumber(line), columnNumber(-1) {}
    FEContext(string file, int line, int col) :
	fileName(file), lineNumber(line), columnNumber(col) {}

    string getFileName() 
    { 
	return fileName;
    }
    
    int getLineNumber()
    {
	return lineNumber;
    }

    int getColumnNumber()
    {
	return columnNumber;
    }

    string getLocation() 
    {
	string file = getFileName();
	if (file.size() == 0) file = "<unknown>";
        int line = getLineNumber();
        if (line >= 0) {
	    std::stringstream ss;
	    ss << file << ":" << line;
	    return ss.str();
	}
        return file;
    }
    
    string toString()
    {
        return getLocation();
    }

private:
    string fileName;
    int lineNumber;
    int columnNumber;
};

}

#endif
