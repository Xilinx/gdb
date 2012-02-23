# GDB related environment variables settings
for l in $PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/tcl*.*; do
[ -f $l/init.tcl ] && TCL_LIBRARY=$l && break
done
for l in $PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/tk*.*; do
[ -d $l ] && TK_LIBRARY=$l && break;
done
REDHAT_GUI_LIBRARY=$PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/redhat/gui;
for l in $PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/insight*.*; do
[ -d $l ] && GDBTK_LIBRARY=$l && break;
done
for l in $PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/iwidgets*.*; do
[ -d $l ] && INSIGHT_PLUGINS=$l && break;
done
export TCL_LIBRARY TK_LIBRARY REDHAT_GUI_LIBRARY INSIGHT_PLUGINS
