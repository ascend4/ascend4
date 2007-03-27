#ifndef ASCXX_INSTANCEINTERFACEDATA_H
#define ASCXX_INSTANCEINTERFACEDATA_H

#include "instance.h"

/**
	This defines a flexible interface for assigned extra
	data to instances for the sake of tricky interface
	features.

	The first of these is the ability to show whether or
	not an instance has been solved. We will have a function
	that calls instance.setInterfaceValue(ASCXX_VAR_STATUS,ASCXX_VAR_ACTIVE)
	when a solver variable is made active, and then
	instance.setInterfaceValue(ASCXX_VAR_STATUS,ASCXX_VAR_SOLVED)
	once it's solved.

	From the interface, python will be able to call
	instance.getInterfaceValue(ASCXX_VAR_STATUS) and
	will then be able to show an icon accordingly.

	@NOTE LEAKY!
*/
class InstanceInterfaceData{
private:
	friend class Instanc;
	InstanceInterfaceData();
	InstanceStatus status;
};
	

#endif /* ASCXX_INSTANCEINTERFACEDATA_H */
