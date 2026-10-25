#pragma once

#include "CoreMinimal.h"


// 데이터 읽기·검증 중 나온 오류와 경고. 오류가 하나라도 있으면 그 데이터는 반영하지 않는다.
struct FJ3DataReport
{
	TArray<FString> Errors;
	TArray<FString> Warnings;

	void AddError(const FString& File, int32 Line, const FString& Message);
	void AddWarning(const FString& File, int32 Line, const FString& Message);

	bool HasErrors() const
	{
		return Errors.Num() > 0;
	}
};


// 헤더가 있는 CSV 파일 하나. 열은 헤더 이름으로 찾는다.
class FJ3CsvTable
{
public:
	struct FRow
	{
		// 원본 파일의 줄 번호 (1부터, 헤더가 1줄)
		int32 Line = 0;
		TArray<FString> Cells;
	};

	// 파일을 읽고 파싱한다. 원본 내용은 데이터 해시 계산용으로 돌려준다.
	bool Load(const FString& FilePath, FJ3DataReport& Report, FString& OutRawContent);

	// 필요한 열이 모두 헤더에 있는지 검사한다.
	bool RequireColumns(const TArray<FName>& Columns, FJ3DataReport& Report) const;

	// 열 값 (앞뒤 공백 제거). 열이 없거나 칸이 모자라면 빈 문자열.
	FString Get(const FRow& Row, FName Column) const;

	const TArray<FRow>& GetRows() const
	{
		return Rows;
	}

	const FString& GetFileName() const
	{
		return FileName;
	}

private:
	FString FileName;
	TMap<FName, int32> ColumnIndices;
	TArray<FRow> Rows;
};
