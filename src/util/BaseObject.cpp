//
//  BaseObject.cpp
//  OpenLieroX
//
//  Created by Albert Zeyer on 20.01.12.
//  code under LGPL
//

#include "BaseObject.h"
#include "CBytestream.h"
#include "game/Attr.h"

void ObjRef::writeToBs(CBytestream* bs) const {
	bs->writeInt16(classId);
	bs->writeInt16(objId);
}

void ObjRef::readFromBs(CBytestream* bs) {
	classId = bs->readInt16();
	objId = bs->readInt16();
}

std::string ObjRef::description() const {
	const ClassInfo* classInfo = getClassInfo(classId);
	std::string r;
	if(classInfo) r += classInfo->name;
	else r += "<unknown class " + to_string(classId) + ">";
	r += ":";
	r += to_string(objId);
	return r;
}

BaseObject::BaseObject() {
	thisRef.obj.set(this);
}

BaseObject::BaseObject(const BaseObject& o) {
	(*this) = o;
}

BaseObject& BaseObject::operator=(const BaseObject& o) {
	if(thisRef.obj) thisRef.obj.overwriteShared(NULL);
	thisRef.obj.set(this);
	if(thisRef.classId == ClassId(-1))
		// Note: This classId assignment is a bit risky
		// because we cannot assure at this point that it is
		// correct because op= might be called at an early
		// point.
		thisRef.classId = o.thisRef.classId;
	else
		assert(thisRef.classId == o.thisRef.classId);
	if(o.thisRef.objId != ObjId(-1))
		// The reason this is an error is because we cannot really know if
		// the copy is really a takeover (in that case, we want to copy the
		// objId/WeakRef and invalidate it on the source) or a real copy
		// (in that case, we have a new objId/WeakRef).
		// If there is ever a need for a sane way, there should be two
		// distinct functions for both cases.
		// Right now, we only want to allow copying objects which are not
		// registered by an objId. E.g. CustomVar objects.
		errors << "BaseObject copy: the source has a specific objId" << endl;
	return *this;
}

BaseObject::~BaseObject() {
	thisRef.obj.overwriteShared(NULL);
}
