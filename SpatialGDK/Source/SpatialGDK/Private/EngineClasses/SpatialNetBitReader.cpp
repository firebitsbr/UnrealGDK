// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "EngineClasses/SpatialNetBitReader.h"

#include "UObject/WeakObjectPtr.h"

#include "EngineClasses/SpatialPackageMapClient.h"
#include "SpatialConstants.h"

DEFINE_LOG_CATEGORY(LogSpatialNetBitReader);

FSpatialNetBitReader::FSpatialNetBitReader(USpatialPackageMapClient* InPackageMap, uint8* Source, int64 CountBits, TSet<FUnrealObjectRef>& InUnresolvedRefs)
	: FNetBitReader(InPackageMap, Source, CountBits)
	, UnresolvedRefs(InUnresolvedRefs) {}

void FSpatialNetBitReader::DeserializeObjectRef(FUnrealObjectRef& ObjectRef)
{
	int64 EntityId;
	*this << EntityId;
	ObjectRef.Entity = EntityId;
	*this << ObjectRef.Offset;

	uint8 HasPath;
	SerializeBits(&HasPath, 1);
	if (HasPath)
	{
		FString Path;
		*this << Path;

		ObjectRef.Path = Path;
	}

	uint8 HasOuter;
	SerializeBits(&HasOuter, 1);
	if (HasOuter)
	{
		ObjectRef.Outer = FUnrealObjectRef();
		DeserializeObjectRef(*ObjectRef.Outer);
	}

	SerializeBits(&ObjectRef.bNoLoadOnClient, 1);
	SerializeBits(&ObjectRef.bUseSingletonClassPath, 1);
}

FArchive& FSpatialNetBitReader::operator<<(UObject*& Value)
{
	FUnrealObjectRef ObjectRef;

	DeserializeObjectRef(ObjectRef);
	check(ObjectRef != FUnrealObjectRef::UNRESOLVED_OBJECT_REF);

	bool bUnresolved = false;
	Value = FUnrealObjectRef::ToObjectPtr(ObjectRef, Cast<USpatialPackageMapClient>(PackageMap), bUnresolved);

	if (bUnresolved)
	{
		UnresolvedRefs.Add(ObjectRef);
	}

	return *this;
}

FArchive& FSpatialNetBitReader::operator<<(FWeakObjectPtr& Value)
{
	UObject* Object;
	*this << Object;

	Value = Object;

	return *this;
}
