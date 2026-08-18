 SUBR attract_mode
#loop
    JSRP show_hstd
;   JSRP commented_out
    JSRP DCS_LOGO
    JSRP show_sports_logo
    JSRP show_gameplay
    JSRP creditscreen
    JSRP show_title
    JSRP show_gameplay
    JSRP creditscreen
    JSRP DO_HINTS
    JSRP show_gen_tips
    JSRP show_bios
    movk 1,a14
    JSRP show_bios_tips
    JSRP show_operatormsg
    calla RemapIO
    JSRP creditscreen
