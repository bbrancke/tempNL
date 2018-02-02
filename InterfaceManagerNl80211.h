// InterfaceManagerNl80211.h
// InterfaceManager using NL80211 subsystem.

#ifndef INTERFACEMANAGERNL80211_H_
#define INTERFACEMANAGERNL80211_H_

#include <iostream>
#include <string>
#include <sstream>
/***
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
****/
#include "Log.h"
#include "Nl80211Base.h"
#include "IInterfaceManager.h"
/*********
Use an Interface with Base class(es):
//
// https://stackoverflow.com/questions/3686210/c-using-a-base-class-as-the-implementation-of-an-interface
// I:
class Interface {
    public:
        virtual void myfunction() = 0;
};

class Base {
    public:
        virtual void myfunction() {/ *...* /}
};

class Derived : public Interface, public Base {
    public:
        void myfunction() { Base::myfunction(); }  // forwarding call  <===
};

int main() {
   Derived d;
   d.myfunction();
   return 0;
}
//
// Or, II: We want to provide several base classes that implement the same interface.
// The Interface class is the base of the other classes:

// in this order:
class Interface  <== IInterfaceManager
{
    virtual void myfunction() = 0;
}

// "Concrete" implementation - InterfaceManagerNl80211 class:
class BaseA : public Interface
{   
    // here "virtual" is optional as if the parent is virtual, the child is virtual too
    virtual void myfunction() {/ *...* /}; // BaseA specific implementation
}
class BaseB : public Interface
{
    virtual void myfunction() {/ *...* /}; // BaseB specific implementation
}
**********/

using namespace std;

class InterfaceManagerNl80211 : public IInterfaceManager, public Nl80211Base
{
public:
	static InterfaceManagerNl80211* GetInstance();
	virtual bool GetInterfaceList();
private:
	InterfaceManagerNl80211();  // Private so that it can  not be called
	InterfaceManagerNl80211(InterfaceManagerNl80211 const&) {};  // copy constructor is private
	InterfaceManagerNl80211& operator=(InterfaceManagerNl80211 const&){};  // assignment operator is private
	static InterfaceManagerNl80211* m_pInstance;
};


#endif  // INTERFACEMANAGERNL80211_H_

