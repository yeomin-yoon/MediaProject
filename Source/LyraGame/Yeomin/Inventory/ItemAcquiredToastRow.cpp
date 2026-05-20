#include "ItemAcquiredToastRow.h"
#include "ItemAcquiredToastEntry.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"

#include "CommonTextBlock.h"
#include "Styling/SlateColor.h"

void UItemAcquiredToastRow::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	UItemAcquiredToastEntry* Entry =
		Cast<UItemAcquiredToastEntry>(ListItemObject);

	if (!Entry)
		return;

	// =========================
	// 텍스트
	// =========================
	ItemNameAndQtyWidget->SetText(Entry->ItemText);

	// =========================
	// 핵심: SlateColor로 적용 (CommonTextBlock 대응)
	// =========================
	ItemNameAndQtyWidget->SetColorAndOpacity(
		FSlateColor(Entry->RarityColor)
	);

	// =========================
	// 아이콘
	// =========================
	if (ItemIconWidget && Entry->Icon)
	{
		ItemIconWidget->SetBrushFromTexture(Entry->Icon);
	}
}