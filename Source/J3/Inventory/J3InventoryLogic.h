// Copyright (c) ACCENT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/J3InventoryTypes.h"


// 인벤토리 규칙만 담은 순수 함수 모음. 네트워크·UObject와 분리해 자동 테스트로 검증한다.
// 스택 크기는 데이터에서 오므로 호출하는 쪽이 함수로 넘긴다.
namespace J3InventoryLogic
{
	using FStackSizeFn = TFunctionRef<int32(FName)>;

	// 같은 아이템 칸부터 채우고 남으면 빈 칸에 넣는다. 못 넣은 개수를 돌려준다.
	J3_API int32 Add(TArray<FJ3ItemStack>& Slots, FName ItemId, int32 Count, FStackSizeFn StackSize);

	J3_API int32 CountOf(const TArray<FJ3ItemStack>& Slots, FName ItemId);

	// 전부 있을 때만 뺀다 (일부만 빼지 않음)
	J3_API bool Remove(TArray<FJ3ItemStack>& Slots, FName ItemId, int32 Count);

	J3_API int32 FindFirstEmpty(const TArray<FJ3ItemStack>& Slots);

	// 드래그 이동: 같은 아이템이면 스택 한도까지 합치고, 아니면 자리를 바꾼다
	J3_API bool Move(TArray<FJ3ItemStack>& Slots, int32 From, int32 To, FStackSizeFn StackSize);

	// 마우스 휠 순환: 맨손 → 1 → 2 → 3 → 4 → 맨손. 빈 장비 칸은 건너뛴다.
	// Direction은 +1(다음) 또는 -1(이전).
	J3_API int32 NextActiveIndex(const TArray<FJ3ItemStack>& Equipment, int32 CurrentIndex, int32 Direction);
}
