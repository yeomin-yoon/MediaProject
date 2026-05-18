#include "ItemAcquiredToastRow.h"
#include "ItemAcquiredToastEntry.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"

void UItemAcquiredToastRow::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	UItemAcquiredToastEntry* Entry =
		Cast<UItemAcquiredToastEntry>(ListItemObject);

	if (!Entry)
		return;

	ItemNameAndQtyWidget->SetText(Entry->ItemText);

	// 아이콘 적용
	if (ItemIconWidget && Entry->Icon)
	{
		ItemIconWidget->SetBrushFromTexture(Entry->Icon);
	}
}
