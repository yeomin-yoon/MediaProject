// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryItemToolTipUI.h"

#include "CommonBorder.h"
#include "CommonTextBlock.h"
#include "InventoryFragment_EquipEffect.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"

void UInventoryItemToolTipUI::Setup(
	ULyraInventoryItemInstance* Item)
{
	if (!Item)
		return;

	const ULyraInventoryItemDefinition* Def =
		GetDefault<ULyraInventoryItemDefinition>(
			Item->GetItemDef());

	if (!Def)
		return;

	const UInventoryFragment_EquipEffect* Frag =
		Cast<UInventoryFragment_EquipEffect>(
			Def->FindFragmentByClass(
				UInventoryFragment_EquipEffect::StaticClass()));

	if (!Frag)
		return;

	// =========================
	// 랜덤 수치 계산
	// =========================
	float Value = Frag->RollRandomValue(Item);

	// =========================
	// 옵션 + 스탯 색
	// =========================
	FString OptionName;
	FLinearColor StatColor = FLinearColor::White;

	switch (Item->OptionType)
	{
	case EItemOptionType::Attack:
		OptionName = TEXT("Attack Power");
		StatColor = FLinearColor(1.0f, 0.25f, 0.25f);
		break;

	case EItemOptionType::Health:
		OptionName = TEXT("Health");
		StatColor = FLinearColor(1.0f, 0.85f, 0.2f);
		break;

	case EItemOptionType::Stamina:
		OptionName = TEXT("Stamina");
		StatColor = FLinearColor(0.3f, 0.65f, 1.0f);
		break;
	}

	// =========================
	// 희귀도 문자열 + 색
	// =========================
	FString RarityString;
	FLinearColor RarityColor = FLinearColor::White;

	switch (Item->Rarity)
	{
	case EItemRarity::Common:
		RarityString = TEXT("Common");
		RarityColor = FLinearColor(0.65f, 0.65f, 0.65f);
		break;

	case EItemRarity::Uncommon:
		RarityString = TEXT("Uncommon");
		RarityColor = FLinearColor(0.25f, 0.55f, 1.0f);
		break;

	case EItemRarity::Rare:
		RarityString = TEXT("Rare");
		RarityColor = FLinearColor(0.7f, 0.35f, 1.0f);
		break;

	case EItemRarity::Epic:
		RarityString = TEXT("Epic");
		RarityColor = FLinearColor(1.0f, 0.82f, 0.2f);
		break;
	}

	// =========================
	// UI 텍스트
	// =========================
	ItemNameText->SetText(
		FText::FromString(
			FString::Printf(TEXT("Shard of %s"), *OptionName)));

	StatText->SetText(
		FText::FromString(
			FString::Printf(TEXT("+%.2f %s"), Value, *OptionName)));

	RarityText->SetText(FText::FromString(RarityString));

	// =========================
	// 설명
	// =========================
	FString StoryText;

	switch (Item->OptionType)
	{
	case EItemOptionType::Attack:
		StoryText = TEXT("전투의 열망이 깃든 파편.\n공격성을 증폭시킨다.");
		break;

	case EItemOptionType::Health:
		StoryText = TEXT("생명의 기운이 응축된 파편.\n생존력을 강화한다.");
		break;

	case EItemOptionType::Stamina:
		StoryText = TEXT("끊임없는 움직임의 잔재.\n지구력을 부여한다.");
		break;
	}

	ItemStoryText->SetText(FText::FromString(StoryText));

	// =========================
	// 색 적용
	// =========================
	RarityText->SetColorAndOpacity(RarityColor);
	ItemNameText->SetColorAndOpacity(FLinearColor::White);
	StatText->SetColorAndOpacity(StatColor);

	// =========================
	// HeaderBorder
	// =========================
	if (HeaderBorder)
	{
		HeaderBorder->SetBrushColor(
			FLinearColor(
				RarityColor.R,
				RarityColor.G,
				RarityColor.B,
				0.25f
			)
		);
	}
}