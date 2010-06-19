// Market.cpp
// Implements the Market Class
#include "Market.h"
#include "Commodity.h"
#include "GenException.h"

using namespace std;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 Market::Market()
{
	cout << "Used the default Market constructor!!"<< endl;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Market::printMyName(){
	cout << "this is the printMyName function in Market.cpp"<<endl; 
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Market::Market(string mktName, int SN) : 
	myName(mktName), myID(SN)
{

	myCommods = vector<Commodity*>();
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Market::addCommod(string s, bool fissile, bool sepMat)
{
	Commodity* newCommod = new Commodity(s, this, fissile, sepMat);
	myCommods.push_back(newCommod);
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Market::addCommod(Commodity* c)
{
	myCommods.push_back(c);
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Commodity* Market::getCommod(int SN)
{
	vector<Commodity*>::iterator iter;
	for(iter = myCommods.begin(); iter != myCommods.end(); iter ++)
		if ((*iter)->getSN() == SN)
			return *iter;
	throw GenException("Error: tried to access Inst not present in this Region");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
pair<vector<Commodity*>::iterator, vector<Commodity*>::iterator> Market::getCommods()
{
	return make_pair(myCommods.begin(), myCommods.end());
}

Market::~Market()
{
	// This destructor doesn't do anything. 
	// It should delete any data allocated by this class
	// if there ever is any. 
}
