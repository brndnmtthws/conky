#! /usr/bin/lua

local usage = [[
Usage: convert.lua old_conkyrc [new_conkyrc]

Tries to convert conkyrc from the old v1.x format to the new, lua-based format.

Keep in mind that there is no guarantee that the output will work correctly
with conky, or that it will be able to convert every conkyrc. However, it
should provide a good starting point.

Altough you can use this script with only 1 arg and let it overwrite the old
config, it's suggested to use 2 args so that the new config is written in a new
file (so that you have backup if something went wrong).

For more information about the new format, read the wiki page
<http://wiki.conky.be/index.php?title=conky2rc_format>
]];

local function quote(s)
    if not s:find("[\n'\\]") then
        return "'" .. s .. "'";
    end;
    local q = '';
    while s:find(']' .. q .. ']', 1, true) do
        q = q .. '=';
    end;
    return string.format('[%s[\n%s]%s]', q, s, q);
end;

local bool_setting = {
    background = true, disable_auto_reload = true, double_buffer = true, draw_borders = true,
    draw_graph_borders = true, draw_outline = true, draw_shades = true, extra_newline = true,
    format_human_readable = true, no_buffers = true, out_to_console = true,
    out_to_ncurses = true, out_to_stderr = true, out_to_x = true, override_utf8_locale = true,
    own_window = true, own_window_argb_visual = true, own_window_transparent = true,
    short_units = true, show_graph_range = true, show_graph_scale = true,
    times_in_seconds = true, top_cpu_separate = true, uppercase = true, use_xft = true
};

local num_setting = {
    border_inner_margin = true, border_outer_margin = true, border_width = true,
    cpu_avg_samples = true, diskio_avg_samples = true, gap_x = true, gap_y = true,
    imlib_cache_flush_interval = true, imlib_cache_size = true,
    max_port_monitor_connections = true, max_text_width = true, max_user_text = true,
    maximum_width = true, mpd_port = true, music_player_interval = true, net_avg_samples = true,
    own_window_argb_value = true, pad_percents = true, stippled_borders = true,
    text_buffer_size = true, top_name_width = true, total_run_times = true,
    update_interval = true, update_interval_on_battery = true, xftalpha = true, 
    xinerama_head = true, 
};

local split_setting = {
    default_bar_size = true, default_gauge_size = true, default_graph_size = true,
    minimum_size = true
};

local colour_setting = {
    color0 = true, color1 = true, color2 = true, color3 = true, color4 = true, color5 = true,
    color6 = true, color7 = true, color8 = true, color9 = true, default_color = true,
    default_outline_color = true, default_shade_color = true, own_window_colour = true
};

local function alignment_map(value)
    local map = { m = 'middle', t = 'top', b = 'bottom', r = 'right', l = 'left' };
    if map[value] == nil then
        return value;
    else
        return map[value];
    end;
end;

local function handle(setting, value)
    setting = setting:lower();
    if setting == '' then
        return '';
    end;
    if split_setting[setting] then
        local x, y = value:match('^(%S+)%s*(%S*)$');
        local ret = setting:gsub('_size', '_width = ') .. x .. ',';
        if y ~= '' then
            ret = ret .. ' ' .. setting:gsub('_size', '_height = ') .. y .. ',';
        end;
        return '\t' .. ret;
    end;
    if bool_setting[setting] then
        value = value:lower();
        if value == 'yes' or value == 'true' or value == '1' or value == '' then
            value = 'true';
        else
            value = 'false';
        end;
    elseif not num_setting[setting] then
        if setting == 'alignment' and value:len() == 2 then
            value = alignment_map(value:sub(1,1)) .. '_' .. alignment_map(value:sub(2,2));
        elseif colour_setting[setting] and value:match('^[0-9a-fA-F]+$') then
            value = '#' .. value;
        elseif setting == 'xftfont' then
            setting = 'font';
        end;
        value = quote(value);
    end;
    return '\t' .. setting .. ' = ' .. value .. ',';
end;

local function convert(s)
    local setting, comment = s:match('^([^#]*)#?(.*)\n$');
    if comment ~= '' then
        comment = '--' .. comment;
    end;
    comment = comment .. '\n';
    return handle(setting:match('^%s*(%S*)%s*(.-)%s*$')) ..  comment;
end;

local input;
local output;

if conky == nil then --> standalone program
    -- 1 arg: arg is input and outputfile
    -- 2 args: 1st is inputfile, 2nd is outputfile
    -- 0, 3 or more args: print usage to STDERR and quit
    if #arg == 1 or #arg == 2 then
        input = io.input(arg[1]);
    else
        io.stderr:write(usage);
        return;
    end;
else
    -- we are called from conky, the filename is the first argument
    input = io.open(..., 'r');
end;


local config = input:read('*a');
input:close();

local settings, text = config:match('^(.-)TEXT\n(.*)$');

local converted = 'conky.config = {\n' .. settings:gsub('.-\n', convert) .. '};\n\nconky.text = ' ..
                quote(text) .. ';\n';

if conky == nil then
    if #arg == 2 then
        output = io.output(arg[2]);
    else
        output = io.output(arg[1]);
    end
    output:write(converted);
    output:close();
else
    return assert(loadstring(converted, 'converted config'));
end;
