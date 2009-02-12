" Vim syntax file
" Language:   conkyrc
" Author:     Ciaran McCreesh <ciaranm@gentoo.org>
" Version:    20060307
" Copyright:  Copyright (c) 2005 Ciaran McCreesh
" Licence:    You may redistribute this under the same terms as Vim itself
"

if exists("b:current_syntax")
	finish
endif

syn region ConkyrcComment start=/^\s*#/ end=/$/

syn keyword ConkyrcSetting
			\ alignment
			\ background
			\ show_graph_scale
			\ show_graph_range
			\ border_margin
			\ border_width
			\ color0
			\ color1
			\ color2
			\ color3
			\ color4
			\ color5
			\ color6
			\ color7
			\ color8
			\ color9
			\ default_color
			\ default_shade_color
			\ default_shadecolor
			\ default_outline_color
			\ default_outlinecolor
			\ imap
			\ pop3
			\ mpd_host
			\ mpd_port
			\ mpd_password
			\ music_player_interval
			\ sensor_device
			\ cpu_avg_samples
			\ net_avg_samples
			\ double_buffer
			\ override_utf8_locale
			\ draw_borders
			\ draw_graph_borders
			\ draw_shades
			\ draw_outline
			\ out_to_console
			\ out_to_stderr
			\ overwrite_file
			\ append_file
			\ use_spacer
			\ use_xft
			\ font
			\ xftalpha
			\ xftfont
			\ use_xft
			\ gap_x
			\ gap_y
			\ mail_spool
			\ minimum_size
			\ maximum_width
			\ no_buffers
			\ top_cpu_separate
			\ short_units
			\ pad_percents
			\ own_window
			\ own_window_class
			\ own_window_title
			\ own_window_transparent
			\ own_window_colour
			\ own_window_hints
			\ own_window_type
			\ stippled_borders
			\ temp1
			\ temp2
			\ update_interval
			\ template0
			\ template1
			\ template2
			\ template3
			\ template4
			\ template5
			\ template6
			\ template7
			\ template8
			\ template9
			\ total_run_times
			\ uppercase
			\ max_specials
			\ max_user_text
			\ text_buffer_size
			\ text
			\ max_port_monitor_connections

syn keyword ConkyrcConstant
			\ above
			\ below
			\ bottom_left
			\ bottom_right
			\ bottom_middle
			\ desktop
			\ dock
			\ no
			\ none
			\ normal
			\ override
			\ skip_pager
			\ skip_taskbar
			\ sticky
			\ top_left
			\ top_right
			\ top_middle
			\ middle_left
			\ middle_right
			\ undecorated
			\ yes

syn match ConkyrcNumber /\S\@<!\d\+\(\.\d\+\)\?\(\S\@!\|}\@=\)/
			\ nextgroup=ConkyrcNumber,ConkyrcColour skipwhite
syn match ConkyrcColour /\S\@<!#[a-fA-F0-9]\{6\}\(\S\@!\|}\@=\)/
			\ nextgroup=ConkyrcNumber,ConkyrcColour skipwhite

syn region ConkyrcText start=/^TEXT$/ end=/\%$/ contains=ConkyrcVar

syn region ConkyrcVar start=/\${/ end=/}/ contained contains=ConkyrcVarStuff
syn region ConkyrcVar start=/\$\w\@=/ end=/\W\@=\|$/ contained contains=ConkyrcVarName

