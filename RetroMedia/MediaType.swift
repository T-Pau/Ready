/*
 MediaType.swift -- Supported Media Types
 Copyright (C) 2020 Dieter Baron
 
 The author can be contacted at <dillo@tpau.group>
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 
 2. The names of the authors may not be used to endorse or promote
 products derived from this software without specific prior
 written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

import Foundation

/*
 Type names consist of up to four parts:
 - media type (cartridge, disk, tape)
 - pysical media format (e.g. 3.5" DD disk)
 - data format
 
 */
public enum MediaType {
    case cartridgeCommodore64
    case cartridgeCommodorePlus4
    case cartridgeCommodoreVIC20
    
    case cassetteAtari
    case cassetteCommodore
    case cassetteSpectrum
    
    case compactFlashIde64
    
    case cd
    
    case disk3_5DoubleDensityCmd
    case disk3_5DoubleDensityCommodore
    case disk3_5HighDensityCmd
    case disk3_5ExtendedDensityCmd
    case disk5_25SingleDensitySingleSidedCommodore
    case disk5_25SingleDensityDoubleSidedCommodore
    case disk5_25DoubleDensitySingleSidedCommodore
    case disk5_25DoubleDensityDoulbeSidedCommodore
    
    case harddiskIdeIde64
}
