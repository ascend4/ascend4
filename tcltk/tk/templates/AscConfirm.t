# /************************************************************\  #
#		    Module: AscConfirm.t
#                   written by Kirk Abbott
#                   Date:$Date: 1997/06/23 18:47:28 $
#                   Version: $Revision: 1.6 $
# \************************************************************/  #
#
# Module: AscConfirm.t
# Tcl version: 8.0
# Tk version: 8.0
#
# AscConfirmid $Id: AscConfirm.t,v 1.6 1997/06/23 18:47:28 ballan Exp $
#
#

global AscConfirm
set AscConfirm(button) 0

# /************************************************************\  #
# ascConfirm
# Generic confirm/cancel box. Will return the number of the button
# pressed. 1 for confirm, 2 for cancel. The box is modal.
# Arguments :
# AscConfirmGeom -- geometry -- default 
# AscConfirm(font) -*-* (duh)
# \************************************************************/  #

set AscConfirm(font) -*-*

#
# procedures to show window .ascConfirm
#
proc VShowWindow.ascConfirm {{AscConfirmGeom "190x50"} {yestext "Confirm"} {title "confirmation"}} {# xf ignore me 5

global AscConfirm
  
  toplevel .ascConfirm 

  # Window manager configurations
  global tk_version
  wm positionfrom .ascConfirm ""
  wm sizefrom .ascConfirm ""
  wm minsize .ascConfirm 170 40
  wm geometry .ascConfirm $AscConfirmGeom
  wm title .ascConfirm $title
  wm protocol .ascConfirm WM_DELETE_WINDOW {bell;bell;bell}


  # build widget .ascConfirm.main_frm
  frame .ascConfirm.main_frm 

  # build widget .ascConfirm.main_frm.cancel_btn
  button .ascConfirm.main_frm.cancel_btn \
    -font $AscConfirm(font) \
    -text {Cancel} \
    -width {7} \
    -command "
        set AscConfirm(button) 2
        catch {destroy .ascConfirm}"

  # build widget .ascConfirm.main_frm.confirm_btn
  button .ascConfirm.main_frm.confirm_btn \
    -font $AscConfirm(font) \
    -text "$yestext" \
    -width [expr [string length "$yestext"] +1] \
    -command "
        set AscConfirm(button) 1
        catch {destroy .ascConfirm}"

  # pack widget .ascConfirm.main_frm
  pack append .ascConfirm.main_frm \
    .ascConfirm.main_frm.confirm_btn {left frame center expand} \
    .ascConfirm.main_frm.cancel_btn {right frame center expand}

  # pack widget .ascConfirm
  pack append .ascConfirm \
    .ascConfirm.main_frm {top frame center expand fill}

  #
  # Get Data and Catch response.
  #
  # wait for the box to be destroyed
  update idletask
  catch { grab -global .ascConfirm } err
  tkwait window .ascConfirm
  return $AscConfirm(button)
}

proc DestroyWindow.ascConfirm {} {# xf ignore me 5

    catch "destroy .ascConfirm"
    update
}
# eof
#

