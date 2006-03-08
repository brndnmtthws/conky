" Vim syntax file
" Language:	conkyrc
" Author:	Ciaran McCreesh <ciaranm@gentoo.org>
" Version:	20060307
" Copyright:	Copyright (c) 2005 Ciaran McCreesh
" Licence:	You may redistribute this under the same terms as Vim itself
"
" $Id$

if exists("b:current_syntax")
  finish
endif

syn region ConkyrcComment start=/^\s*#/ end=/$/

syn keyword ConkyrcSetting
        \ alignment
        \ background
        \ border_margin
        \ border_width
        \ cpu_avg_samples
        \ default_color
        \ default_outline_color
        \ default_shade_color
        \ double_buffer
        \ draw_borders
        \ draw_graph_borders
        \ draw_outline
        \ draw_shades
        \ font
        \ gap_x
        \ gap_y
        \ mail_spool
        \ maximum_width
        \ minimum_size
        \ min_port_monitor_connections
        \ min_port_monitors
        \ mldonkey_hostname
        \ mldonkey_login
        \ mldonkey_password
        \ mldonkey_port
        \ mpd_host
        \ mpd_password
        \ mpd_port
        \ net_avg_samples
        \ no_buffers
        \ on_bottom
        \ out_to_console
        \ override_utf8_locale
        \ own_window
        \ own_window_colour
        \ own_window_hints
        \ own_window_transparent
	\ own_window_type
        \ pad_percents
        \ stippled_borders
        \ total_run_times
        \ update_interval
        \ uppercase
        \ use_spacer
        \ use_xft
        \ wm_class_name
        \ xftalpha
        \ xftfont
        \ xmms_player

syn keyword ConkyrcConstant 
        \ above
        \ audacious
        \ below
        \ bmp
        \ bottom_left
        \ bottom_right
	\ desktop
        \ no
        \ none
	\ normal
        \ skip_pager
        \ skip_taskbar
        \ sticky
        \ top_left
        \ top_right
        \ undecorated
        \ xmms
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
	\ acpiacadapter 
	\ acpifan 
	\ acpitemp 
	\ acpitempf 
      	\ addr 
	\ adt746xcpu
      	\ adt746xfan 
	\ alignc 
	\ alignr 
	\ apm_adapter 
	\ apm_battery_life 
	\ apm_battery_time
      	\ battery 
	\ bmpx_album 
	\ bmpx_artist 
	\ bmpx_bitrate
	\ bmpx_title 
	\ bmpx_track 
      	\ bmpx_uri 
	\ buffers 
	\ cached 
	\ color 
      	\ colour 
	\ colour 
	\ cpu 
	\ cpubar 
	\ diskio 
	\ downspeed 
	\ downspeedf
	\ else 
	\ exec 
	\ execbar 
	\ execgraph 
	\ execi 
	\ execibar 
	\ execigraph 
	\ font 
	\ freq
	\ freq_dyn 
	\ freq_dyn_g 
      	\ freq_g 
	\ fs_bar 
	\ fs_free 
	\ fs_free_perc 
	\ fs_size 
	\ fs_used head
      	\ hr 
	\ i2c 
	\ i8k_ac_status 
	\ i8k_bios 
	\ i8k_buttons_status 
	\ i8k_cpu_temp 
	\ i8k_cpu_tempf
      	\ i8k_left_fan_rpm 
	\ i8k_left_fan_status 
	\ i8k_right_fan_rpm 
	\ i8k_right_fan_status
      	\ i8k_serial 
	\ i8k_version 
	\ if_existing 
	\ if_mounted 
	\ if_running 
	\ kernel 
	\ linkstatus 
	\ loadavg
      	\ machine 
	\ mails 
	\ mem 
	\ membar 
	\ memmax 
	\ memperc 
	\ ml_download_counter
	\ ml_ndownloaded_files 
	\ ml_ndownloading_files
      	\ ml_nshared_files 
	\ ml_shared_counter 
	\ ml_tcp_download_rate
	\ ml_tcp_upload_rate 
	\ ml_udp_download_rate 
      	\ ml_udp_upload_rate 
	\ ml_upload_counter 
	\ mpd_album 
      	\ mpd_artist 
	\ mpd_bar 
	\ mpd_bitrate 
	\ mpd_elapsed
      	\ mpd_length 
	\ mpd_percent 
	\ mpd_status 
	\ mpd_title 
	\ mpd_file 
	\ mpd_name 
	\ mpd_smart 
	\ mpd_vol 
	\ new_mails 
	\ nodename 
	\ offset 
	\ outlinecolor 
	\ pre_exec 
	\ processes
      	\ running_processes 
	\ seti_credit 
	\ seti_prog 
	\ seti_progbar 
	\ shadecolor 
	\ stippled_hr 
	\ swap 
	\ swapbar 
	\ swapmax 
	\ swapperc 
	\ sysname
	\ tail 
      	\ tcp_portmon 
      	\ texeci 
	\ time 
	\ top 
	\ top_mem 
	\ totaldown 
	\ totalup 
	\ updates 
	\ upspeed 
	\ upspeedf
      	\ upspeedgraph 
	\ uptime 
	\ uptime_short 
	\ voffset
	\ xmms_bar 
	\ xmms_bitrate 
	\ xmms_channels 
	\ xmms_filename 
	\ xmms_frequency
      	\ xmms_length 
	\ xmms_length_seconds 
	\ xmms_playlist_length
	\ xmms_playlist_position 
      	\ xmms_position 
	\ xmms_position_seconds 
	\ xmms_status 
	\ xmms_title

hi def link ConkyrcComment   Comment
hi def link ConkyrcSetting   Keyword
hi def link ConkyrcConstant  Constant
hi def link ConkyrcNumber    Number
hi def link ConkyrcColour    Special

hi def link ConkyrcText      String
hi def link ConkyrcVar       Identifier
hi def link ConkyrcVarName   Keyword

let b:current_syntax = "conkyrc"

