// Copyright 2024, Roberto De Ioris.


#include "glTFRuntimeLDrawFunctionLibrary.h"

struct FglTFRuntimeLDrawCacheData : FglTFRuntimePluginCacheData
{
	TMap<FString, TArray<TArray<FString>>> Parts;
	TArray<TArray<FString>> EmptyPart;
	TMap<int32, FVector4> ColorsMap;
};

namespace glTFRuntimeLDraw
{

	void FillLinesFromBlob(const TArray64<uint8>& Blob, TArray<TArray<FString>>& Lines)
	{
		TArray<FString> CurrentLine;
		TArray<uint8> CurrentString;

		for (int64 Index = 0; Index < Blob.Num(); Index++)
		{
			const uint8 Char = Blob[Index];
			if (Char == ' ' || Char == '\t' || Char == '\r' || Char == '\n')
			{
				if (CurrentString.Num() > 0)
				{
					CurrentString.Add(0);
					CurrentLine.Add(FString(UTF8_TO_TCHAR(CurrentString.GetData())).TrimStartAndEnd());
				}
				CurrentString.Empty();
				if (Char == '\r' || Char == '\n')
				{
					if (CurrentLine.Num() > 0)
					{
						Lines.Add(CurrentLine);
					}
					CurrentLine.Empty();
				}
			}
			else
			{
				CurrentString.Add(Char);
			}
		}

		if (CurrentString.Num() > 0)
		{
			CurrentString.Add(0);
			CurrentLine.Add(FString(UTF8_TO_TCHAR(CurrentString.GetData())).TrimStartAndEnd());
		}

		if (CurrentLine.Num() > 0)
		{
			Lines.Add(CurrentLine);
		}
	}

	const TArray<TArray<FString>>& GetLinesFromPart(TSharedRef<FglTFRuntimeLDrawCacheData> RuntimeLDrawCacheData, const FString& PartName)
	{
		if (RuntimeLDrawCacheData->Parts.Contains(PartName))
		{
			return RuntimeLDrawCacheData->Parts[PartName];
		}

		const TArray<FString> SearchDirs = { "models", "p", "parts" };
		for (const FString& SearchDir : SearchDirs)
		{
			const FString FullPath = FPaths::Combine("D:/Downloads/lego/ldraw", SearchDir, PartName);
			if (FPaths::FileExists(FullPath))
			{
				TArray64<uint8> PartBlob;
				if (FFileHelper::LoadFileToArray(PartBlob, *FullPath))
				{
					RuntimeLDrawCacheData->Parts.Add(PartName);
					FillLinesFromBlob(PartBlob, RuntimeLDrawCacheData->Parts[PartName]);
					return RuntimeLDrawCacheData->Parts[PartName];
				}
			}
		}

		UE_LOG(LogGLTFRuntime, Error, TEXT("Unable to find part %s"), *PartName);

		return RuntimeLDrawCacheData->EmptyPart;
	}

