/*
 * Copyright (c) 2018, Adrian Chemicz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "ugui.h"
#include <stdint.h>

/*
 * This module contain all images used by uGui library. 8-bit picture is stored in format
 * 3 pixel red, 3 pixel green, 2 pixel blue.
 */

extern const UG_BMP homePictureOutside;
extern const UG_BMP homePictureInside;
extern const UG_BMP wifiPicture;
extern const UG_BMP furnacePicture;
extern const UG_BMP optionsPicture;
extern const UG_BMP closePicture;
extern const UG_BMP incrementMinuteHourPicture;
extern const UG_BMP decrementMinuteHourPicture;
extern const UG_BMP smallIncrementPicture;
extern const UG_BMP smallDecrementPicture;
extern const UG_BMP moveHistoryGraphLeftPicture;
extern const UG_BMP moveHistoryGraphRightPicture;
extern const UG_BMP upWifiNetwork;
extern const UG_BMP downWifiNetwork;

#endif /* _IMAGE_H_ */
