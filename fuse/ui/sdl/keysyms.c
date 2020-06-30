/* keysyms.c: UI keysym to Fuse input layer keysym mappings
   Copyright (c) 2000-2007 Philip Kendall, Matan Ziv-Av, Russell Marks
                           Fredrick Meunier, Catalin Mihaila, Stuart Brady

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

/* This file is autogenerated from keysyms.dat by keysyms.pl.
   Do not edit unless you know what you're doing! */

#include <config.h>


#include "input.h"
#include "keyboard.h"

#include <SDL.h>

keysyms_map_t keysyms_map[] = {

  { SDLK_TAB,          INPUT_KEY_Tab          },
  { SDLK_RETURN,       INPUT_KEY_Return       },
  { SDLK_ESCAPE,       INPUT_KEY_Escape       },
  { SDLK_SPACE,        INPUT_KEY_space        },
  { SDLK_EXCLAIM,      INPUT_KEY_exclam       },
  { SDLK_QUOTEDBL,     INPUT_KEY_quotedbl     },
  { SDLK_HASH,         INPUT_KEY_numbersign   },
  { SDLK_DOLLAR,       INPUT_KEY_dollar       },
  { SDLK_AMPERSAND,    INPUT_KEY_ampersand    },
  { SDLK_QUOTE,        INPUT_KEY_apostrophe   },
  { SDLK_LEFTPAREN,    INPUT_KEY_parenleft    },
  { SDLK_RIGHTPAREN,   INPUT_KEY_parenright   },
  { SDLK_ASTERISK,     INPUT_KEY_asterisk     },
  { SDLK_PLUS,         INPUT_KEY_plus         },
  { SDLK_COMMA,        INPUT_KEY_comma        },
  { SDLK_MINUS,        INPUT_KEY_minus        },
  { SDLK_PERIOD,       INPUT_KEY_period       },
  { SDLK_SLASH,        INPUT_KEY_slash        },
  { SDLK_0,            INPUT_KEY_0            },
  { SDLK_1,            INPUT_KEY_1            },
  { SDLK_2,            INPUT_KEY_2            },
  { SDLK_3,            INPUT_KEY_3            },
  { SDLK_4,            INPUT_KEY_4            },
  { SDLK_5,            INPUT_KEY_5            },
  { SDLK_6,            INPUT_KEY_6            },
  { SDLK_7,            INPUT_KEY_7            },
  { SDLK_8,            INPUT_KEY_8            },
  { SDLK_9,            INPUT_KEY_9            },
  { SDLK_COLON,        INPUT_KEY_colon        },
  { SDLK_SEMICOLON,    INPUT_KEY_semicolon    },
  { SDLK_LESS,         INPUT_KEY_less         },
  { SDLK_EQUALS,       INPUT_KEY_equal        },
  { SDLK_GREATER,      INPUT_KEY_greater      },
  { SDLK_QUESTION,     INPUT_KEY_question     },
  { SDLK_AT,           INPUT_KEY_at           },
  { SDLK_LEFTBRACKET,  INPUT_KEY_bracketleft  },
  { SDLK_BACKSLASH,    INPUT_KEY_backslash    },
  { SDLK_RIGHTBRACKET, INPUT_KEY_bracketright },
  { SDLK_CARET,        INPUT_KEY_asciicircum  },
  { SDLK_UNDERSCORE,   INPUT_KEY_underscore   },
  { SDLK_a,            INPUT_KEY_a            },
  { SDLK_b,            INPUT_KEY_b            },
  { SDLK_c,            INPUT_KEY_c            },
  { SDLK_d,            INPUT_KEY_d            },
  { SDLK_e,            INPUT_KEY_e            },
  { SDLK_f,            INPUT_KEY_f            },
  { SDLK_g,            INPUT_KEY_g            },
  { SDLK_h,            INPUT_KEY_h            },
  { SDLK_i,            INPUT_KEY_i            },
  { SDLK_j,            INPUT_KEY_j            },
  { SDLK_k,            INPUT_KEY_k            },
  { SDLK_l,            INPUT_KEY_l            },
  { SDLK_m,            INPUT_KEY_m            },
  { SDLK_n,            INPUT_KEY_n            },
  { SDLK_o,            INPUT_KEY_o            },
  { SDLK_p,            INPUT_KEY_p            },
  { SDLK_q,            INPUT_KEY_q            },
  { SDLK_r,            INPUT_KEY_r            },
  { SDLK_s,            INPUT_KEY_s            },
  { SDLK_t,            INPUT_KEY_t            },
  { SDLK_u,            INPUT_KEY_u            },
  { SDLK_v,            INPUT_KEY_v            },
  { SDLK_w,            INPUT_KEY_w            },
  { SDLK_x,            INPUT_KEY_x            },
  { SDLK_y,            INPUT_KEY_y            },
  { SDLK_z,            INPUT_KEY_z            },
  { SDLK_BACKSPACE,    INPUT_KEY_BackSpace    },
  { SDLK_KP_ENTER,     INPUT_KEY_KP_Enter     },
  { SDLK_UP,           INPUT_KEY_Up           },
  { SDLK_DOWN,         INPUT_KEY_Down         },
  { SDLK_LEFT,         INPUT_KEY_Left         },
  { SDLK_RIGHT,        INPUT_KEY_Right        },
  { SDLK_INSERT,       INPUT_KEY_Insert       },
  { SDLK_DELETE,       INPUT_KEY_Delete       },
  { SDLK_HOME,         INPUT_KEY_Home         },
  { SDLK_END,          INPUT_KEY_End          },
  { SDLK_PAGEUP,       INPUT_KEY_Page_Up      },
  { SDLK_PAGEDOWN,     INPUT_KEY_Page_Down    },
  { SDLK_F1,           INPUT_KEY_F1           },
  { SDLK_F2,           INPUT_KEY_F2           },
  { SDLK_F3,           INPUT_KEY_F3           },
  { SDLK_F4,           INPUT_KEY_F4           },
  { SDLK_F5,           INPUT_KEY_F5           },
  { SDLK_F6,           INPUT_KEY_F6           },
  { SDLK_F7,           INPUT_KEY_F7           },
  { SDLK_F8,           INPUT_KEY_F8           },
  { SDLK_F9,           INPUT_KEY_F9           },
  { SDLK_F10,          INPUT_KEY_F10          },
  { SDLK_F11,          INPUT_KEY_F11          },
  { SDLK_F12,          INPUT_KEY_F12          },
  { SDLK_LSHIFT,       INPUT_KEY_Shift_L      },
  { SDLK_RSHIFT,       INPUT_KEY_Shift_R      },
  { SDLK_LCTRL,        INPUT_KEY_Control_L    },
  { SDLK_RCTRL,        INPUT_KEY_Control_R    },
  { SDLK_LALT,         INPUT_KEY_Alt_L        },
  { SDLK_RALT,         INPUT_KEY_Alt_R        },
  { SDLK_LMETA,        INPUT_KEY_Meta_L       },
  { SDLK_RMETA,        INPUT_KEY_Meta_R       },
  { SDLK_LSUPER,       INPUT_KEY_Super_L      },
  { SDLK_RSUPER,       INPUT_KEY_Super_R      },
  { SDLK_MENU,         INPUT_KEY_Mode_switch  },

  { 0, 0 }			/* End marker: DO NOT MOVE! */

};


