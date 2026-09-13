// Copyright 2026 Jim Jenkins. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "WindowWidget.generated.h"

class UTextBlock;
class UButton;

/** Broadcast when a window closes itself (its own close button), so whoever opened it can react */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWindowClosedDelegate, UWindowWidget*, Window);

/**
 *  Reusable floating-window chrome for a UUserWidget: an optional title bar with a close
 *  button, plus drag-to-move and resize-from-corner. All parts are optional so a WBP that
 *  doesn't provide them simply doesn't get that behavior.
 */
UCLASS(abstract)
class SMORESUI_API UWindowWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** Optional title bar text. Name it "TitleText" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** Optional close ("X") button. Name it "CloseButton" in the WBP to auto-bind. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/**
	 *  Optional drag region - must be a widget whose geometry covers the title bar.
	 *  Dragging only starts when a mouse-down lands within this widget. Name it
	 *  "TitleBarDragHandle" in the WBP to auto-bind.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TitleBarDragHandle;

	/**
	 *  Optional resize grip, typically a small widget in the bottom-right corner.
	 *  Name it "ResizeHandle" in the WBP to auto-bind.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ResizeHandle;

	/** Text applied to TitleText (if bound) on construct */
	UPROPERTY(EditAnywhere, Category = "Window")
	FText WindowTitle;

	/** Initial floating-window position (top-left), in viewport space */
	UPROPERTY(EditAnywhere, Category = "Window")
	FVector2D InitialWindowPosition = FVector2D(100.f, 100.f);

	/** Initial floating-window size, in viewport space */
	UPROPERTY(EditAnywhere, Category = "Window")
	FVector2D InitialWindowSize = FVector2D(420.f, 320.f);

	/** Smallest size the window can be resized to */
	UPROPERTY(EditAnywhere, Category = "Window")
	FVector2D MinWindowSize = FVector2D(220.f, 160.f);

	/** If false, TitleBarDragHandle is ignored even if bound */
	UPROPERTY(EditAnywhere, Category = "Window")
	bool bAllowDrag = true;

	/** If false, ResizeHandle is ignored even if bound */
	UPROPERTY(EditAnywhere, Category = "Window")
	bool bAllowResize = true;

private:

	bool bIsDragging = false;
	bool bIsResizing = false;
	FVector2D GestureStartScreenPos = FVector2D::ZeroVector;
	FGameViewportWidgetSlot GestureStartSlot;

public:

	/** Sets the title text at runtime (no-op if TitleText isn't bound) */
	UFUNCTION(BlueprintCallable, Category = "Window")
	void SetWindowTitle(const FText& NewTitle);

	/**
	 *  Fired when this window closes itself. The owner that spawned it can't see its close
	 *  button, so without this a window shut that way leaves whatever was opened alongside it
	 *  (a pawn's paperdoll beside its pack) stranded, and leaves input scoped to a window that
	 *  is no longer there.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Window")
	FOnWindowClosedDelegate OnWindowClosed;

	/**
	 *  Requests that this window close. The base implementation only announces it - subclasses
	 *  override to actually remove/hide themselves, and should call Super *after* doing so, so
	 *  listeners see a window that has already gone.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Window")
	void RequestClose();
	virtual void RequestClose_Implementation();

protected:

	UFUNCTION()
	void HandleCloseButtonClicked();

	//~ Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End UUserWidget interface

private:

	static bool IsUnderWidget(const UWidget* Widget, const FVector2D& ScreenSpacePosition);
};
