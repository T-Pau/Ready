//
//  MediaType.swift
//  RetroMedia
//
//  Created by Dieter Baron on 07.07.20.
//  Copyright © 2020 Spiderlab. All rights reserved.
//

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
