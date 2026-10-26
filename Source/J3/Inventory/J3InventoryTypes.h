// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "J3InventoryTypes.generated.h"


// 인벤토리·장비 칸 하나. ItemId가 None이면 빈 칸이다.
USTRUCT(BlueprintType)
struct FJ3ItemStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "J3|Inventory")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "J3|Inventory")
	int32 Count = 0;

	bool IsEmpty() const
	{
		return ItemId.IsNone() || Count <= 0;
	}

	void Clear()
	{
		ItemId = NAME_None;
		Count = 0;
	}

	bool operator==(const FJ3ItemStack& Other) const
	{
		return ItemId == Other.ItemId && Count == Other.Count;
	}
};


// 장비 칸 순환에서 "맨손"을 나타내는 번호
inline constexpr int32 J3BareHandIndex = INDEX_NONE;