	void BuildPrimitiveFromLines(TSharedRef<FglTFRuntimeLDrawCacheData> RuntimeLDrawCacheData, const FString& PartName, const TArray<TArray<FString>>& Lines, const FTransform& Transform, const FVector4 CurrentColor, const bool bCurrentCCW, FglTFRuntimePrimitive& Primitive)
	{
		bool bCCW = bCurrentCCW;
		bool bInvertNext = false;
		for (const TArray<FString>& Line : Lines)
		{
			//UE_LOG(LogTemp, Error, TEXT("Line code: %s (size: %d)"), *(Line[0]), Line.Num());
			if (Line[0] == "1" && Line.Num() == 15)
			{
				FVector4 Color = FVector4(1, 1, 1, 1);
				const int32 ColorIndex = FCString::Atoi(*(Line[1]));
				if (ColorIndex == 16)
				{
					Color = CurrentColor;
				}
				else if (ColorIndex == 24)
				{
					Color = FVector4(0, 0, 0, 1);
				}
				else if (RuntimeLDrawCacheData->ColorsMap.Contains(ColorIndex))
				{
					Color = RuntimeLDrawCacheData->ColorsMap[ColorIndex];
				}

				FMatrix Matrix = FMatrix::Identity;

				Matrix.M[3][0] = FCString::Atod(*(Line[2]));
				Matrix.M[3][1] = FCString::Atod(*(Line[3]));
				Matrix.M[3][2] = FCString::Atod(*(Line[4]));

				Matrix.M[0][0] = FCString::Atod(*(Line[5]));
				Matrix.M[1][0] = FCString::Atod(*(Line[6]));
				Matrix.M[2][0] = FCString::Atod(*(Line[7]));

				Matrix.M[0][1] = FCString::Atod(*(Line[8]));
				Matrix.M[1][1] = FCString::Atod(*(Line[9]));
				Matrix.M[2][1] = FCString::Atod(*(Line[10]));

				Matrix.M[0][2] = FCString::Atod(*(Line[11]));
				Matrix.M[1][2] = FCString::Atod(*(Line[12]));
				Matrix.M[2][2] = FCString::Atod(*(Line[13]));

				const FQuat Quat = Matrix.ToQuat();
				FTransform PartTransform = FTransform(Matrix);

				PartTransform.SetRotation(Quat);

				const TArray<TArray<FString>> PartLines = GetLinesFromPart(RuntimeLDrawCacheData, Line[14]);
				if (PartLines.Num() > 0)
				{
					if (bInvertNext)
					{
						bCCW = !bCCW;
					}
					BuildPrimitiveFromLines(RuntimeLDrawCacheData, Line[14], PartLines, PartTransform * Transform, Color, bCCW, Primitive);
					if (bInvertNext)
					{
						bCCW = !bCCW;
						bInvertNext = false;
					}
				}
			}
			else if (Line[0] == "3" && Line.Num() == 11)
			{
				FVector4 Color = FVector4(1, 1, 1, 1);
				const int32 ColorIndex = FCString::Atoi(*(Line[1]));
				if (ColorIndex == 16)
				{
					Color = CurrentColor;
				}
				else if (ColorIndex == 24)
				{
					Color = FVector4(0, 0, 0, 1);
				}
				else if (RuntimeLDrawCacheData->ColorsMap.Contains(ColorIndex))
				{
					Color = RuntimeLDrawCacheData->ColorsMap[ColorIndex];
				}

				const double X0 = FCString::Atod(*(Line[2]));
				const double Y0 = FCString::Atod(*(Line[3]));
				const double Z0 = FCString::Atod(*(Line[4]));

				const double X1 = FCString::Atod(*(Line[5]));
				const double Y1 = FCString::Atod(*(Line[6]));
				const double Z1 = FCString::Atod(*(Line[7]));

				const double X2 = FCString::Atod(*(Line[8]));
				const double Y2 = FCString::Atod(*(Line[9]));
				const double Z2 = FCString::Atod(*(Line[10]));

				Primitive.Positions.Add(Transform.TransformPosition(FVector(X0, Y0, Z0)));
				if (bCCW)
				{
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X1, Y1, Z1)));
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X2, Y2, Z2)));
				}
				else
				{
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X2, Y2, Z2)));
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X1, Y1, Z1)));
				}

				for (int32 Index = 0; Index < 3; Index++)
				{
					Primitive.Indices.Add(Primitive.Indices.Num());
					Primitive.Colors.Add(Color);
				}
			}
			else if (Line[0] == "4" && Line.Num() == 14)
			{
				FVector4 Color = FVector4(1, 1, 1, 1);
				const int32 ColorIndex = FCString::Atoi(*(Line[1]));
				if (ColorIndex == 16)
				{
					Color = CurrentColor;
				}
				else if (ColorIndex == 24)
				{
					Color = FVector4(0, 0, 0, 1);
				}
				else if (RuntimeLDrawCacheData->ColorsMap.Contains(ColorIndex))
				{
					Color = RuntimeLDrawCacheData->ColorsMap[ColorIndex];
				}

				const double X0 = FCString::Atod(*(Line[2]));
				const double Y0 = FCString::Atod(*(Line[3]));
				const double Z0 = FCString::Atod(*(Line[4]));

				const double X1 = FCString::Atod(*(Line[5]));
				const double Y1 = FCString::Atod(*(Line[6]));
				const double Z1 = FCString::Atod(*(Line[7]));

				const double X2 = FCString::Atod(*(Line[8]));
				const double Y2 = FCString::Atod(*(Line[9]));
				const double Z2 = FCString::Atod(*(Line[10]));

				const double X3 = FCString::Atod(*(Line[11]));
				const double Y3 = FCString::Atod(*(Line[12]));
				const double Z3 = FCString::Atod(*(Line[13]));

				Primitive.Positions.Add(Transform.TransformPosition(FVector(X0, Y0, Z0)));
				if (bCCW)
				{
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X1, Y1, Z1)));
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X2, Y2, Z2)));
				}
				else
				{
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X2, Y2, Z2)));
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X1, Y1, Z1)));
				}


				Primitive.Positions.Add(Transform.TransformPosition(FVector(X3, Y3, Z3)));
				if (bCCW)
				{
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X0, Y0, Z0)));
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X2, Y2, Z2)));
				}
				else
				{
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X2, Y2, Z2)));
					Primitive.Positions.Add(Transform.TransformPosition(FVector(X0, Y0, Z0)));
				}

				for (int32 Index = 0; Index < 6; Index++)
				{
					Primitive.Indices.Add(Primitive.Indices.Num());
					Primitive.Colors.Add(Color);
				}
			}
			else if (Line[0] == "0" && Line.Num() > 1 && Line[1] == "BFC")
			{
				for (int32 Index = 2; Index < Line.Num(); Index++)
				{
					if (Line[Index] == "CW")
					{
						bCCW = false;
					}
					else if (Line[Index] == "CCW")
					{
						bCCW = true;
					}
					else if (Line[Index] == "INVERTNEXT")
					{
						bInvertNext = true;
					}
				}
			}
		}
	}

	TSharedPtr<FglTFRuntimeLDrawCacheData> GetCacheData(UglTFRuntimeAsset* Asset)
	{
		if (Asset->GetParser()->PluginsCacheData.Contains("LDraw"))
		{
			if (Asset->GetParser()->PluginsCacheData["LDraw"] && Asset->GetParser()->PluginsCacheData["LDraw"]->bValid)
			{
				return StaticCastSharedPtr<FglTFRuntimeLDrawCacheData>(Asset->GetParser()->PluginsCacheData["LDraw"]);
			}
		}

		if (!Asset->GetParser()->PluginsCacheData.Contains("LDraw"))
		{
			Asset->GetParser()->PluginsCacheData.Add("LDraw", MakeShared<FglTFRuntimeLDrawCacheData>());
		}

		TSharedPtr<FglTFRuntimeLDrawCacheData> RuntimeLDrawCacheData = StaticCastSharedPtr<FglTFRuntimeLDrawCacheData>(Asset->GetParser()->PluginsCacheData["LDraw"]);

		TArray64<uint8> ConfigBlob;
		if (FFileHelper::LoadFileToArray(ConfigBlob, TEXT("D:/Downloads/lego/ldraw/LDConfig.ldr")))
		{
			TArray<TArray<FString>> ColorLines;
			FillLinesFromBlob(ConfigBlob, ColorLines);

			for (const TArray<FString>& ColorLine : ColorLines)
			{
				if (ColorLine.Num() > 6 && ColorLine[0] == "0" && ColorLine[1] == "!COLOUR")
				{
					FString ColorCode;
					FString ColorRGB;
					FString ColorAlpha;
					for (int32 Index = 2; Index < ColorLine.Num(); Index++)
					{
						if (ColorLine[Index] == "CODE" && (Index + 1) < ColorLine.Num())
						{
							ColorCode = ColorLine[Index + 1];
							Index++;
							continue;
						}
						else if (ColorLine[Index] == "VALUE" && (Index + 1) < ColorLine.Num())
						{
							ColorRGB = ColorLine[Index + 1];
							Index++;
							continue;
						}
						else if (ColorLine[Index] == "ALPHA" && (Index + 1) < ColorLine.Num())
						{
							ColorAlpha = ColorLine[Index + 1];
							Index++;
							continue;
						}
					}

					if (!ColorCode.IsEmpty() && !ColorRGB.IsEmpty())
					{
						const int32 ColorKey = FCString::Atoi(*ColorCode);
						FVector4 ColorValue = FLinearColor(FColor::FromHex(ColorRGB));
						if (!ColorAlpha.IsEmpty())
						{
							ColorValue.W = 255.0 / FCString::Atoi(*ColorAlpha);
						}
						RuntimeLDrawCacheData->ColorsMap.Add(ColorKey, ColorValue);
					}
				}
			}
		}

		RuntimeLDrawCacheData->bValid = true;

		return RuntimeLDrawCacheData;
	}
}

bool UglTFRuntimeLDrawFunctionLibrary::LoadLDrawAsRuntimeLOD(UglTFRuntimeAsset* Asset, FglTFRuntimeMeshLOD& RuntimeLOD, const FglTFRuntimeMaterialsConfig& MaterialsConfig)
{
	if (!Asset)
	{
		return false;
	}

	TSharedPtr<FglTFRuntimeLDrawCacheData> RuntimeLDrawCacheData = glTFRuntimeLDraw::GetCacheData(Asset);

	if (!RuntimeLDrawCacheData)
	{
		return false;
	}

	const TArray64<uint8>& Blob = Asset->GetParser()->GetBlob();
	TArray<TArray<FString>> Lines;

	glTFRuntimeLDraw::FillLinesFromBlob(Blob, Lines);
	FglTFRuntimePrimitive Primitive;

	glTFRuntimeLDraw::BuildPrimitiveFromLines(RuntimeLDrawCacheData.ToSharedRef(), "", Lines, FTransform::Identity, FVector4(1, 1, 1, 1), true, Primitive);

	RuntimeLOD.Primitives.Add(MoveTemp(Primitive));

	return true;
}