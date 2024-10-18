#pragma once

#include "app/gui.h"

#include "core/string_view.h"

#include "misc/string_util.h"

using namespace soul;

namespace khaos
{
  void dialog_text(NotNull<app::Gui*> gui, StringView text);
} // namespace khaos
