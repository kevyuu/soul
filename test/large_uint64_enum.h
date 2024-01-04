#pragma once

#include "core/flag_set.h"

#include "builtins.h"

enum class LargeUint64TestEnum : u64
{
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  SEVEN,
  EIGHT,
  NINE,
  TEN,
  ELEVEN,
  TWELVE,
  THIRTEEN,
  FOURTEEN,
  FIFTEEN,
  SIXTEEN,
  SEVENTEEN,
  EIGHTEEN,
  NINETEEN,
  TWENTY,
  TWENTY_ONE,
  TWENTY_TWO,
  TWENTY_THREE,
  TWENTY_FOUR,
  TWENTY_FIVE,
  TWENTY_SIX,
  TWENTY_SEVEN,
  TWENTY_EIGHT,
  TWENTY_NINE,
  THIRTY,
  THIRTY_ONE,
  THIRTY_TWO,
  THIRTY_THREE,
  THIRTY_FOUR,
  THIRTY_FIVE,
  THIRTY_SIX,
  THIRTY_SEVEN,
  THIRTY_EIGHT,
  THIRTY_NINE,
  FORTY,
  COUNT
};

using LargeUint64FlagSet = soul::FlagSet<LargeUint64TestEnum>;
