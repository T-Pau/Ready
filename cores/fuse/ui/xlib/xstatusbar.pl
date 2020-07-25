#!/usr/bin/perl

die "usage: $0 <file>" unless @ARGV >= 1;

my $maxh = 0;
my $o = 0;
my $stat = 0;
my $add_pixmap = '';

my (%pxm, %map_map);
my @pxm;
my $check_mask = '';

my $do_icon = 0;

print "/*\n This file generated from ../gtk/pixmaps.c with xstatusbar.pl\n" .
      "*/\n\n";

my $pixmaps = shift;

#xpm_read('abc_pixmap.dat', 'abc');

$do_icon = 1;
xpm_read($pixmaps, 'icon');
foreach( sort keys %pxm ) {
  print $pxm{$_} . "\n";
  push @pxm, $_ if($_ !~ /_mask$/);
}

#my $n = 0;
#foreach( @pxm ) {
#  print "#define " . uc($_) . " $n\n";
#  $n++;
#}

#print "\nstatic void *pixmaps[]= {\n";
#foreach( @pxm ) {
#  print "  \&$_, " . (defined($map_map{$_ . '_mask'}) ? "\&" . $map_map{$_ . '_mask'} : "NULL") . ",\n";
#}
#print "};\n";


my $xstates = join(', ', keys %xstates);
my $idx = 1000;
my $w;
foreach(sort keys %xoverlay) {
  $idx--;
  $w = 900 if /^tape/;
  $w = 800 if /^mdr/;
  $w = 700 if /^disk/;
  $w = 600 if /^pause/;
  $w = 500 if /^mouse/;
  $overlay{sprintf('%04d', $idx - $w)} = $_;
#  print sprintf('%04d', $idx - $w) . " $_\n";
}
foreach(sort keys %overlay) {
  my $n = $overlay{$_};
  my $N = uc $n;
$overlay .= <<EOS;

  switch( ${n}_state ) {
  case UI_STATUSBAR_STATE_ACTIVE:
    x = ${N}_ACTIVE_OFF; w = ${N}_ACTIVE_W; h = ${N}_ACTIVE_H;
    break;
  case UI_STATUSBAR_STATE_INACTIVE:
    x = ${N}_INACTIVE_OFF; w = ${N}_INACTIVE_W; h = ${N}_INACTIVE_H;
    break;
  case UI_STATUSBAR_STATE_NOT_AVAILABLE:
    w = 0;
    break;
  }
  if( w ) xstatusbar_put_icon( x, w, h );
EOS
}
print "#define PIXMAPS_W $offset\n#define PIXMAPS_H $maxh\n\n";

print <<EOS;
static ui_statusbar_state $xstates;
static int status_updated;
static int icon_size = 0;

static void
xstatusbar_add_pixmap(int x, int pw, int h, libspectrum_word *colors)
{
  int y = 3 * DISPLAY_SCREEN_HEIGHT;
  x *= icon_size;
  for( ; h > 0; h-- ) {
    int w = pw;
    for( ; w > 0; w-- ) {
      int i;
      for( i = icon_size; i > 0; i-- ) {
        xdisplay_putpixel( x, y, colors);
        if( icon_size > 1 )
          xdisplay_putpixel( x, y + 1, colors);
        if( icon_size > 2 )
          xdisplay_putpixel( x, y + 2, colors);
        x++;
      }
      colors++;
    }
    x -= pw * icon_size; y += icon_size;
  }
}
/* put status icons to X(Shm)Image extra area */
void
xstatusbar_init( int size )
{
  if( icon_size == size )
    return;
  icon_size = size;

$add_pixmap}

static void
xstatusbar_put_icon( int x, int w, int h )
{
  static int dx = 0;
  static int dy = 0;
  w *= icon_size; x *= icon_size; h *= icon_size;
  if( x < 0 || dx == 0 ) {
    dx = ( DISPLAY_ASPECT_WIDTH - 2 ) * xdisplay_current_size;
    dy = ( DISPLAY_SCREEN_HEIGHT - 2 ) * xdisplay_current_size - PIXMAPS_H * icon_size;
    return;
  }
  dx -= w;
  if( shm_used ) {
#ifdef X_USE_SHM
    /* FIXME: should wait for an ShmCompletion event here */
    XShmPutImage( display, xui_mainWindow, gc, image, x, 3 * DISPLAY_SCREEN_HEIGHT, dx, dy, w, h, True );
#endif				/* #ifdef X_USE_SHM */
  } else {
    XPutImage( display, xui_mainWindow, gc, image, x, 3 * DISPLAY_SCREEN_HEIGHT, dx, dy, w, h );
  }
  dx -= 4; /* 4px space */
}

void
xstatusbar_overlay( void )
{
  int x = 0, w = 0, h;
  xstatusbar_put_icon( -1, 0, 0 );

$overlay
  status_updated = 0;
}

EOS

