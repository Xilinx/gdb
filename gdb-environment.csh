# GDB related environment variables settings
foreach l ($PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/tcl*.*)
	if ( -f $l/init.tcl ) then
		setenv TCL_LIBRARY $l
		break
	endif
end

foreach l ($PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/tk*.*)
	if ( -d $l ) then
		setenv TK_LIBRARY $l
		break
	endif
end

setenv REDHAT_GUI_LIBRARY $PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/redhat/gui;

foreach l ($PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/insight*.*)
	if ( -d $l ) then
		setenv GDBTK_LIBRARY $l
		break
	endif
end

foreach l ($PETALINUX/tools/linux-i386/microblaze-unknown-linux-gnu/share/iwidgets*.*)
	if ( -d $l ) then
		setenv INSIGHT_PLUGINS $l
		break
	endif
end