syn match ConkyrcVarStuff /{\@<=/ms=s contained nextgroup=ConkyrcVarName

syn keyword ConkyrcVarName contained nextgroup=ConkyrcNumber,ConkyrcColour skipwhite
			\ acpitemp
			\ acpitempf
			\ freq
			\ freq_g
			\ voltage_mv
			\ voltage_v
			\ wireless_essid
			\ wireless_mode
			\ wireless_bitrate
			\ wireless_ap
			\ wireless_link_qual
			\ wireless_link_qual_max
			\ wireless_link_qual_perc
			\ wireless_link_bar
			\ freq_dyn
			\ freq_dyn_g
			\ adt746xcpu
			\ adt746xfan
			\ acpifan
			\ acpiacadapter
			\ battery
			\ battery_time
			\ battery_percent
			\ battery_bar
			\ buffers
			\ cached
			\ cpu
			\ cpubar
			\ cpugraph
			\ loadgraph
			\ color
			\ color0
			\ color1
			\ color2
			\ color3
			\ color4
			\ color5
			\ color6
			\ color7
			\ color8
			\ color9
			\ conky_version
			\ conky_build_date
			\ conky_build_arch
			\ disk_protect
			\ i8k_version
			\ i8k_bios
			\ i8k_serial
			\ i8k_cpu_temp
			\ i8k_cpu_tempf
			\ i8k_left_fan_status
			\ i8k_right_fan_status
			\ i8k_left_fan_rpm
			\ i8k_right_fan_rpm
			\ i8k_ac_status
			\ i8k_buttons_status
			\ ibm_fan
			\ ibm_temps
			\ ibm_volume
			\ ibm_brightness
			\ if_up
			\ if_gw
			\ gw_iface
			\ gw_ip
			\ laptop_mode
			\ pb_battery
			\ obsd_sensors_temp
			\ obsd_sensors_fan
			\ obsd_sensors_volt
			\ obsd_vendor
			\ obsd_product
			\ font
			\ diskio
			\ diskio_write
			\ diskio_read
			\ diskiograph
			\ diskiograph_read
			\ diskiograph_write
			\ downspeed
			\ downspeedf
			\ downspeedgraph
			\ else
			\ endif
			\ addr
			\ addrs
			\ image
			\ exec
			\ execp
			\ execbar
			\ execgraph
			\ execibar
			\ execigraph
			\ execi
			\ execpi
			\ texeci
			\ imap_unseen
			\ imap_messages
			\ pop3_unseen
			\ pop3_used
			\ fs_bar
			\ fs_free
			\ fs_free_perc
			\ fs_size
			\ fs_type
			\ fs_used
			\ fs_bar_free
			\ fs_used_perc
			\ loadavg
			\ goto
			\ tab
			\ hr
			\ nameserver
			\ rss
			\ hddtemp
			\ offset
			\ voffset
			\ i2c
			\ platform
			\ hwmon
			\ alignr
			\ alignc
			\ if_empty
			\ if_existing
			\ if_mounted
			\ if_running
			\ ioscheduler
			\ kernel
			\ machine
			\ mem
			\ memeasyfree
			\ memfree
			\ memmax
			\ memperc
			\ membar
			\ memgraph
			\ mixer
			\ mixerl
			\ mixerr
			\ mixerbar
			\ mixerlbar
			\ mixerrbar
			\ mails
			\ mboxscan
			\ new_mails
			\ nodename
			\ outlinecolor
			\ processes
			\ running_processes
			\ scroll
			\ lines
			\ words
			\ shadecolor
			\ stippled_hr
			\ swap
			\ swapmax
			\ swapperc
			\ swapbar
			\ sysname
			\ template0
			\ template1
			\ template2
			\ template3
			\ template4
			\ template5
			\ template6
			\ template7
			\ template8
			\ template9
			\ time
			\ utime
			\ tztime
			\ totaldown
			\ totalup
			\ updates
			\ upspeed
			\ upspeedf
			\ upspeedgraph
			\ uptime_short
			\ uptime
			\ user_names
			\ user_terms
			\ user_times
			\ user_number
			\ apm_adapter
			\ apm_battery_life
			\ apm_battery_time
			\ monitor
			\ monitor_number
			\ mpd_title
			\ mpd_artist
			\ mpd_album
			\ mpd_random
			\ mpd_repeat
			\ mpd_track
			\ mpd_name
			\ mpd_file
			\ mpd_vol
			\ mpd_bitrate
			\ mpd_status
			\ mpd_elapsed
			\ mpd_length
			\ mpd_percent
			\ mpd_bar
			\ mpd_smart
			\ xmms2_artist
			\ xmms2_album
			\ xmms2_title
			\ xmms2_genre
			\ xmms2_comment
			\ xmms2_url
			\ xmms2_status
			\ xmms2_date
			\ xmms2_tracknr
			\ xmms2_bitrate
			\ xmms2_id
			\ xmms2_size
			\ xmms2_elapsed
			\ xmms2_duration
			\ xmms2_percent
			\ xmms2_bar
			\ xmms2_playlist
			\ xmms2_timesplayed
			\ xmms2_smart
			\ audacious_status
			\ audacious_title
			\ audacious_length
			\ audacious_length_seconds
			\ audacious_position
			\ audacious_position_seconds
			\ audacious_bitrate
			\ audacious_frequency
			\ audacious_channels
			\ audacious_filename
			\ audacious_playlist_length
			\ audacious_playlist_position
			\ audacious_bar
			\ bmpx_title
			\ bmpx_artist
			\ bmpx_album
			\ bmpx_uri
			\ bmpx_track
			\ bmpx_bitrate
			\ top
			\ top_mem
			\ tail
			\ head
			\ tcp_portmon
			\ iconv_start
			\ iconv_stop
			\ entropy_avail
			\ entropy_poolsize
			\ entropy_bar
			\ smapi
			\ if_smapi_bat_installed
			\ smapi_bat_perc
			\ smapi_bat_bar

hi def link ConkyrcComment   Comment
hi def link ConkyrcSetting   Keyword
hi def link ConkyrcConstant  Constant
hi def link ConkyrcNumber    Number
hi def link ConkyrcColour    Special

hi def link ConkyrcText      String
hi def link ConkyrcVar       Identifier
hi def link ConkyrcVarName   Keyword

let b:current_syntax = "conkyrc"
