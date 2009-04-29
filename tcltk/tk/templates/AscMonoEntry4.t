# ----------------------------------------------------------------
#		    Module: ascMonoEntry4.tcl
#                   written by Kirk Abbott
#                   Date:$Date: 1998/06/16 14:07:34 $
#                   Version: $Revision: 1.6 $
# ----------------------------------------------------------------
#
# Module: ascMonoEntry4.tcl
# Tcl version: 8.0
# Tk version: 8.0
# XF version: 2.2
#
# ascMonoEntry4.tcl $Id: AscMonoEntry4.t,v 1.6 1998/06/16 14:07:34 ballan Exp $
#
#

global AscMonoEntry4
set AscMonoEntry4(button) 0
set AscMonoEntry4(resultNW) ""
set AscMonoEntry4(resultNE) "" 
set AscMonoEntry4(resultSW) ""
set AscMonoEntry4(resultSE) ""
set AscMonoEntry4(font) -*-*
set AscMonoEntry4(count) "0"

#
# procedure to show window .ascMonoEntry4
#
proc VShowWindow.ascMonoEntry4 { {AscMonoEntry4Text "Find Type"}
  {AscMonoEntry4Geom 400x100} 
  {AscMonoEntry4LabelNW "Type"}         {AscMonoEntry4LabelNE "Attribute"}
  {AscMonoEntry4LabelSW "Low Value"}    {AscMonoEntry4LabelSE "High Value"}
  {AscMonoEntry4InfoNW "Info NWbox"}    {AscMonoEntry4InfoNE "Info NEbox"}
  {AscMonoEntry4InfoSW "Info SWbox"}    {AscMonoEntry4InfoSE "Info SEbox"}} {

  global AscMonoEntry4
  toplevel .ascMonoEntry4 

  # Window manager configurations
  global tk_version
  wm positionfrom .ascMonoEntry4 ""
  wm sizefrom .ascMonoEntry4 ""
  wm minsize .ascMonoEntry4 400 100
  wm geometry .ascMonoEntry4 $AscMonoEntry4Geom
  wm title .ascMonoEntry4 { }


  # build widget .ascMonoEntry4.lbl_frm
  frame .ascMonoEntry4.lbl_frm

  # build widget .ascMonoEntry4.lbl_frm.bmcanvas
  set tl .ascMonoEntry4.lbl_frm
  canvas $tl.bmcanvas -width 33 -height 27
  $tl.bmcanvas create bitmap 0 0 -background yellow \
     -anchor nw -bitmap grablock
  pack append $tl $tl.bmcanvas {left frame center}

  # build widget .ascMonoEntry4.lbl_frm.main_lbl
  label .ascMonoEntry4.lbl_frm.main_lbl \
    -font $AscMonoEntry4(font) \
    -text $AscMonoEntry4Text

  # build widget .ascMonoEntry4.main_frm
  frame .ascMonoEntry4.main_frm

  # build widget .ascMonoEntry4.main_frm.top_frm
  frame .ascMonoEntry4.main_frm.top_frm

  # build widget .ascMonoEntry4.main_frm.top_frm.entry_left
  frame .ascMonoEntry4.main_frm.top_frm.entry_left 

  # build widget .ascMonoEntry4.main_frm.top_frm.entry_left.label4
  label .ascMonoEntry4.main_frm.top_frm.entry_left.label4 \
    -text $AscMonoEntry4LabelNW: \
    -font $AscMonoEntry4(font) \
    -width {10}

  # build widget .ascMonoEntry4.main_frm.top_frm.entry_left.entry4
  entry .ascMonoEntry4.main_frm.top_frm.entry_left.entry4 \
    -exportselection 0 \
    -font $AscMonoEntry4(font) \
    -width {15} \

  # build widget .ascMonoEntry4.main_frm.bot_frm
  frame .ascMonoEntry4.main_frm.bot_frm

  # build widget .ascMonoEntry4.main_frm.bot_frm.entry_left
  frame .ascMonoEntry4.main_frm.bot_frm.entry_left

  # build widget .ascMonoEntry4.main_frm.bot_frm.entry_left.label4
  label .ascMonoEntry4.main_frm.bot_frm.entry_left.label4 \
    -text $AscMonoEntry4LabelSW: \
    -font $AscMonoEntry4(font) \
    -width {10}

  # build widget .ascMonoEntry4.main_frm.bot_frm.entry_left.entry4
  entry .ascMonoEntry4.main_frm.bot_frm.entry_left.entry4 \
    -exportselection 0 \
    -font $AscMonoEntry4(font) \
    -width {15}

  # build widget .ascMonoEntry4.main_frm.top_frm.entry_right
  frame .ascMonoEntry4.main_frm.top_frm.entry_right

  # build widget .ascMonoEntry4.main_frm.top_frm.entry_right.entry4
  entry .ascMonoEntry4.main_frm.top_frm.entry_right.entry4 \
    -exportselection 0 \
    -font $AscMonoEntry4(font) \
    -width {15}

  # build widget .ascMonoEntry4.main_frm.top_frm.entry_right.label4
  label .ascMonoEntry4.main_frm.top_frm.entry_right.label4 \
    -text $AscMonoEntry4LabelNE: \
    -font $AscMonoEntry4(font) \
    -width {10}

  # build widget .ascMonoEntry4.main_frm.bot_frm.entry_right
  frame .ascMonoEntry4.main_frm.bot_frm.entry_right

  # build widget .ascMonoEntry4.main_frm.bot_frm.entry_right.label4
  label .ascMonoEntry4.main_frm.bot_frm.entry_right.label4 \
    -text $AscMonoEntry4LabelSE:\
    -font $AscMonoEntry4(font) \
    -width {10}

  # build widget .ascMonoEntry4.main_frm.bot_frm.entry_right.entry4
  entry .ascMonoEntry4.main_frm.bot_frm.entry_right.entry4 \
    -exportselection 0 \
    -font $AscMonoEntry4(font) \
    -width {15}

  # pack widget .ascMonoEntry4.main_frm.top_frm.entry_left
  pack append .ascMonoEntry4.main_frm.top_frm.entry_left \
    .ascMonoEntry4.main_frm.top_frm.entry_left.label4 {left frame w} \
    .ascMonoEntry4.main_frm.top_frm.entry_left.entry4 \
        {left frame center expand fill}

  # pack widget .ascMonoEntry4.main_frm.top_frm.entry_right
  pack append .ascMonoEntry4.main_frm.top_frm.entry_right \
    .ascMonoEntry4.main_frm.top_frm.entry_right.label4 {left frame w} \
    .ascMonoEntry4.main_frm.top_frm.entry_right.entry4 \
        {left frame center expand fill}

  # pack widget .ascMonoEntry4.main_frm.top_frm
  pack append .ascMonoEntry4.main_frm.top_frm \
    .ascMonoEntry4.main_frm.top_frm.entry_left \
       {left frame center expand fill} \
    .ascMonoEntry4.main_frm.top_frm.entry_right \
       {left frame center expand fill} \

  # pack widget .ascMonoEntry4.main_frm.bot_frm.entry_left
  pack append .ascMonoEntry4.main_frm.bot_frm.entry_left \
    .ascMonoEntry4.main_frm.bot_frm.entry_left.label4 {left frame w} \
    .ascMonoEntry4.main_frm.bot_frm.entry_left.entry4 \
        {left frame center expand fill}

  # pack widget .ascMonoEntry4.main_frm.bot_frm.entry_right
  pack append .ascMonoEntry4.main_frm.bot_frm.entry_right \
    .ascMonoEntry4.main_frm.bot_frm.entry_right.label4 \
       {left frame w} \
    .ascMonoEntry4.main_frm.bot_frm.entry_right.entry4 \
       {top frame center expand fill}

  # pack widget .ascMonoEntry4.main_frm.bot_frm
  pack append .ascMonoEntry4.main_frm.bot_frm \
    .ascMonoEntry4.main_frm.bot_frm.entry_left \
       {left frame center expand fill} \
    .ascMonoEntry4.main_frm.bot_frm.entry_right \
        {left frame center expand fill}

  # pack widget .ascMonoEntry4.main_frm
  pack append .ascMonoEntry4.main_frm \
    .ascMonoEntry4.main_frm.top_frm {top frame center expand fill} \
    .ascMonoEntry4.main_frm.bot_frm {top frame center expand fill}

  # pack widget .ascMonoEntry4
  pack append .ascMonoEntry4 \
    .ascMonoEntry4.lbl_frm {top frame center fillx} \
    .ascMonoEntry4.main_frm {top frame center expand fill}

  # build widget .ascMonoEntry4.zbtns_frm
  frame .ascMonoEntry4.zbtns_frm

  # build widget .ascMonoEntry4.zbtns_frm.ok_btn
  button .ascMonoEntry4.zbtns_frm.ok_btn \
      -text {OK} \
      -width {10} \
      -font $AscMonoEntry4(font) \
      -command {ascMonoEntry4_StuffResults}

  # build widget .ascMonoEntry4.zbtns_frm.cancel_btn
  button .ascMonoEntry4.zbtns_frm.cancel_btn \
      -text {Cancel} \
      -font $AscMonoEntry4(font) \
      -width {10} \
      -command " \
          global AscMonoEntry4
          CancelCommand.ascMonoEntry4
          set AscMonoEntry4(button) 2
          catch {destroy .ascMonoEntry4}"

  # build widget .ascMonoEntry4.zbtns_frm.help_btn
  button .ascMonoEntry4.zbtns_frm.help_btn \
      -text {Help} \
      -font $AscMonoEntry4(font) \
      -width {10} \
      -command "global AscMonoEntry4; set AscMonoEntry4(button) 3
          catch {destroy .ascMonoEntry4}"
  
  # pack widget .ascMonoEntry4.zbtns_frm
  pack append .ascMonoEntry4.zbtns_frm \
      .ascMonoEntry4.zbtns_frm.ok_btn {left frame center expand fill} \
      .ascMonoEntry4.zbtns_frm.cancel_btn {left frame center expand fill} \
      .ascMonoEntry4.zbtns_frm.help_btn {left frame center expand fill}

  # pack widget .ascMonoEntry4.lbl_frm
  pack append .ascMonoEntry4.lbl_frm \
    .ascMonoEntry4.lbl_frm.main_lbl {top frame center}

  # pack widget .ascMonoEntry4
  pack append .ascMonoEntry4 \
    .ascMonoEntry4.zbtns_frm {top frame center fillx}

  #
  #Initial Entry
  .ascMonoEntry4.main_frm.top_frm.entry_left.entry4 \
      insert end "$AscMonoEntry4InfoNW"
  .ascMonoEntry4.main_frm.bot_frm.entry_left.entry4 \
      insert end "$AscMonoEntry4InfoSW"
  .ascMonoEntry4.main_frm.top_frm.entry_right.entry4 \
      insert end "$AscMonoEntry4InfoNE"
  .ascMonoEntry4.main_frm.bot_frm.entry_right.entry4 \
      insert end "$AscMonoEntry4InfoSE"

  bind  .ascMonoEntry4.main_frm.top_frm.entry_left.entry4 <Return>  {
    ascMonoEntry4_StuffResults
  }
  bind  .ascMonoEntry4.main_frm.bot_frm.entry_left.entry4 <Return>  {
    ascMonoEntry4_StuffResults
  }
  bind  .ascMonoEntry4.main_frm.top_frm.entry_right.entry4 <Return>  {
     ascMonoEntry4_StuffResults
  }
  bind  .ascMonoEntry4.main_frm.bot_frm.entry_right.entry4 <Return> {
    ascMonoEntry4_StuffResults
  }

  #
  # Get Data and Catch response.
  #
  # wait for the box to be destroyed
  focus .ascMonoEntry4.main_frm.top_frm.entry_left.entry4
  update idletask
  bind .ascMonoEntry4 <Visibility> {ascKeepOnTop %W}
  grab .ascMonoEntry4
  tkwait window .ascMonoEntry4
  set data  [list $AscMonoEntry4(button) \
	$AscMonoEntry4(resultNW) $AscMonoEntry4(resultNE) \
	$AscMonoEntry4(resultSW) $AscMonoEntry4(resultSE)]
  return $data
}

proc DestroyWindow.ascMonoEntry4 {} {# xf ignore me 7

    catch "destroy .ascMonoEntry4"
    update
}
proc OKCommand.ascMonoEntry4 {} {
  global AscMonoEntry4
}
proc CancelCommand.ascMonoEntry4 {} {
  global AscMonoEntry4
}

proc ascMonoEntry4_StuffResults {} {
     global AscMonoEntry4
     set AscMonoEntry4(button) 1
     set AscMonoEntry4(resultNW) \
       [.ascMonoEntry4.main_frm.top_frm.entry_left.entry4 get]
     set AscMonoEntry4(resultNE) \
       [.ascMonoEntry4.main_frm.top_frm.entry_right.entry4 get]
     set AscMonoEntry4(resultSW) \
       [.ascMonoEntry4.main_frm.bot_frm.entry_left.entry4 get]
     set AscMonoEntry4(resultSE) \
       [.ascMonoEntry4.main_frm.bot_frm.entry_right.entry4 get]
     OKCommand.ascMonoEntry4
     catch {destroy .ascMonoEntry4}
}
# eof
#

