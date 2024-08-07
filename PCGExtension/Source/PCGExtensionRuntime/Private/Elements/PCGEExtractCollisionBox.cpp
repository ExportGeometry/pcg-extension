// Copyright © Mason Stevenson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
// THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Elements/PCGEExtractCollisionBox.h"

#include "PCGParamData.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "Metadata/Accessors/PCGAttributeAccessorHelpers.h"
#include "PhysicsEngine/BodySetup.h"

#define LOCTEXT_NAMESPACE "PCGEExtractCollisionBoxElement"

TArray<FPCGPinProperties> UPCGEExtractCollisionBoxSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	FPCGPinProperties& InputProperty = PinProperties.Emplace_GetRef(PCGPinConstants::DefaultInputLabel, EPCGDataType::Param, false);
	InputProperty.SetRequiredPin();
	
	return PinProperties;
}

TArray<FPCGPinProperties> UPCGEExtractCollisionBoxSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point);

	return PinProperties;
}

FPCGElementPtr UPCGEExtractCollisionBoxSettings::CreateElement() const
{
	return MakeShared<FPCGEExtractCollisionBoxElement>();
}

bool FPCGEExtractCollisionBoxElement::PrepareDataInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGEExtractCollisionBoxElement::Execute);

	check(Context);
	auto* ThisContext = static_cast<FPCGEExtractCollisionBoxContext*>(Context);

	if (ThisContext->WasLoadRequested())
	{
		return true;
	}

	const auto* Settings = ThisContext->GetInputSettings<UPCGEExtractCollisionBoxSettings>();
	check(Settings);

	TArray<FPCGTaggedData> Inputs = ThisContext->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	if (Inputs.IsEmpty())
	{
		return true;
	}

	const FPCGTaggedData& Input = Inputs[0];
	if (!Input.Data)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("InputMissing", "Input data is missing."));
		return true;
	}
	const FPCGAttributePropertyInputSelector AttributeSelector = Settings->MeshAttribute.CopyAndFixLast(Input.Data);
	const TUniquePtr<const IPCGAttributeAccessor> Accessor = PCGAttributeAccessorHelpers::CreateConstAccessor(Input.Data, AttributeSelector);
	const TUniquePtr<const IPCGAttributeAccessorKeys> Keys = PCGAttributeAccessorHelpers::CreateConstKeys(Input.Data, AttributeSelector);

	if (!Accessor)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidAccessor", "Unable to access MeshAttribute. Check that you are referencing a valid attribute."));
		return true;
	}
	if (!Keys)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidKeys", "Invalid Keys."));
		return true;
	}
	
	TArray<FSoftObjectPath> MeshPaths;
	MeshPaths.SetNum(Keys->GetNum());

	TArray<const PCGMetadataEntryKey*> InputKeys;
	InputKeys.SetNum(Keys->GetNum());
	TArrayView<const PCGMetadataEntryKey*> InputKeysView(InputKeys);
	Keys->GetKeys<PCGMetadataEntryKey>(0, InputKeysView);

	// Note that GetRange is fetching a *copy* of the data and assigning it into MeshPaths. I'm not really sure what the
	// point of using an ArrayView is here.
	if (!Accessor->GetRange<FSoftObjectPath>(MeshPaths, 0, *Keys))
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("AccessorFailed", "Accessor failed to load attribute data."));
		return true;
	}

	if (MeshPaths.IsEmpty())
	{
		return true;
	}

	for (const PCGMetadataEntryKey* KeyPtr : InputKeys)
	{
		ThisContext->InputKeys.Add(*KeyPtr);
	}
	ThisContext->MeshPaths = MeshPaths;
	
	return ThisContext->RequestResourceLoad(ThisContext, std::move(MeshPaths), !Settings->bSynchronousLoad);
}

bool FPCGEExtractCollisionBoxElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGEExtractCollisionBoxElement::Execute);

	check(Context)
	auto* ThisContext = static_cast<FPCGEExtractCollisionBoxContext*>(Context);
	
	const auto* InputSettings = ThisContext->GetInputSettings<UPCGEExtractCollisionBoxSettings>();
	check(InputSettings);

	TArray<FPCGTaggedData> Inputs = ThisContext->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	
	TArray<FPCGTaggedData>& Outputs = ThisContext->OutputData.TaggedData;
	UPCGPointData* OutPointData = nullptr;
	
	if (Inputs.IsEmpty())
	{
		return true;
	}
	if (ThisContext->InputKeys.IsEmpty())
	{
		return true;
	}
	if (ThisContext->InputKeys.Num() != ThisContext->MeshPaths.Num())
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("InputCountMissmatch", "Expected InputKeys and MeshPaths to have the same count."));
		return false;
	}

	// Only one input is allowed.
	FPCGTaggedData& InputData = Inputs[0];
		
	auto* ParamData = Cast<UPCGParamData>(InputData.Data);
	if (!ParamData)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("IncorrectInputData", "Input data is incorrect type (expected Param Data)"));
		return false;
	}

	for (int32 MetadataIndex = 0; MetadataIndex < ThisContext->InputKeys.Num(); MetadataIndex++)
	{
		PCGMetadataEntryKey InputKey = ThisContext->InputKeys[MetadataIndex];
		FSoftObjectPath& ObjectPath = ThisContext->MeshPaths[MetadataIndex];

		const UObject* Object = ObjectPath.ResolveObject();
		if(!Object)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("ObjectLoadFailed", "Failed to load object"));
			continue;
		}
		const auto* MeshPtr = Cast<UStaticMesh>(Object);
		if (!MeshPtr)
		{
			PCGE_LOG(Error, GraphAndLog, LOCTEXT("ObjectWrongType", "Object is not of type UStaticMesh"));
			continue;
		}

		TArray<FKBoxElem> CollisionBoxes = MeshPtr->GetBodySetup()->AggGeom.BoxElems;

		if (CollisionBoxes.IsEmpty())
		{
			PCGE_LOG(Warning, GraphAndLog, LOCTEXT("HasNoCollisionBoxes", "Mesh has no collision boxes."));
			continue;
		}

		FKBoxElem CollisionBox;
		bool FoundBox = false;
		
		if (!InputSettings->CollisionBoxName.IsNone())
		{
			for (FKBoxElem BoxElem : CollisionBoxes)
			{
				if (BoxElem.GetName().IsEqual(InputSettings->CollisionBoxName))
				{
					CollisionBox = BoxElem;
					FoundBox = true;
					break;
				}
			}
		}
		else
		{
			// just grab the first box we can find
			CollisionBox = CollisionBoxes[0];
			FoundBox = true;
		}

		// Only create the output data if there is a data point to actually add.
		if (!OutPointData)
		{
			OutPointData = NewObject<UPCGPointData>();
			
			// This node does a passthrough of whatever attribute data you pass into it.
			// Here we just copy attribute descriptions only. Values are copied below (see OutMetadata->SetAttributes).
			OutPointData->Metadata->AddAttributes(ParamData->Metadata);
		}
		
		if (FoundBox)
		{
			FPCGPoint& NewPoint = OutPointData->GetMutablePoints().Emplace_GetRef();
			
			NewPoint.Transform = CollisionBox.GetTransform();
			FBox Bounds = BoxElemToFBox(CollisionBox);
			NewPoint.BoundsMin = Bounds.Min;
			NewPoint.BoundsMax = Bounds.Max;

			// Copies all the attribute data from the input into the new point.
			UPCGMetadata* OutMetadata = OutPointData->MutableMetadata();
			NewPoint.MetadataEntry = OutMetadata->AddEntry();
			OutMetadata->SetAttributes(InputKey, ParamData->Metadata, NewPoint.MetadataEntry);
		}
		else
		{
			PCGE_LOG(Warning, GraphAndLog, LOCTEXT("CollisionBoxMissing", "Couldn't find collision box."));
		}
	}
	
	if (OutPointData)
	{
		Outputs.Emplace_GetRef().Data = OutPointData;
	}

	return true;
}

FBox FPCGEExtractCollisionBoxElement::BoxElemToFBox(FKBoxElem BoxElem) const
{
	FVector3f Extent(0.5f * BoxElem.X, 0.5f * BoxElem.Y, 0.5f * BoxElem.Z);
	return FBox(-Extent, Extent);
}


#undef LOCTEXT_NAMESPACE