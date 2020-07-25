/* scaler_hq2x.c: included into scalers.c
 * Copyright (C) 2008 Gergely Szasz
 *
 * HQ2x and HQ3x scalers taken from HiEnd3D Demos (http://www.hiend3d.com)
 * Copyright (C) 2003 MaxSt ( maxst@hiend3d.com )
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
      switch( pattern ) {
      case 0:
      case 1:
      case 4:
      case 32:
      case 128:
      case 5:
      case 132:
      case 160:
      case 33:
      case 129:
      case 36:
      case 133:
      case 164:
      case 161:
      case 37:
      case 165:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 2:
      case 34:
      case 130:
      case 162:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 16:
      case 17:
      case 48:
      case 49:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 64:
      case 65:
      case 68:
      case 69:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 8:
      case 12:
      case 136:
      case 140:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 3:
      case 35:
      case 131:
      case 163:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 6:
      case 38:
      case 134:
      case 166:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 20:
      case 21:
      case 52:
      case 53:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 144:
      case 145:
      case 176:
      case 177:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 192:
      case 193:
      case 196:
      case 197:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 96:
      case 97:
      case 100:
      case 101:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 40:
      case 44:
      case 168:
      case 172:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 9:
      case 13:
      case 137:
      case 141:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 18:
      case 50:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 80:
      case 81:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_21;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 72:
      case 76:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_20;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 10:
      case 138:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 66:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 24:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 7:
      case 39:
      case 135:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 148:
      case 149:
      case 180:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 224:
      case 228:
      case 225:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 41:
      case 169:
      case 45:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 22:
      case 54:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 208:
      case 209:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_21;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 104:
      case 108:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_20;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 11:
      case 139:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 19:
      case 51:
	{
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q = HQ_PIXEL00_11;
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q = HQ_PIXEL00_60;
	    *q1 = HQ_PIXEL01_90;
	  }
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 146:
      case 178:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	    *qN1 = HQ_PIXEL11_12;
	  } else {
	    *q1 = HQ_PIXEL01_90;
	    *qN1 = HQ_PIXEL11_61;
	  }
	  *qN = HQ_PIXEL10_20;
	  break;
	}
      case 84:
      case 85:
	{
	  *q = HQ_PIXEL00_20;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *q1 = HQ_PIXEL01_11;
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *q1 = HQ_PIXEL01_60;
	    *qN1 = HQ_PIXEL11_90;
	  }
	  *qN = HQ_PIXEL10_21;
	  break;
	}
      case 112:
      case 113:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_22;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN = HQ_PIXEL10_12;
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN = HQ_PIXEL10_61;
	    *qN1 = HQ_PIXEL11_90;
	  }
	  break;
	}
      case 200:
      case 204:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_20;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	    *qN1 = HQ_PIXEL11_11;
	  } else {
	    *qN = HQ_PIXEL10_90;
	    *qN1 = HQ_PIXEL11_60;
	  }
	  break;
	}
      case 73:
      case 77:
	{
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *q = HQ_PIXEL00_12;
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *q = HQ_PIXEL00_61;
	    *qN = HQ_PIXEL10_90;
	  }
	  *q1 = HQ_PIXEL01_20;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 42:
      case 170:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	    *qN = HQ_PIXEL10_11;
	  } else {
	    *q = HQ_PIXEL00_90;
	    *qN = HQ_PIXEL10_60;
	  }
	  *q1 = HQ_PIXEL01_21;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 14:
      case 142:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	    *q1 = HQ_PIXEL01_12;
	  } else {
	    *q = HQ_PIXEL00_90;
	    *q1 = HQ_PIXEL01_61;
	  }
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 67:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 70:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 28:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 152:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 194:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 98:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 56:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 25:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 26:
      case 31:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 82:
      case 214:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_21;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 88:
      case 248:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_22;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 74:
      case 107:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_21;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 27:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_10;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 86:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 216:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_10;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 106:
	{
	  *q = HQ_PIXEL00_10;
	  *q1 = HQ_PIXEL01_21;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 30:
	{
	  *q = HQ_PIXEL00_10;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 210:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_10;
	  *qN = HQ_PIXEL10_21;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 120:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_22;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 75:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_10;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 29:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 198:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 184:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 99:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 57:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 71:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 156:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 226:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 60:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 195:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 102:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 153:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 58:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 83:
	{
	  *q = HQ_PIXEL00_11;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_21;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 92:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_11;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 202:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  *q1 = HQ_PIXEL01_21;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 78:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  *q1 = HQ_PIXEL01_12;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 154:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 114:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_12;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 89:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_22;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 90:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 55:
      case 23:
	{
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q = HQ_PIXEL00_11;
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q = HQ_PIXEL00_60;
	    *q1 = HQ_PIXEL01_90;
	  }
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 182:
      case 150:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	    *qN1 = HQ_PIXEL11_12;
	  } else {
	    *q1 = HQ_PIXEL01_90;
	    *qN1 = HQ_PIXEL11_61;
	  }
	  *qN = HQ_PIXEL10_20;
	  break;
	}
      case 213:
      case 212:
	{
	  *q = HQ_PIXEL00_20;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *q1 = HQ_PIXEL01_11;
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *q1 = HQ_PIXEL01_60;
	    *qN1 = HQ_PIXEL11_90;
	  }
	  *qN = HQ_PIXEL10_21;
	  break;
	}
      case 241:
      case 240:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_22;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN = HQ_PIXEL10_12;
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN = HQ_PIXEL10_61;
	    *qN1 = HQ_PIXEL11_90;
	  }
	  break;
	}
      case 236:
      case 232:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_20;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	    *qN1 = HQ_PIXEL11_11;
	  } else {
	    *qN = HQ_PIXEL10_90;
	    *qN1 = HQ_PIXEL11_60;
	  }
	  break;
	}
      case 109:
      case 105:
	{
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *q = HQ_PIXEL00_12;
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *q = HQ_PIXEL00_61;
	    *qN = HQ_PIXEL10_90;
	  }
	  *q1 = HQ_PIXEL01_20;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 171:
      case 43:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	    *qN = HQ_PIXEL10_11;
	  } else {
	    *q = HQ_PIXEL00_90;
	    *qN = HQ_PIXEL10_60;
	  }
	  *q1 = HQ_PIXEL01_21;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 143:
      case 15:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	    *q1 = HQ_PIXEL01_12;
	  } else {
	    *q = HQ_PIXEL00_90;
	    *q1 = HQ_PIXEL01_61;
	  }
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 124:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_11;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 203:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_10;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 62:
	{
	  *q = HQ_PIXEL00_10;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 211:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_10;
	  *qN = HQ_PIXEL10_21;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 118:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 217:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_10;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 110:
	{
	  *q = HQ_PIXEL00_10;
	  *q1 = HQ_PIXEL01_12;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 155:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_10;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 188:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 185:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_22;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 61:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 157:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 103:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 227:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_21;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 230:
	{
	  *q = HQ_PIXEL00_22;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 199:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_21;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 220:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_11;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 158:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 234:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  *q1 = HQ_PIXEL01_21;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 242:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_12;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 59:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 121:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_22;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 87:
	{
	  *q = HQ_PIXEL00_11;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_21;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 79:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_12;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 122:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 94:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 218:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 91:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 229:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 167:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 173:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_20;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 181:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 186:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 115:
	{
	  *q = HQ_PIXEL00_11;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_12;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 93:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_11;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 206:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  *q1 = HQ_PIXEL01_12;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 205:
      case 201:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_20;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_10;
	  } else {
	    *qN = HQ_PIXEL10_70;
	  }
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 174:
      case 46:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_10;
	  } else {
	    *q = HQ_PIXEL00_70;
	  }
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 179:
      case 147:
	{
	  *q = HQ_PIXEL00_11;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_10;
	  } else {
	    *q1 = HQ_PIXEL01_70;
	  }
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 117:
      case 116:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_12;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_10;
	  } else {
	    *qN1 = HQ_PIXEL11_70;
	  }
	  break;
	}
      case 189:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 231:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 126:
	{
	  *q = HQ_PIXEL00_10;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 219:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_10;
	  *qN = HQ_PIXEL10_10;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 125:
	{
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *q = HQ_PIXEL00_12;
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *q = HQ_PIXEL00_61;
	    *qN = HQ_PIXEL10_90;
	  }
	  *q1 = HQ_PIXEL01_11;
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 221:
	{
	  *q = HQ_PIXEL00_12;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *q1 = HQ_PIXEL01_11;
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *q1 = HQ_PIXEL01_60;
	    *qN1 = HQ_PIXEL11_90;
	  }
	  *qN = HQ_PIXEL10_10;
	  break;
	}
      case 207:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	    *q1 = HQ_PIXEL01_12;
	  } else {
	    *q = HQ_PIXEL00_90;
	    *q1 = HQ_PIXEL01_61;
	  }
	  *qN = HQ_PIXEL10_10;
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 238:
	{
	  *q = HQ_PIXEL00_10;
	  *q1 = HQ_PIXEL01_12;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	    *qN1 = HQ_PIXEL11_11;
	  } else {
	    *qN = HQ_PIXEL10_90;
	    *qN1 = HQ_PIXEL11_60;
	  }
	  break;
	}
      case 190:
	{
	  *q = HQ_PIXEL00_10;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	    *qN1 = HQ_PIXEL11_12;
	  } else {
	    *q1 = HQ_PIXEL01_90;
	    *qN1 = HQ_PIXEL11_61;
	  }
	  *qN = HQ_PIXEL10_11;
	  break;
	}
      case 187:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	    *qN = HQ_PIXEL10_11;
	  } else {
	    *q = HQ_PIXEL00_90;
	    *qN = HQ_PIXEL10_60;
	  }
	  *q1 = HQ_PIXEL01_10;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 243:
	{
	  *q = HQ_PIXEL00_11;
	  *q1 = HQ_PIXEL01_10;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN = HQ_PIXEL10_12;
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN = HQ_PIXEL10_61;
	    *qN1 = HQ_PIXEL11_90;
	  }
	  break;
	}
      case 119:
	{
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q = HQ_PIXEL00_11;
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q = HQ_PIXEL00_60;
	    *q1 = HQ_PIXEL01_90;
	  }
	  *qN = HQ_PIXEL10_12;
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 237:
      case 233:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_20;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_100;
	  }
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 175:
      case 47:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_100;
	  }
	  *q1 = HQ_PIXEL01_12;
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_20;
	  break;
	}
      case 183:
      case 151:
	{
	  *q = HQ_PIXEL00_11;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_100;
	  }
	  *qN = HQ_PIXEL10_20;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 245:
      case 244:
	{
	  *q = HQ_PIXEL00_20;
	  *q1 = HQ_PIXEL01_11;
	  *qN = HQ_PIXEL10_12;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_100;
	  }
	  break;
	}
      case 250:
	{
	  *q = HQ_PIXEL00_10;
	  *q1 = HQ_PIXEL01_10;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 123:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_10;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 95:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_10;
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 222:
	{
	  *q = HQ_PIXEL00_10;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_10;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 252:
	{
	  *q = HQ_PIXEL00_21;
	  *q1 = HQ_PIXEL01_11;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_100;
	  }
	  break;
	}
      case 249:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_22;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_100;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 235:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_21;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_100;
	  }
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 111:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_100;
	  }
	  *q1 = HQ_PIXEL01_12;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_22;
	  break;
	}
      case 63:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_100;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_21;
	  break;
	}
      case 159:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_100;
	  }
	  *qN = HQ_PIXEL10_22;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 215:
	{
	  *q = HQ_PIXEL00_11;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_100;
	  }
	  *qN = HQ_PIXEL10_21;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 246:
	{
	  *q = HQ_PIXEL00_22;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  *qN = HQ_PIXEL10_12;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_100;
	  }
	  break;
	}
      case 254:
	{
	  *q = HQ_PIXEL00_10;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_100;
	  }
	  break;
	}
      case 253:
	{
	  *q = HQ_PIXEL00_12;
	  *q1 = HQ_PIXEL01_11;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_100;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_100;
	  }
	  break;
	}
      case 251:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  *q1 = HQ_PIXEL01_10;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_100;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 239:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_100;
	  }
	  *q1 = HQ_PIXEL01_12;
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_100;
	  }
	  *qN1 = HQ_PIXEL11_11;
	  break;
	}
      case 127:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_100;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_20;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_20;
	  }
	  *qN1 = HQ_PIXEL11_10;
	  break;
	}
      case 191:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_100;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_100;
	  }
	  *qN = HQ_PIXEL10_11;
	  *qN1 = HQ_PIXEL11_12;
	  break;
	}
      case 223:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_20;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_100;
	  }
	  *qN = HQ_PIXEL10_10;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_20;
	  }
	  break;
	}
      case 247:
	{
	  *q = HQ_PIXEL00_11;
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_100;
	  }
	  *qN = HQ_PIXEL10_12;
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_100;
	  }
	  break;
	}
      case 255:
	{
	  if( HQ_YUVDIFF( y[4], u[4], v[4], y[2], u[2], v[2] ) ) {
	    *q = HQ_PIXEL00_0;
	  } else {
	    *q = HQ_PIXEL00_100;
	  }
	  if( HQ_YUVDIFF( y[2], u[2], v[2], y[6], u[6], v[6] ) ) {
	    *q1 = HQ_PIXEL01_0;
	  } else {
	    *q1 = HQ_PIXEL01_100;
	  }
	  if( HQ_YUVDIFF( y[8], u[8], v[8], y[4], u[4], v[4] ) ) {
	    *qN = HQ_PIXEL10_0;
	  } else {
	    *qN = HQ_PIXEL10_100;
	  }
	  if( HQ_YUVDIFF( y[6], u[6], v[6], y[8], u[8], v[8] ) ) {
	    *qN1 = HQ_PIXEL11_0;
	  } else {
	    *qN1 = HQ_PIXEL11_100;
	  }
	  break;
	}
      }