#static char* <variable_name>[] = {
#<Values>
#<Colors>
#<Pixels>
#<Extensions>
sub xpm_read() {
  my $st = '<variable_name>';
  my( $var_name, $width, $height, $colors, $depth, $char, $dim, $w, $h );
  my %colors;
  my $col;
  my $def;
  my $do = $_[1];
  my $mask;
  local $offset = 0;
  open FILE, '<', $_[0];
  while(<FILE>) {
    s!/\*.*\*/!!g;
    if( $st eq '<variable_name>' ) {
    #static char * <pixmap_name>[] = {
      if( /\s*(static\s+)?char\s*\*\s*([^[]+)\[/ ) {
        $var_name = $2;
        next if($var_name =~ /tape_marker/);
        next if($var_name =~ /pause/);
        next if($var_name =~ /mouse/);
        $var_name =~ s/gtkpixmap/pixmap/;
        %colors = ();
        $def = '';
        $col = 'AA';
        $mask = $do eq 'icon' ? 'add mask' : ( $var_name =~ /_mask$/ ? 'mask' : 'no mask');
        $st = '<Values>'
      }
    } elsif( $st eq '<Values>' ) {
    #<width><height><numcolors><cpp> [ <x_hotspot><y_hotspot> ] [ XPMEXT ]
      if( /\s*\x22\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)/ ) {
        ($width, $height, $colors, $depth) = ($1, $2, $3, $4);
        ($w, $h) = ($width + 1, $height);
#        $dim = "$width, $height, \n";
        $maxh = $height if( $do_icon && $mask ne 'mask' && $height > $maxh );
        $st = '<Colors>'
      }
    } elsif( $st eq '<Colors>' ) {
    #<character> { <key> <color> } { <key> <color> }
      if( /\s*\x22\s*([ -~])\s+c\s+(\x23[0-9a-fA-F]{6}|None)/ ) {
        if( substr( $2, 0, 1 ) eq '#' ) {
          $_ = '0x' . sprintf('%04x', 
               ((hex( substr( $2, 1, 2 ) ) >> 3) << 11) +
               ((hex( substr( $2, 3, 2 ) ) >> 2) << 5) +
               ((hex( substr( $2, 5, 2 ) ) >> 3))
          );
#          $colors{$1} .= sprintf('%04x', $_ );
          $def .= "#define $col $_,\n";
          $colors{$1} = $col;
          $mask{$1} = '1,';
          $col++;
        } else {
          $colors{$1} = '__';
          $mask{$1} = '0,';
          $def = "#define __ 0x0,\n" . $def;
        }
        $colors--;
        $st = '<Pixels>' if( !$colors ) ;
      }
    } elsif( $st eq '<Pixels>' ) {
    #<width><height><numcolors><cpp> [ <x_hotspot><y_hotspot> ] [ XPMEXT ]
      if( /\s*\x22(.{$w})/ ) {
        $pxm{$var_name} .= '  ' . join( ' ', map { $colors{chr($_)} } unpack("C[$w]") ) . "\n" if( $mask ne 'mask');
#        $pxm{$var_name . '_mask'} .= '  ' . join( ' ', map { $mask{chr($_)} } unpack("C[$w]") ) . "\n" if( $mask eq 'add mask');
#        $pxm{$var_name} .= '  ' . join( ' ', map { $mask{chr($_)} } unpack("C[$w]") ) . "\n" if( $mask eq 'mask');
        $h--;
        if( !$h ) {
          $st = '<variable_name>';
          if( $mask ne 'no mask') {
            my $mask_name = $mask eq 'mask' ? $var_name : $var_name . '_mask';
            $_ = $pxm{$mask_name};
            if( ! /0/ ) {
              if( $mask eq 'add mask') {
                delete( $pxm{$var_name . '_mask'});
              }
              if( $mask eq 'mask') {
                delete( $pxm{$var_name} );
                next;
              }
            } elsif ($check_mask =~ /\n([^=]+?)=$_=/) {
              $map_map{$mask_name} = $1;
              if( $mask eq 'add mask') {
                delete( $pxm{$var_name . '_mask'});
              }
              if( $mask eq 'mask') {
                delete( $pxm{$var_name} );
                next;
              }
            } else {
              $map_map{$mask_name} = $mask_name;
              $check_mask .= "\n" . $mask_name . "=$_=";
            }
          }
          my $defdim = "#define " . uc($var_name) . "_OFF $offset\n" .
                       "#define " . uc($var_name) . "_W $width\n" .
                       "#define " . uc($var_name) . "_H $height";
          if( $do eq 'icon' ) {
            $add_pixmap .= "  xstatusbar_add_pixmap( " . uc($var_name) . "_OFF, " . uc($var_name) . "_W, " . uc($var_name) . "_H, ${var_name});\n";
            my $name = $var_name;
            $name =~ s/_(in)?active$//i;
            $xstates{"${name}_state"} = 1;
            $xoverlay{$name} = 1;
          }

          $offset += $width if( $do eq 'icon' );

          $pxm{$var_name} = "$def\n$defdim\nstatic libspectrum_word ${var_name}[] = {\n$dim" . $pxm{$var_name} if( $mask ne 'mask');
          $def =~ s/\x23define (..).*/#undef $1/g;
          $pxm{$var_name} .= "};\n\n$def" if( $mask ne 'mask');

          $pxm{$var_name . '_mask'} = "static libspectrum_byte ${var_name}_mask[] = {\n" . $pxm{$var_name . '_mask'} if( $mask eq 'add mask' and defined( $pxm{$var_name . '_mask'} ) );
          $pxm{$var_name} = "static libspectrum_byte ${var_name}[] = {\n" . $pxm{$var_name} if( $mask eq 'mask');

          $pxm{$var_name . '_mask'} .= "};\n" if( $mask ne 'no mask' and defined( $pxm{$var_name . '_mask'} ));
          $maxh = $h if( $do_icon && $mask ne 'mask' && $h > $maxh );
        }
      }
    }
  }
  close FILE;
};
