/* hyperlinks.h: hyperlink control
   Copyright 2002 Neal Stublen

   Taken (almost) verbatim from public domain code found on CodeGuru:
   http://www.codeguru.com/cpp/controls/staticctrl/article.php/c5803
   Copyright 2002 Neal Stublen
   All rights reserved.

   The CodeGuru license is a bit vague, but they state:
   "While we are talking about copyrights, you retain copyright of your 
   article and code, but by submitting it to CodeGuru you give permission
   to use it in a fair manner and also permit all developers to freely use 
   the code in their own applications -even if they are commercial."
   License: CodeGuru-specific, 2009

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#ifndef FUSE_HYPERLINKS_H
#define FUSE_HYPERLINKS_H

BOOL ConvertStaticToHyperlink( HWND hwndCtl );
BOOL ConvertCtlStaticToHyperlink( HWND hwndParent, UINT uiCtlId );

#endif                          /* #ifndef FUSE_HYPERLINKS_H */
