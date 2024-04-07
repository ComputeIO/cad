/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gecko_keys.h>
#include <wx/defs.h>


namespace GeckoKeys
{

unsigned int ScanCodeFromKeyEvent( uint32_t aRawKeyCode, uint32_t aRawKeyFlags )
{
#if defined( _WIN32 )

    uint16_t scanCode = ( aRawKeyFlags >> 16 ) & 0xFF;

    if( ( aRawKeyFlags & 0x1000000 ) != 0 ) // Extended
    {
        return ( 0xE000 | scanCode );
    }

    return scanCode;
#elif defined( __APPLE__ )
    return aRawKeyCode;
#else
    return aRawKeyFlags;
#endif
}


CodeNameIndex CodeNameIndexFromScanCode( unsigned int aScanCode )
{
    // clang-format off
    switch (aScanCode) {
#define NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX(aNativeKey, aCodeNameIndex) \
    case aNativeKey:                                                     \
      return aCodeNameIndex;

#include "NativeKeyToDOMCodeName.h"

#undef NS_NATIVE_KEY_TO_DOM_CODE_NAME_INDEX

      default:
          break;
    }
    // clang-format on

    return CODE_NAME_INDEX_UNKNOWN;
}


int WXKFromCodeNameIndex( CodeNameIndex aCodeNameIndex )
{
    // clang-format off
    switch (aCodeNameIndex) {
#define NS_WX_KEY_TO_DOM_CODE_NAME_INDEX(aWXKey, aCPPCodeName) \
    case aCPPCodeName:                    \
      return aWXKey;

#include "NativeKeyToDOMCodeName.h"

#undef NS_WX_KEY_TO_DOM_CODE_NAME_INDEX

      default:
          break;
    }
    // clang-format on

    return WXK_NONE;
}


CodeNameIndex CodeNameIndexFromWXK( int aWXKey )
{
    // clang-format off
    switch (aWXKey) {
#define NS_WX_KEY_TO_DOM_CODE_NAME_INDEX(aWXKey, aCPPCodeName) \
    case aWXKey:                          \
      return aCPPCodeName;

#include "NativeKeyToDOMCodeName.h"

#undef NS_WX_KEY_TO_DOM_CODE_NAME_INDEX

      default:
          break;
    }
    // clang-format on

    return CODE_NAME_INDEX_UNKNOWN;
}


int WXKFromKeyEvent( uint32_t aRawKeyCode, uint32_t aRawKeyFlags )
{
    return WXKFromCodeNameIndex(
            CodeNameIndexFromScanCode( ScanCodeFromKeyEvent( aRawKeyCode, aRawKeyFlags ) ) );
}


}
