// NullFacility.cpp
// Implements the NullFacility class
#include <iostream>

#include "NullFacility.h"

#include "Logician.h"
#include "GenException.h"
#include "InputXML.h"

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void NullFacility::init(xmlNodePtr cur)
{ 
  FacilityModel::init(cur);
  
  in_commod = out_commod = NULL; 
  
  // move XML pointer to current model
  cur = XMLinput->get_xpath_element(cur,"model/NullFacility");

  // all facilities require commodities - possibly many
  string commod_name;
  Commodity* new_commod;
  
  commod_name = XMLinput->get_xpath_content(cur,"incommodity");
  in_commod = LI->getCommodity(commod_name);
  if (NULL == in_commod)
    throw GenException("Input commodity '" + commod_name 
                       + "' does not exist for facility '" + getName() 
                       + "'.");
  
  commod_name = XMLinput->get_xpath_content(cur,"outcommodity");
  out_commod = LI->getCommodity(commod_name);
  if (NULL == out_commod)
    throw GenException("Output commodity '" + commod_name 
                       + "' does not exist for facility '" + getName() 
                       + "'.");
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void NullFacility::copy(NullFacility* src)
{

  FacilityModel::copy(src);

  in_commod = src->in_commod;
  out_commod = src->out_commod;

}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void NullFacility::print() 
{ 
  FacilityModel::print(); 
  cout << "converts commodity {"
      << in_commod->getName()
      << "} into commodity {"
      << out_commod->getName()
      << "}" << endl;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void NullFacility::receiveMessage(Message* msg)
{
  // find out the amount of the transaction 
  Transaction trans = msg->getTrans();
  // check if you have enough in inventory to fill the order
  Mass inv = this->checkInventory();
  if(inv >= trans.amount){
    // if there's enough, package the material and send it
    
  }
  else if (inv < trans.amount){
    // if not, send what you can and print a warning

  }

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void NullFacility::sendMaterial(Transaction trans, Communicator* receiver)
{

  // create an appropriate material object
  // it should be of out_commod Commodity type
  if(trans.commod != out_commod){
    throw GenException("NullFacility can only send out_commod materials.");
  }

  // delete the transaction amount from the inventory

  // it should be of the same size, recipe, etc. as the transaction

  // push the material into a vector

  // make the reciever receive the manifest
  
  // record this action
}
    
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void NullFacility::receiveMaterial(Transaction trans, vector<Material*>)
{
  // interperet the Transaction information
  // add the material object from the manifest to the stocks
  // record this action
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void NullFacility::handleTick(int time)
{
  // find out how much this NullFacility needs to request
    // it will be Capacity - Stocks (or stocklimit - stocks ?)
  // request it

  // find out how much this NullFacility can offer
    // it will be Inventory + SpotCapacity 
  // offer it
}
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -    
void NullFacility::handleTock(int time)
{
  // at rate allowed by capacity, convert material in Stocks to out_commod type
  // move converted material into Inventory

  // receive information about matched orders from the Market?
    // fill them by sending/receiving material  

  // record these actions
}

