/* debugger.h: the Win32 debugger
   Copyright (c) 2004 Marek Januszewski
   Copyright (c) 2015 Stuart Brady

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

/* FIXME: http://msdn.microsoft.com/en-us/library/t2zechd4%28v=VS.80%29.aspx */

#define IDD_DBG				2000
#define IDC_DBG_LV_PC			( IDD_DBG + 1 )
#define IDC_DBG_SB_PC			( IDD_DBG + 2 )
#define IDC_DBG_LV_STACK		( IDD_DBG + 3 )
#define IDC_DBG_LV_EVENTS		( IDD_DBG + 4 )
#define IDC_DBG_LV_BPS			( IDD_DBG + 5 )
#define IDC_DBG_ED_EVAL			( IDD_DBG + 6 )
#define IDC_DBG_BTN_EVAL		( IDD_DBG + 7 )
#define IDC_DBG_GRP_MEMMAP		( IDD_DBG + 8 )
#define IDC_DBG_BTN_STEP		( IDD_DBG + 9 )
#define IDC_DBG_BTN_CONT		( IDD_DBG + 10 )
#define IDC_DBG_BTN_BREAK		( IDD_DBG + 11 )

#define IDC_DBG_MAP11			( IDD_DBG + 12 )
#define IDC_DBG_MAP12			( IDC_DBG_MAP11 + 1 )
#define IDC_DBG_MAP13			( IDC_DBG_MAP12 + 1 )
#define IDC_DBG_MAP14			( IDC_DBG_MAP13 + 1 )
#define IDC_DBG_MAP21			( IDC_DBG_MAP14 + 1 )
#define IDC_DBG_MAP22			( IDC_DBG_MAP21 + 1 )
#define IDC_DBG_MAP23			( IDC_DBG_MAP22 + 1 )
#define IDC_DBG_MAP24			( IDC_DBG_MAP23 + 1 )
#define IDC_DBG_MAP31			( IDC_DBG_MAP24 + 1 )
#define IDC_DBG_MAP32			( IDC_DBG_MAP31 + 1 )
#define IDC_DBG_MAP33			( IDC_DBG_MAP32 + 1 )
#define IDC_DBG_MAP34			( IDC_DBG_MAP33 + 1 )
#define IDC_DBG_MAP41			( IDC_DBG_MAP34 + 1 )
#define IDC_DBG_MAP42			( IDC_DBG_MAP41 + 1 )
#define IDC_DBG_MAP43			( IDC_DBG_MAP42 + 1 )
#define IDC_DBG_MAP44			( IDC_DBG_MAP43 + 1 )
#define IDC_DBG_MAP51			( IDC_DBG_MAP44 + 1 )
#define IDC_DBG_MAP52			( IDC_DBG_MAP51 + 1 )
#define IDC_DBG_MAP53			( IDC_DBG_MAP52 + 1 )
#define IDC_DBG_MAP54			( IDC_DBG_MAP53 + 1 )
#define IDC_DBG_MAP61			( IDC_DBG_MAP54 + 1 )
#define IDC_DBG_MAP62			( IDC_DBG_MAP61 + 1 )
#define IDC_DBG_MAP63			( IDC_DBG_MAP62 + 1 )
#define IDC_DBG_MAP64			( IDC_DBG_MAP63 + 1 )
#define IDC_DBG_MAP71			( IDC_DBG_MAP64 + 1 )
#define IDC_DBG_MAP72			( IDC_DBG_MAP71 + 1 )
#define IDC_DBG_MAP73			( IDC_DBG_MAP72 + 1 )
#define IDC_DBG_MAP74			( IDC_DBG_MAP73 + 1 )
#define IDC_DBG_MAP81			( IDC_DBG_MAP74 + 1 )
#define IDC_DBG_MAP82			( IDC_DBG_MAP81 + 1 )
#define IDC_DBG_MAP83			( IDC_DBG_MAP82 + 1 )
#define IDC_DBG_MAP84			( IDC_DBG_MAP83 + 1 )

#define IDC_DBG_TEXT_ADDRESS		( IDC_DBG_MAP84 + 1 )
#define IDC_DBG_TEXT_SOURCE		( IDC_DBG_MAP84 + 2 )
#define IDC_DBG_TEXT_WRITABLE		( IDC_DBG_MAP84 + 3 )
#define IDC_DBG_TEXT_CONTENDED		( IDC_DBG_MAP84 + 4 )

#define IDC_DBG_REG_PC			( IDC_DBG_TEXT_CONTENDED + 1 )
#define IDC_DBG_REG_SP			( IDC_DBG_REG_PC + 1 )
#define IDC_DBG_REG_AF			( IDC_DBG_REG_SP + 1 )
#define IDC_DBG_REG_AF_			( IDC_DBG_REG_AF + 1 )
#define IDC_DBG_REG_BC			( IDC_DBG_REG_AF_ + 1 )
#define IDC_DBG_REG_BC_			( IDC_DBG_REG_BC + 1 )
#define IDC_DBG_REG_DE			( IDC_DBG_REG_BC_ + 1 )
#define IDC_DBG_REG_DE_			( IDC_DBG_REG_DE + 1 )
#define IDC_DBG_REG_HL			( IDC_DBG_REG_DE_ + 1 )
#define IDC_DBG_REG_HL_			( IDC_DBG_REG_HL + 1 )
#define IDC_DBG_REG_IX			( IDC_DBG_REG_HL_ + 1 )
#define IDC_DBG_REG_IY			( IDC_DBG_REG_IX + 1 )
#define IDC_DBG_REG_I			( IDC_DBG_REG_IY + 1 )
#define IDC_DBG_REG_R			( IDC_DBG_REG_I + 1 )
#define IDC_DBG_REG_HALTED		( IDC_DBG_REG_R + 1 )
#define IDC_DBG_REG_T_STATES		( IDC_DBG_REG_HALTED + 1 )
#define IDC_DBG_REG_FLAGS		( IDC_DBG_REG_T_STATES + 1 )
#define IDC_DBG_REG_ULA			( IDC_DBG_REG_FLAGS + 1 )
#define IDC_DBG_REG_IM			( IDC_DBG_REG_ULA + 1 )

#define NUM_DBG_REGS			( IDC_DBG_REG_IM - IDC_DBG_REG_PC + 1 )

#define IDM_DBG_MENU			( IDC_DBG_REG_PC + NUM_DBG_REGS )
#define IDM_DBG_VIEW			( IDM_DBG_MENU + 1 )
#define IDM_DBG_REG			( IDM_DBG_VIEW + 1 )
#define IDM_DBG_MEMMAP			( IDM_DBG_REG + 1 )
#define IDM_DBG_BPS			( IDM_DBG_MEMMAP + 1 )
#define IDM_DBG_DIS			( IDM_DBG_BPS + 1 )
#define IDM_DBG_STACK			( IDM_DBG_DIS + 1 )
#define IDM_DBG_EVENTS			( IDM_DBG_STACK + 1 )
