#include "ItemAcquiredToastRow.h"
#include "ItemAcquiredToastEntry.h"
#include "CommonTextBlock.h"

void UItemAcquiredToastRow::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	UItemAcquiredToastEntry* Entry =
		Cast<UItemAcquiredToastEntry>(ListItemObject);

	if (!Entry)
		return;

	ItemNameAndQtyWidget->SetText(Entry->ItemText);
}
