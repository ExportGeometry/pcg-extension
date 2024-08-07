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

#pragma once

#include "PCGContext.h"
#include "PCGSettings.h"
#include "Async/PCGAsyncLoadingContext.h"
#include "Metadata/PCGMetadataCommon.h"

#include "UObject/SoftObjectPtr.h"

#include "PCGEExtractCollisionBox.generated.h"

struct FKBoxElem;

/**
 * Extracts collision box data from a collection of static meshes.
 */
UCLASS(BlueprintType, ClassGroup = (Procedural))
class UPCGEExtractCollisionBoxSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("ExtractCollisionBoxes")); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("PCGEExtractCollisionBoxElement", "NodeTitle", "Extract Collision Boxes"); }
	virtual FText GetNodeTooltipText() const override { return NSLOCTEXT("PCGEExtractCollisionBoxElement", "NodeTooltip", "Extracts collision box data from a collection of static meshes."); }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	/** Describes the attribute on the input data that contains the mesh that should have its collision box extracted. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FPCGAttributePropertyInputSelector MeshAttribute;

	/** The name of the collision box to use. If None, the first available collision box will be used. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FName CollisionBoxName;
	
	/** By default, mesh loading is asynchronous, can force it synchronous if needed. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Debug")
	bool bSynchronousLoad = false;
};

struct FPCGEExtractCollisionBoxContext : public FPCGContext, public IPCGAsyncLoadingContext
{
public:
	TArray<PCGMetadataEntryKey> InputKeys;
	TArray<FSoftObjectPath> MeshPaths;
};

class FPCGEExtractCollisionBoxElement : public IPCGElement
{
public:
	// Accessing objects outside of PCG is not guaranteed to be thread safe
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override { return true; }

	virtual FPCGContext* CreateContext() override { return new FPCGEExtractCollisionBoxContext(); }
	
protected:
	virtual bool PrepareDataInternal(FPCGContext* Context) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
	
	FBox BoxElemToFBox(FKBoxElem BoxElem) const;
};