keysyms_map_t unicode_keysyms_map[] = {

  { ' ',               INPUT_KEY_space        },
  { '!',               INPUT_KEY_exclam       },
  { '\"',              INPUT_KEY_quotedbl     },
  { '#',               INPUT_KEY_numbersign   },
  { '$',               INPUT_KEY_dollar       },
  { '%',               INPUT_KEY_percent      },
  { '&',               INPUT_KEY_ampersand    },
  { '\'',              INPUT_KEY_apostrophe   },
  { '(',               INPUT_KEY_parenleft    },
  { ')',               INPUT_KEY_parenright   },
  { '*',               INPUT_KEY_asterisk     },
  { '+',               INPUT_KEY_plus         },
  { ',',               INPUT_KEY_comma        },
  { '-',               INPUT_KEY_minus        },
  { '.',               INPUT_KEY_period       },
  { '/',               INPUT_KEY_slash        },
  { '0',               INPUT_KEY_0            },
  { '1',               INPUT_KEY_1            },
  { '2',               INPUT_KEY_2            },
  { '3',               INPUT_KEY_3            },
  { '4',               INPUT_KEY_4            },
  { '5',               INPUT_KEY_5            },
  { '6',               INPUT_KEY_6            },
  { '7',               INPUT_KEY_7            },
  { '8',               INPUT_KEY_8            },
  { '9',               INPUT_KEY_9            },
  { ':',               INPUT_KEY_colon        },
  { ';',               INPUT_KEY_semicolon    },
  { '<',               INPUT_KEY_less         },
  { '=',               INPUT_KEY_equal        },
  { '>',               INPUT_KEY_greater      },
  { '?',               INPUT_KEY_question     },
  { '@',               INPUT_KEY_at           },
  { 'A',               INPUT_KEY_A            },
  { 'B',               INPUT_KEY_B            },
  { 'C',               INPUT_KEY_C            },
  { 'D',               INPUT_KEY_D            },
  { 'E',               INPUT_KEY_E            },
  { 'F',               INPUT_KEY_F            },
  { 'G',               INPUT_KEY_G            },
  { 'H',               INPUT_KEY_H            },
  { 'I',               INPUT_KEY_I            },
  { 'J',               INPUT_KEY_J            },
  { 'K',               INPUT_KEY_K            },
  { 'L',               INPUT_KEY_L            },
  { 'M',               INPUT_KEY_M            },
  { 'N',               INPUT_KEY_N            },
  { 'O',               INPUT_KEY_O            },
  { 'P',               INPUT_KEY_P            },
  { 'Q',               INPUT_KEY_Q            },
  { 'R',               INPUT_KEY_R            },
  { 'S',               INPUT_KEY_S            },
  { 'T',               INPUT_KEY_T            },
  { 'U',               INPUT_KEY_U            },
  { 'V',               INPUT_KEY_V            },
  { 'W',               INPUT_KEY_W            },
  { 'X',               INPUT_KEY_X            },
  { 'Y',               INPUT_KEY_Y            },
  { 'Z',               INPUT_KEY_Z            },
  { '[',               INPUT_KEY_bracketleft  },
  { '\\',              INPUT_KEY_backslash    },
  { ']',               INPUT_KEY_bracketright },
  { '^',               INPUT_KEY_asciicircum  },
  { '_',               INPUT_KEY_underscore   },
  { 'a',               INPUT_KEY_a            },
  { 'b',               INPUT_KEY_b            },
  { 'c',               INPUT_KEY_c            },
  { 'd',               INPUT_KEY_d            },
  { 'e',               INPUT_KEY_e            },
  { 'f',               INPUT_KEY_f            },
  { 'g',               INPUT_KEY_g            },
  { 'h',               INPUT_KEY_h            },
  { 'i',               INPUT_KEY_i            },
  { 'j',               INPUT_KEY_j            },
  { 'k',               INPUT_KEY_k            },
  { 'l',               INPUT_KEY_l            },
  { 'm',               INPUT_KEY_m            },
  { 'n',               INPUT_KEY_n            },
  { 'o',               INPUT_KEY_o            },
  { 'p',               INPUT_KEY_p            },
  { 'q',               INPUT_KEY_q            },
  { 'r',               INPUT_KEY_r            },
  { 's',               INPUT_KEY_s            },
  { 't',               INPUT_KEY_t            },
  { 'u',               INPUT_KEY_u            },
  { 'v',               INPUT_KEY_v            },
  { 'w',               INPUT_KEY_w            },
  { 'x',               INPUT_KEY_x            },
  { 'y',               INPUT_KEY_y            },
  { 'z',               INPUT_KEY_z            },
  { '{',               INPUT_KEY_braceleft    },
  { '|',               INPUT_KEY_bar          },
  { '}',               INPUT_KEY_braceright   },
  { '~',               INPUT_KEY_asciitilde   },

  { 0, 0 }			/* End marker: DO NOT MOVE! */

};
