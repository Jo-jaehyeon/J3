#include "Data/J3CsvTable.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"


namespace J3CsvTablePrivate
{
	// 줄 번호가 없는(표 전체에 대한) 메시지는 "파일: 내용"으로 쓴다
	FString FormatLocation(const FString& File, int32 Line, const FString& Message)
	{
		if (Line <= 0)
		{
			return FString::Printf(TEXT("%s: %s"), *File, *Message);
		}
		return FString::Printf(TEXT("%s:%d: %s"), *File, Line, *Message);
	}
}


void FJ3DataReport::AddError(const FString& File, int32 Line, const FString& Message)
{
	Errors.Add(J3CsvTablePrivate::FormatLocation(File, Line, Message));
}


void FJ3DataReport::AddWarning(const FString& File, int32 Line, const FString& Message)
{
	Warnings.Add(J3CsvTablePrivate::FormatLocation(File, Line, Message));
}


bool FJ3CsvTable::Load(const FString& FilePath, FJ3DataReport& Report, FString& OutRawContent)
{
	// 파일 읽기
	FileName = FPaths::GetCleanFilename(FilePath);
	ColumnIndices.Reset();
	Rows.Reset();
	if (!FFileHelper::LoadFileToString(OutRawContent, *FilePath))
	{
		Report.AddError(FileName, 0, TEXT("파일을 읽을 수 없습니다"));
		return false;
	}


	// 헤더 → 열 번호
	const FCsvParser Parser(OutRawContent);
	const FCsvParser::FRows& ParsedRows = Parser.GetRows();
	if (ParsedRows.Num() == 0)
	{
		Report.AddError(FileName, 1, TEXT("헤더가 없습니다"));
		return false;
	}
	for (int32 Col = 0; Col < ParsedRows[0].Num(); ++Col)
	{
		const FString Header = FString(ParsedRows[0][Col]).TrimStartAndEnd();
		ColumnIndices.Add(FName(*Header), Col);
	}


	// 데이터 행. 모든 칸이 빈 줄(파일 끝의 빈 줄 등)은 건너뛴다.
	for (int32 RowIndex = 1; RowIndex < ParsedRows.Num(); ++RowIndex)
	{
		FRow Row;
		Row.Line = RowIndex + 1;
		bool bHasValue = false;
		for (const TCHAR* Cell : ParsedRows[RowIndex])
		{
			FString Value = FString(Cell).TrimStartAndEnd();
			bHasValue |= !Value.IsEmpty();
			Row.Cells.Add(MoveTemp(Value));
		}
		if (bHasValue)
		{
			Rows.Add(MoveTemp(Row));
		}
	}
	return true;
}


bool FJ3CsvTable::RequireColumns(const TArray<FName>& Columns, FJ3DataReport& Report) const
{
	bool bAllFound = true;
	for (const FName& Column : Columns)
	{
		if (!ColumnIndices.Contains(Column))
		{
			Report.AddError(FileName, 1, FString::Printf(TEXT("필수 열 '%s'이(가) 없습니다"), *Column.ToString()));
			bAllFound = false;
		}
	}
	return bAllFound;
}


FString FJ3CsvTable::Get(const FRow& Row, FName Column) const
{
	const int32* Index = ColumnIndices.Find(Column);
	if (Index == nullptr || !Row.Cells.IsValidIndex(*Index))
	{
		return FString();
	}
	return Row.Cells[*Index];
}
