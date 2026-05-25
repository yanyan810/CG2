#include "FieldUi.h"

// FieldUi implementation is split by responsibility under Game/UI/FieldUi/:
// - FieldUiCore.cpp: utility, layout application, sprite setup, debug setters
// - FieldUiUpdate.cpp: battle-state driven UI update
// - FieldUiDraw.cpp: rendering
// - FieldUiDebug.cpp: ImGui editor/debug UI
// - FieldUiLayoutIO.cpp: JSON layout load/save
